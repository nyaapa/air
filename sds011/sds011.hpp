#pragma once

#include <termios.h>
#include <string>

class sds011 {
 public:
	sds011(const char* const path);

	~sds011();

	void firmware_ver();

	void set_sleep(bool sleep);

	void set_working_period(unsigned char period);

	void set_mode(unsigned char mode);

	void query_data();

 private:
	const int fh;

	termios tty_back;

	unsigned char response[10];
	unsigned char request[19] = {0xaa, 0xb4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 0, 0xab};

	void send_command();

	void print_version();

	void print_data();
};