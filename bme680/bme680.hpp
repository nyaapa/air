#pragma once

#include <cstdint>
#include <string>

class bme680 {
 public:
	explicit bme680();
	explicit bme680(bme680&&);

	~bme680();

	void print_data();

	struct data {
		std::string data;
	};

	[[nodiscard]] data get_data();

 private:
	int pid;
};