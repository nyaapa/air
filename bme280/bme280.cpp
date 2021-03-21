// Heavily based on https://github.com/andreiva/raspberry-pi-bme280

#include "bme280.hpp"
#include <errno.h>
#include <fmt/core.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <wiringPiI2C.h>

namespace {

constexpr uint16_t bme280_address = 0x76;

constexpr uint16_t bme280_register_dig_t1 = 0x88;
constexpr uint16_t bme280_register_dig_t2 = 0x8A;
constexpr uint16_t bme280_register_dig_t3 = 0x8C;
constexpr uint16_t bme280_register_dig_p1 = 0x8E;
constexpr uint16_t bme280_register_dig_p2 = 0x90;
constexpr uint16_t bme280_register_dig_p3 = 0x92;
constexpr uint16_t bme280_register_dig_p4 = 0x94;
constexpr uint16_t bme280_register_dig_p5 = 0x96;
constexpr uint16_t bme280_register_dig_p6 = 0x98;
constexpr uint16_t bme280_register_dig_p7 = 0x9A;
constexpr uint16_t bme280_register_dig_p8 = 0x9C;
constexpr uint16_t bme280_register_dig_p9 = 0x9E;
constexpr uint16_t bme280_register_dig_h1 = 0xA1;
constexpr uint16_t bme280_register_dig_h2 = 0xE1;
constexpr uint16_t bme280_register_dig_h3 = 0xE3;
constexpr uint16_t bme280_register_dig_h4 = 0xE4;
constexpr uint16_t bme280_register_dig_h5 = 0xE5;
constexpr uint16_t bme280_register_dig_h6 = 0xE7;
constexpr uint16_t bme280_register_controlhumid = 0xF2;
constexpr uint16_t bme280_register_control = 0xF4;
constexpr uint16_t bme280_register_config = 0xF5;
constexpr uint16_t bme280_register_pressuredata = 0xF7;

struct bme280_calib_data {
	int32_t dig_T1;
	int32_t dig_T2;
	int32_t dig_T3;

	uint16_t dig_P1;
	int64_t dig_P2;
	int64_t dig_P3;
	int64_t dig_P4;
	int64_t dig_P5;
	int64_t dig_P6;
	int64_t dig_P7;
	int64_t dig_P8;
	int64_t dig_P9;

	int32_t dig_H1;
	int32_t dig_H2;
	int32_t dig_H3;
	int32_t dig_H4;
	int32_t dig_H5;
	int32_t dig_H6;
};

/*
 * Raw sensor measurement data from bme280
 */
struct bme280_raw_data {
	uint16_t deca_temperature_k;
	uint16_t deca_pressure;
	uint16_t deca_humidity;
};

uint32_t read20(int fd) {
	auto a = static_cast<uint8_t>(wiringPiI2CRead(fd));
	auto b = static_cast<uint8_t>(wiringPiI2CRead(fd));
	auto c = static_cast<uint8_t>(wiringPiI2CRead(fd));
	return (static_cast<uint32_t>(a) << 12) + (static_cast<uint32_t>(b) << 4) + (c >> 4);
}

uint32_t read16(int fd) {
	auto a = static_cast<uint8_t>(wiringPiI2CRead(fd));
	auto b = static_cast<uint8_t>(wiringPiI2CRead(fd));
	return (static_cast<uint32_t>(a) << 8) + b;
}

int32_t get_temperature_calibration(bme280_calib_data& cal, int32_t adc_t) {
	int32_t var1 = ((((adc_t >> 3) - (cal.dig_T1 << 1))) * cal.dig_T2) >> 11;

	int32_t var2 = (((((adc_t >> 4) - cal.dig_T1) * ((adc_t >> 4) - cal.dig_T1)) >> 12) * cal.dig_T3) >> 14;

	return var1 + var2;
}

bme280_calib_data read_calibration_data(const int fd) {
	bme280_calib_data result;

	result.dig_T1 = static_cast<uint16_t>(wiringPiI2CReadReg16(fd, bme280_register_dig_t1));
	result.dig_T2 = static_cast<int16_t>(wiringPiI2CReadReg16(fd, bme280_register_dig_t2));
	result.dig_T3 = static_cast<int16_t>(wiringPiI2CReadReg16(fd, bme280_register_dig_t3));

	result.dig_P1 = static_cast<uint16_t>(wiringPiI2CReadReg16(fd, bme280_register_dig_p1));
	result.dig_P2 = static_cast<int16_t>(wiringPiI2CReadReg16(fd, bme280_register_dig_p2));
	result.dig_P3 = static_cast<int16_t>(wiringPiI2CReadReg16(fd, bme280_register_dig_p3));
	result.dig_P4 = static_cast<int16_t>(wiringPiI2CReadReg16(fd, bme280_register_dig_p4));
	result.dig_P5 = static_cast<int16_t>(wiringPiI2CReadReg16(fd, bme280_register_dig_p5));
	result.dig_P6 = static_cast<int16_t>(wiringPiI2CReadReg16(fd, bme280_register_dig_p6));
	result.dig_P7 = static_cast<int16_t>(wiringPiI2CReadReg16(fd, bme280_register_dig_p7));
	result.dig_P8 = static_cast<int16_t>(wiringPiI2CReadReg16(fd, bme280_register_dig_p8));
	result.dig_P9 = static_cast<int16_t>(wiringPiI2CReadReg16(fd, bme280_register_dig_p9));

	result.dig_H1 = static_cast<uint8_t>(wiringPiI2CReadReg8(fd, bme280_register_dig_h1));
	result.dig_H2 = static_cast<int16_t>(wiringPiI2CReadReg16(fd, bme280_register_dig_h2));
	result.dig_H3 = static_cast<uint8_t>(wiringPiI2CReadReg8(fd, bme280_register_dig_h3));
	result.dig_H4 = (wiringPiI2CReadReg8(fd, bme280_register_dig_h4) << 4) |
	                (wiringPiI2CReadReg8(fd, bme280_register_dig_h4 + 1) & 0xF);
	result.dig_H5 = (wiringPiI2CReadReg8(fd, bme280_register_dig_h5 + 1) << 4) |
	                (wiringPiI2CReadReg8(fd, bme280_register_dig_h5) >> 4);
	result.dig_H6 = static_cast<int8_t>(wiringPiI2CReadReg8(fd, bme280_register_dig_h6));

	return result;
}

float deca_celcius(int32_t t_fine) {
	float t = (t_fine * 5 + 128) >> 8;
	return t / 10;
}

float deca_pressure(int32_t adc_P, bme280_calib_data& cal, int32_t t_fine) {
	int64_t var1 = t_fine, var2, p;

	var1 -= 128000;
	var2 = var1 * var1 * cal.dig_P6;
	var2 = var2 + ((var1 * cal.dig_P5) << 17);
	var2 = var2 + ((cal.dig_P4) << 35);
	var1 = ((var1 * var1 * cal.dig_P3) >> 8) + ((var1 * cal.dig_P2) << 12);
	var1 = (((1ll << 47) + var1)) * (cal.dig_P1) >> 33;

	if (var1 == 0) {
		return 0;  // avoid exception caused by division by zero
	}
	p = 1048576 - adc_P;
	p = (((p << 31) - var2) * 3125) / var1;
	var1 = (cal.dig_P9 * (p >> 13) * (p >> 13)) >> 25;
	var2 = (cal.dig_P8 * p) >> 19;

	p = ((p + var1 + var2) >> 8) + (cal.dig_P7 << 4);
	return p / 2560.0;
}

float deca_humidity(int32_t adc_H, bme280_calib_data& cal, int32_t t_fine) {
	int32_t v_x1_u32r = t_fine - 76800;

	v_x1_u32r =
	    (((((adc_H << 14) - (cal.dig_H4 << 20) - (cal.dig_H5 * v_x1_u32r)) + 16384) >> 15) *
	     (((((((v_x1_u32r * cal.dig_H6) >> 10) * (((v_x1_u32r * cal.dig_H3) >> 11) + 32768)) >> 10) + 2097152) *
	           cal.dig_H2 +
	       8192) >>
	      14));

	v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * cal.dig_H1) >> 4));

	v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
	v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
	float h = (v_x1_u32r >> 12);
	return h / 102.4;
}

bme280_raw_data read_data(const int fd) {
	auto cal = read_calibration_data(fd);

	wiringPiI2CWriteReg8(fd, bme280_register_controlhumid, 0x01);  // humidity oversampling x 1
	wiringPiI2CWriteReg8(fd, bme280_register_control, 0x25);  // pressure and temperature oversampling x 1, mode normal

	wiringPiI2CWrite(fd, bme280_register_pressuredata);

	auto pressure = read20(fd);
	auto temperature = read20(fd);
	auto humidity = read16(fd);

	int32_t t_fine = get_temperature_calibration(cal, temperature);

	return {
	    .deca_temperature_k = static_cast<uint16_t>(deca_celcius(t_fine) + 2731.5),
	    .deca_pressure = static_cast<uint16_t>(deca_pressure(pressure, cal, t_fine)),
	    .deca_humidity = static_cast<uint16_t>(deca_humidity(humidity, cal, t_fine)),
	};
}

};  // namespace

bme280::bme280() : fd{wiringPiI2CSetup(bme280_address)} {
	if (fd < 0)
		throw std::runtime_error("Device not found");
}

bme280::~bme280() {
	close(fd);
}

void bme280::print_data() {
	auto data = read_data(fd);

	fmt::print("{}Temp\n", data.deca_temperature_k);
	fmt::print("{}Humi\n", data.deca_humidity);
}