#include "sds011.hpp"

#include <exception>
#include <iostream>

int main(int, char**) {
	try {
		sds011 h{"/dev/ttyUSB0"};
		h.set_sleep(false);
		h.firmware_ver();
		h.set_working_period(0);
		h.set_mode(1);
		// for (int i = 0 ; i < 10; ++i ) {
		h.query_data();
		// 	sleep(2);
		// }
	} catch (const std::exception& e) {
		std::cerr << e.what() << "\n";
	}

	return 0;
}