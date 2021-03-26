#include "sds011/sds011.hpp"
#include "s8/s8.hpp"
#include "bme280/bme280.hpp"

#include <exception>
#include <iostream>
#include <fmt/core.h>
#include <optional>

namespace {
template<typename T>
std::optional<T> init_handler() {
	std::optional<T> h;
	try {
		h.emplace();
		return h;
	} catch (const std::exception& e) {
		fmt::print("Failed to init: {}\n", e.what());
		return {};
	}
}

template<typename T, typename F>
std::optional<T> init_handler(F init) {
	std::optional<T> h;
	try {
		h.emplace();
		init(h);
		return h;
	} catch (const std::exception& e) {
		fmt::print("Failed to init: {}\n", e.what());
		return {};
	}
}

template<typename T>
void print_data(std::optional<T>& h) {
	try {
		h->print_data();
	} catch (const std::exception& e) {
		fmt::print("Failed to print data: {}\n", e.what());
	}
}
};

int main(int, char**) {
	auto s8h = init_handler<s8>();
	auto sds011h = init_handler<sds011>([](std::optional<sds011>& h){
		h->set_sleep(false);
		h->set_working_period(0);
		h->set_mode(1);
	});
	auto bme280h = init_handler<bme280>();

	print_data(s8h);
	print_data(sds011h);
	print_data(bme280h);

	return 0;
}