#include "sds011/sds011.hpp"
#include "s8/s8.hpp"
#include "bme280/bme280.hpp"

#include "lib/cxxopts.hpp"

#include <exception>
#include <iostream>
#include <fmt/core.h>
#include <optional>
#include <type_traits>
#include <thread>
#include <chrono>

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

void add_data(auto& h, auto&& adder) {
	try {
		const auto data = h->get_data();
		adder(data);
	} catch (const std::exception& e) {
		fmt::print("Failed to add data: {}\n", e.what());
	}
}
};

int main(int argc, char** argv) {
	cxxopts::Options options("air", "Air quality");

	uint sleep_time = 0;
	bool json = false;

	options
		.add_options()
		("s,sleep-time", "sleep time between probes", cxxopts::value<uint>(sleep_time))
		("j,json", "response in json", cxxopts::value<bool>(json))
		("h,help", "Print help")
		;

	auto result = options.parse(argc, argv);

	if (result.count("help")) {
		fmt::print("{}\n", options.help({""}));
		exit(0);
	}

	auto s8h = init_handler<s8>();
	auto sds011h = init_handler<sds011>([](auto& h){
		h->set_sleep(false);
		h->set_working_period(0);
		h->set_mode(1);
	});
	auto bme280h = init_handler<bme280>();

	do {
		if (json) {
			std::string result;
			add_data(s8h, [&result](const auto& data) mutable { 
				result += fmt::format("\"co2\":{}", data.co2);
			});
			add_data(sds011h, [&result](const auto& data) mutable {
				result += fmt::format("{}\"deca_pm25\":{},\"deca_pm10\":{}", (result.empty() ? "" : ","), data.deca_pm25, data.deca_pm10);
			});
			add_data(bme280h, [&result](const auto& data) mutable {
				result += fmt::format("{}\"deca_humidity\":{},\"deca_kelvin\":{}", (result.empty() ? "" : ","), data.deca_humidity, data.deca_kelvin);
			});
			fmt::print("{{{}}}\n", result);
		} else {
			print_data(s8h);
			print_data(sds011h);
			print_data(bme280h);
		}

		if (sleep_time) {
			fmt::print("---------------------------------------------\n");
			std::this_thread::sleep_for(std::chrono::seconds(sleep_time));
		}
	} while (sleep_time);

	return 0;
}
