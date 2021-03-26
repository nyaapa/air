// heavily based on https://github.com/paulvha/sps30_on_raspberry

#include "sds011.hpp"

#include <fcntl.h>
#include <unistd.h>

#include <fmt/core.h>
#include <exception>
#include <numeric>
#include <string_view>

namespace {
constexpr std::string_view path = "/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0";

constexpr uint8_t command_idx = 2;
constexpr uint8_t data1_idx = 3;
constexpr uint8_t data2_idx = 4;

enum class command : uint8_t {
	mode = 2,
	query = 4,
	id = 5,
	sleep = 6,
	firmware = 7,
	working_period = 8,
};

struct version {
	uint8_t year;
	uint8_t month;
	uint8_t day;
	uint8_t id_le[2];
	uint8_t crc;
	uint8_t end;
};

struct data {
	uint8_t pm25_le[2];
	uint8_t pm10_le[2];
	uint8_t device_le[2];
	uint8_t crc;
	uint8_t end;
};

ulong parse_le(const uint8_t (&le)[2]) {
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
	tty.c_cc[VMIN] = 0;
	tty.c_cc[VTIME] = 5;

	if (tcsetattr(fh, TCSANOW, &tty) != 0)
		throw std::runtime_error(fmt::format("Failed to tcsetattr: {}", strerror(errno)));
}
};  // namespace

sds011::sds011() : fh{open(path.data(), O_RDWR | O_NOCTTY | O_SYNC)} {
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

sds011::sds011(sds011&& o) : fh{o.fh}, tty_back{o.tty_back} {
	o.fh = 0;
}

sds011::~sds011() {
	if (fh) {
		if (tcsetattr(fh, TCSANOW, &tty_back) < 0)
			fmt::print(stderr, "Failed to reset tcsetattr: {}", strerror(errno));

		close(fh);
	}
}

void sds011::firmware_ver() {
	request[command_idx] = static_cast<uint8_t>(command::firmware);
	request[data1_idx] = 0;
	request[data2_idx] = 0;
	send_command();
	print_version();
}

void sds011::set_sleep(bool sleep) {
	request[command_idx] = static_cast<uint8_t>(command::sleep);
	request[data1_idx] = 1;
	request[data2_idx] = !sleep;
	send_command();
}

void sds011::set_working_period(uint8_t period) {
	request[command_idx] = static_cast<uint8_t>(command::working_period);
	request[data1_idx] = 1;
	request[data2_idx] = period;
	send_command();
}

void sds011::set_mode(uint8_t mode) {
	request[command_idx] = static_cast<uint8_t>(command::mode);
	request[data1_idx] = 1;
	request[data2_idx] = mode;
	send_command();
}

void sds011::send_command() {
	constexpr uint8_t response_tail = 0xab;
	constexpr uint8_t checksum_request_idx = 17;
	constexpr uint8_t checksum_response_idx = 8;

	request[checksum_request_idx] = std::accumulate(&request[command_idx], &request[checksum_request_idx], 0u);

#ifndef NDEBUG
	fmt::print("< ");
	for (auto el : request)
		fmt::print("{:x} ", el);
	fmt::print("\n");
#endif

	if (write(fh, request, std::extent_v<decltype(request)>) != std::extent_v<decltype(request)>)
		throw std::runtime_error(fmt::format("USB control write failed: {}", +strerror(errno)));

	if (read(fh, response, std::extent_v<decltype(response)>) != std::extent_v<decltype(response)>)
		throw std::runtime_error(fmt::format("USB read failed: {}", strerror(errno)));

#ifndef NDEBUG
	fmt::print("> ");
	for (auto el : response)
		fmt::print("{:x} ", el);
	fmt::print("\n");
#endif

	uint8_t checksum = std::accumulate(&response[command_idx], &response[checksum_response_idx], 0u);
	if (response[checksum_response_idx] != checksum || response[checksum_response_idx + 1] != response_tail)
		throw std::runtime_error("CRC check failed");
}

void sds011::print_version() {
	version& x = *reinterpret_cast<version*>(&response[data1_idx]);

	fmt::print("Y: {}, M: {}, D: {}, ID: 0x{:x}\n", x.year, x.month, x.day, parse_le(x.id_le));
}

void sds011::print_data() {
	auto data = this->get_data();
	fmt::print("PM10: {}\n", data.deca_pm10 / 10.0);
	fmt::print("PM2.5: {}\n", data.deca_pm25 / 10.0);
}

sds011::data sds011::get_data() {
	constexpr uint8_t response_head = 0xc0;
	constexpr uint8_t response_head_idx = 1;

	request[command_idx] = static_cast<uint8_t>(command::query);
	request[data1_idx] = 0;
	request[data2_idx] = 0;
	send_command();

	if (response[response_head_idx] != response_head) {
		throw std::runtime_error("Can't read response from sds011");
	} else {
		::data& x = *reinterpret_cast<::data*>(&response[command_idx]);

		return {.deca_pm25 = parse_le(x.pm25_le), .deca_pm10 = parse_le(x.pm10_le)};
	}
}