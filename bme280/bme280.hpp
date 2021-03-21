#pragma once

class bme280 {
 public:
	bme280();
	~bme280();
	void print_data();

 private:
	const int fd;
};