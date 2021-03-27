#pragma once

#include <cstdint>

class bme280 {
 public:
	explicit bme280();
	explicit bme280(bme280&&);

	~bme280();

	void print_data();

	struct data {
		uint64_t deca_humidity;
		uint64_t deca_kelvin;
	};

	[[nodiscard]] data get_data();

 private:
	int fd;
};