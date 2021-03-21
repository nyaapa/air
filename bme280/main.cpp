#include "bme280.hpp"

#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <wiringPiI2C.h>

int main() {
	bme280{}.print_data();
	return 0;
}