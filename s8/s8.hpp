#pragma once

#include <termios.h>
#include <string>

class s8 {
 public:
	explicit s8();

	~s8();

	void print_data();

 private:
	const int fh;

	termios tty_back;

	uint8_t response[7];

	void send_command();
};