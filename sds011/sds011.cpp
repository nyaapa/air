#include "sds011.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <fmt/core.h>
#include <exception>
#include <numeric>

namespace {
enum class command : unsigned char {
	mode = 2,
	query = 4,
	id = 5,
	sleep = 6,
	firmware = 7,
	working_period = 8,
};

struct version {
	unsigned char year;
	unsigned char month;
	unsigned char day;
	unsigned char id_le[2];
	unsigned char crc;
	unsigned char end;
};

struct data {
	unsigned char pm25_le[2];
	unsigned char pm10_le[2];
	unsigned char device_le[2];
	unsigned char crc;
	unsigned char end;
};

ulong parse_le(const unsigned char (&le)[2]) {
	return (le[1] << 8) + le[0];
}

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
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 1;

	if (tcsetattr(fh, TCSANOW, &tty) != 0)
		throw std::runtime_error(fmt::format("Failed to tcsetattr: {}", strerror(errno)));

	tty.c_cflag |= CS8;      /* 8-bit characters */
	tty.c_cflag &= ~PARENB;  /* no parity bit */
	tty.c_cflag &= ~CSTOPB;  /* on#pack-TEMPLATE%2cLISTly need 1 stop bit */
	tty.c_cflag &= ~CRTSCTS; /* no hardware flowcontrol */

	/* setup for non-canonical mode */
	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tty.c_oflag &= ~OPOST;

	/* fetch bytes as they become available */
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 1;

	if (tcsetattr(fh, TCSANOW, &tty) != 0)
		throw std::runtime_error(fmt::format("Failed to tcsetattr: {}", strerror(errno)));
}

void set_blocking(int fh, int mcount) {
	termios tty;

	if (tcgetattr(fh, &tty) < 0)
		throw std::runtime_error(fmt::format("Failed to tcgetattr: {}", strerror(errno)));

	tty.c_cc[VMIN] = mcount ? 1 : 0;
	tty.c_cc[VTIME] = 5; /* half second timer */

	if (tcsetattr(fh, TCSANOW, &tty) < 0)
		throw std::runtime_error(fmt::format("Failed to tcsetattr: {}", strerror(errno)));
}
};  // namespace

sds011::sds011(const char* const path) : fh{open(path, O_RDWR | O_NOCTTY | O_SYNC)} {
	if (fh < 0)
		throw std::runtime_error(fmt::format("Failed to open '{}': {}", path, strerror(errno)));

	try {
		if (tcgetattr(fh, &tty_back) < 0)
			throw std::runtime_error(fmt::format("Failed to tcgetattr: {}", strerror(errno)));

		configure_interface(fh, B9600);
		set_blocking(fh, 0);

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

sds011::~sds011() {
	if (tcsetattr(fh, TCSANOW, &tty_back) < 0)
		fmt::print(stderr, "Failed to reset tcsetattr: {}", strerror(errno));

	close(fh);
}

void sds011::firmware_ver() {
	request[2] = static_cast<unsigned char>(command::firmware);
	request[3] = 0;
	request[4] = 0;
	send_command();
	print_version();
}

void sds011::set_sleep(bool sleep) {
	request[2] = static_cast<unsigned char>(command::sleep);
	request[3] = 1;
	request[4] = !sleep;
	send_command();
}

void sds011::set_working_period(unsigned char period) {
	request[2] = static_cast<unsigned char>(command::working_period);
	request[3] = 1;
	request[4] = period;
	send_command();
}

void sds011::set_mode(unsigned char mode) {
	request[2] = static_cast<unsigned char>(command::mode);
	request[3] = 1;
	request[4] = mode;
	send_command();
}

void sds011::query_data() {
	request[2] = static_cast<unsigned char>(command::query);
	request[3] = 0;
	request[4] = 0;
	send_command();
	print_data();
}

void sds011::send_command() {
	request[17] = std::accumulate(&request[2], &request[17], 0u);

#ifndef NDEBUG
	fmt::print("< ");
	for (auto el : request)
		fmt::print("{:x} ", el);
	fmt::print("\n");
#endif

	if (write(fh, request, std::extent_v<decltype(request)>) != std::extent_v<decltype(request)>)
		throw std::runtime_error(fmt::format("USB control write failed: {}", +strerror(errno)));

	if (read(fh, response, 10) != 10)
		throw std::runtime_error(fmt::format("USB read failed: {}", strerror(errno)));

#ifndef NDEBUG
	fmt::print("> ");
	for (auto el : response)
		fmt::print("{:x} ", el);
	fmt::print("\n");
#endif
}

void sds011::print_version() {
	version& x = *reinterpret_cast<version*>(&response[3]);
	unsigned char checksum = std::accumulate(&response[2], &response[8], 0u);

	fmt::print(
	    "Y: {}, M: {}, D: {}, ID: 0x{:x}, CRC={}\n", x.year, x.month, x.day, parse_le(x.id_le),
	    x.crc == checksum && x.end == 0xab ? "OK" : "NOK");
}

void sds011::print_data() {
	if (response[1] != 0xc0) {
		fmt::print("<null>\n");
	} else {
		data& x = *reinterpret_cast<data*>(&response[2]);
		unsigned char checksum = std::accumulate(&response[2], &response[8], 0u);

		fmt::print(
		    "PM 2.5: {} μg/m^3  PM 10: {} μg/m^3 CRC={}\n", (parse_le(x.pm25_le) / 10.0), (parse_le(x.pm10_le) / 10.0),
		    (x.crc == checksum && x.end == 0xab ? "OK" : "NOK"));
	}
}