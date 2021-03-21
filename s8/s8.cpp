#include "s8.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <fmt/core.h>
#include <exception>
#include <numeric>

namespace {
constexpr uint8_t request[7] = {0xFE, 0x44, 0x00, 0x08, 0x02, 0x9F, 0x25};

void configure_interface(int fh, int speed) {
	termios tty;

	if (tcgetattr(fh, &tty) < 0)
		throw std::runtime_error(fmt::format("Failed to tcgetattr: {}", strerror(errno)));

	cfsetospeed(&tty, (speed_t)speed);
	cfsetispeed(&tty, (speed_t)speed);

	tty.c_cflag |= (CLOCAL | CREAD); /* ignore modem controls */
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;      /* 8-bit characters */
	tty.c_cflag &= ~PARENB;  /* no parity bit */
	tty.c_cflag &= ~CSTOPB;  /* only need 1 stop bit */
	tty.c_cflag &= ~CRTSCTS; /* no hardware flowcontrol */

	/* setup for non-canonical mode */
	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tty.c_oflag &= ~OPOST;

	/* fetch bytes as they become available */
	tty.c_cc[VMIN] = 7;
	tty.c_cc[VTIME] = 5;

	if (tcsetattr(fh, TCSANOW, &tty) != 0)
		throw std::runtime_error(fmt::format("Failed to tcsetattr: {}", strerror(errno)));
}
};  // namespace

s8::s8(const char* const path) : fh{open(path, O_RDWR | O_NOCTTY | O_SYNC)} {
	if (fh < 0)
		throw std::runtime_error(fmt::format("Failed to open '{}': {}", path, strerror(errno)));

	try {
		if (tcgetattr(fh, &tty_back) < 0)
			throw std::runtime_error(fmt::format("Failed to tcgetattr: {}", strerror(errno)));

		configure_interface(fh, B9600);

		/* There is a problem with flushing buffers on a serial USB that can
		 * not be solved. The only thing one can try is to flush any buffers
		 * after some delay:
		 *
		 * https://bugzilla.kernel.org/show_bug.cgi?id=5730
		 * https://stackoverflow.com/questions/13013387/clearing-the-serial-ports-buffer
		 */
		usleep(10000);
		tcflush(fh, TCIOFLUSH);
	} catch (...) {
		close(fh);
		throw;
	}
}

s8::~s8() {
	if (tcsetattr(fh, TCSANOW, &tty_back) < 0)
		fmt::print(stderr, "Failed to reset tcsetattr: {}", strerror(errno));

	close(fh);
}

void s8::query_data() {
	send_command();
	print_data();
}

void s8::send_command() {
#ifndef NDEBUG
	fmt::print("< ");
	for (auto el : request)
		fmt::print("{:x} ", el);
	fmt::print("\n");
#endif

	if (write(fh, request, std::extent_v<decltype(request)>) != std::extent_v<decltype(request)>)
		throw std::runtime_error(fmt::format("USB control write failed: {}", +strerror(errno)));

	if (auto bytes = read(fh, response, std::extent_v<decltype(response)>); bytes != std::extent_v<decltype(response)>)
		throw std::runtime_error(fmt::format("USB read failed, read only {}: {}", bytes, strerror(errno)));

#ifndef NDEBUG
	fmt::print("> ");
	for (auto el : response)
		fmt::print("{:x} ", el);
	fmt::print("\n");
#endif
}

void s8::print_data() {
	ulong data = (response[3] << 8) + response[4];

	fmt::print("CO2: {} PPM\n", data);
}