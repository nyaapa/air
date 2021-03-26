#pragma once

#include <termios.h>
#include <string>
#include <cstdint>

class s8 {
 public:
	explicit s8();
	explicit s8(s8&&);

	~s8();

	void print_data();

	struct data { uint64_t co2; };

	[[nodiscard]] data get_data();
 private:
	int fh;

	termios tty_back;

	uint8_t response[7];

	void send_command();
};