#include "s8.hpp"

#include <exception>
#include <iostream>

int main(int, char**) {
	try {
		s8 h{"/dev/ttyUSB1"};
		// for (int i = 0 ; i < 10; ++i ) {
		h.print_data();
		// 	sleep(2);
		// }
	} catch (const std::exception& e) {
		std::cerr << e.what() << "\n";
	}

	return 0;
}
