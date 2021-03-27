#include "bme280/bme280.hpp"
#include "s8/s8.hpp"
#include "sds011/sds011.hpp"

#include "lib/cxxopts.hpp"

#include <fmt/core.h>
#include <chrono>
#include <exception>
#include <iostream>
#include <optional>
#include <thread>
#include <type_traits>
#include <string>

namespace {
void init_handler(auto& h) {
	if (!h) {
		try {
			h.emplace();
		} catch (const std::exception& e) {
			fmt::print(stderr, "Failed to init: {}\n", e.what());
			h.reset();
		}
	}
}

void init_handler(auto& h, auto&& init) requires std::is_rvalue_reference_v<decltype(init)> {
	if (!h) {
		try {
			h.emplace();
			init(h);
		} catch (const std::exception& e) {
			fmt::print(stderr, "Failed to init: {}\n", e.what());
			h.reset();
		}
	}
}

void print_data(auto& h) {
	if (h) {
		try {
			h->print_data();
		} catch (const std::exception& e) {
			fmt::print(stderr, "Failed to print data: {}\n", e.what());
			h.reset();
		}
	}
}

void add_data(auto& h, auto&& adder) requires std::is_rvalue_reference_v<decltype(adder)> {
	if (h) {
		try {
			const auto data = h->get_data();
			adder(data);
		} catch (const std::exception& e) {
			fmt::print(stderr, "Failed to add data: {}\n", e.what());
			h.reset();
		}
	}
}
};  // namespace

int main(int argc, char** argv) {
	cxxopts::Options options("air", "Air quality");

	uint sleep_time = 0;
	bool json = false;
	std::string name;
	std::string receiver;

	options.add_options()
		("s,sleep-time", "sleep time between probes", cxxopts::value<uint>(sleep_time))
		("j,json", "response in json", cxxopts::value<bool>(json))
		("n,name", "name of that sender, required for sending", cxxopts::value<std::string>(name))
		("r,receiver", "receiver address, requires name", cxxopts::value<std::string>(receiver))
		("h,help", "Print help");

	auto result = options.parse(argc, argv);

	if (result.count("help")) {
		fmt::print("{}\n", options.help({""}));
		exit(0);
	}

	if (!receiver.empty() && name.empty()) {
		fmt::print("Name should be specified with receiver {}\n", options.help({""}));
		exit(0);
	}

	if (std::find_if(name.begin(), name.end(), [](char c){return !isalnum(c);}) != name.end()) {
		fmt::print("Name should be only alphanumerical {}\n", options.help({""}));
		exit(0);
	}

	std::optional<s8> s8h;
	std::optional<sds011> sds011h;
	std::optional<bme280> bme280h;

	do {
		init_handler(s8h);

		init_handler(sds011h, [](auto& h) {
			h->set_sleep(false);
			h->set_working_period(0);
			h->set_mode(1);
		});

		init_handler(bme280h);

		if (json) {
			std::string result{fmt::format("\"name\":\"{}\"", name)};
			add_data(s8h, [&result](const auto& data) mutable { result += fmt::format(",\"co2\":{}", data.co2); });
			add_data(sds011h, [&result](const auto& data) mutable {
				result += fmt::format(",\"deca_pm25\":{},\"deca_pm10\":{}", data.deca_pm25, data.deca_pm10);
			});
			add_data(bme280h, [&result](const auto& data) mutable {
				result += fmt::format(",\"deca_humidity\":{},\"deca_kelvin\":{}", data.deca_humidity, data.deca_kelvin);
			});
			fmt::print("{{{}}}\n", result);
		} else {
			print_data(s8h);
			print_data(sds011h);
			print_data(bme280h);
		}

		if (sleep_time) {
			fmt::print(stderr, "---------------------------------------------\n");
			std::this_thread::sleep_for(std::chrono::seconds(sleep_time));
		}
	} while (sleep_time);

	return 0;
}
