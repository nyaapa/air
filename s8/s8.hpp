#pragma once

#include <termios.h>
#include <string>

class s8 {
 public:
	s8(const char* const path);

	~s8();

	void query_data();

 private:
	const int fh;

	termios tty_back;

	uint8_t response[7];

	void send_command();
};