#include "sds011/sds011.hpp"
#include "s8/s8.hpp"
#include "bme280/bme280.hpp"

#include <exception>
#include <iostream>
#include <fmt/core.h>
#include <optional>
#include <type_traits>

namespace {
template<typename T>
auto init_handler() {
	std::optional<T> h;

	try {
		h.emplace();
	} catch (const std::exception& e) {
		h.reset();
		fmt::print("Failed to init: {}\n", e.what());
	}

	return h;
}

template<typename T>
auto init_handler(auto&& init)
	requires std::is_rvalue_reference_v<decltype(init)> 
{
	std::optional<T> h;

	try {
		h.emplace();
		init(h);
	} catch (const std::exception& e) {
		h.reset();
		fmt::print("Failed to init: {}\n", e.what());
	}

	return h;
}

void print_data(auto& h) {
	try {
		h->print_data();
	} catch (const std::exception& e) {
		fmt::print("Failed to print data: {}\n", e.what());
	}
}
};

int main(int, char**) {
	auto s8h = init_handler<s8>();
	auto sds011h = init_handler<sds011>([](auto& h){
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