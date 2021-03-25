#include "sds011/sds011.hpp"
#include "s8/s8.hpp"
#include "bme280/bme280.hpp"

#include <exception>
#include <iostream>

int main(int, char**) {
	try {
		sds011 h{};
		h.set_sleep(false);
		h.set_working_period(0);
		h.set_mode(1);
		h.print_data();
	} catch (const std::exception& e) {
		std::cerr << e.what() << "\n";
	}

	try {
		s8 h{};
		h.print_data();
	} catch (const std::exception& e) {
		std::cerr << e.what() << "\n";
	}

	bme280{}.print_data();

	return 0;
}