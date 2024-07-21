#include "bme680.hpp"

#include <unistd.h>
#include <cstdlib>
#include <signal.h>
#include <sys/wait.h>

#include <fmt/core.h>
#include <fstream>
#include <stdexcept>

namespace {

constexpr std::string_view sorry = R"(
import bme680
import time
import tempfile
import os
import shutil

sensor = bme680.BME680(bme680.I2C_ADDR_SECONDARY)

sensor.set_humidity_oversample(bme680.OS_2X)
sensor.set_pressure_oversample(bme680.OS_4X)
sensor.set_temperature_oversample(bme680.OS_8X)
sensor.set_filter(bme680.FILTER_SIZE_3)

sensor.set_gas_status(bme680.ENABLE_GAS_MEAS)
sensor.set_gas_heater_temperature(320)
sensor.set_gas_heater_duration(150)
sensor.select_gas_heater_profile(0)

while True:
    if sensor.get_sensor_data():
        output = '"deca_humidity":{0},"deca_kelvin":{1}'.format(int(sensor.data.humidity * 10), int(sensor.data.temperature * 10 + 2731.5))
        if sensor.data.heat_stable:
            output = '{0},"gas":{1}'.format(output, sensor.data.gas_resistance)
        with tempfile.NamedTemporaryFile(delete=False) as tmp_file:
            temp_filename = tmp_file.name
            tmp_file.write(output)
        shutil.move(temp_filename, '/tmp/bme680')
    time.sleep(1)
)";

};  // namespace

bme680::bme680() {
	{
		std::ofstream ostr("/tmp/bme680_fecher.py");
		ostr << sorry << std::endl;
	}

	pid = fork();
	if (pid == -1) {
		throw std::runtime_error("Failed to fork");
	}

	if (pid == 0) {
		execl("/usr/bin/python2", "/usr/bin/python2", "/tmp/bme680_fecher.py", (char *)NULL);
		std::abort();
	}
}

bme680::~bme680() {
	kill(pid, SIGTERM);
	int status;
	waitpid(pid, &status, 0);
}

void bme680::print_data() {
	auto data = this->get_data();

	fmt::print("{}", data.data);
}

bme680::data bme680::get_data() {
	std::ifstream stream{"/tmp/bme680"};
	bme680::data data;
	std::getline(stream, data.data);
	return data;
}