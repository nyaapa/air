#pragma once

#include <termios.h>
#include <string>
#include <cstdint>

class sds011 {
 public:
	explicit sds011();

	~sds011();

	void firmware_ver();

	void set_sleep(bool sleep);

	void set_working_period(uint8_t period);

	void set_mode(uint8_t mode);

	void print_data();

	struct data { uint64_t deca_pm25; uint64_t deca_pm10; };

	[[nodiscard]] data get_data();
 private:
	const int fh;

	termios tty_back;

	uint8_t response[10];
	uint8_t request[19] = {0xaa, 0xb4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0, 0xab};

	void send_command();

	void print_version();
};