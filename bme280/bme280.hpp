#pragma once

class bme280 {
 public:
	explicit bme280();
	~bme280();
	void print_data();

 private:
	const int fd;
};