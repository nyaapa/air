cmake_minimum_required (VERSION 3.5)

project(air C CXX)

add_compile_options(-Wall -Wextra -Werror)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z -fconcepts")

add_executable(air main.cpp sds011/sds011.cpp s8/s8.cpp bme280/bme280.cpp bme680/bme680.cpp udp/udpclient.cpp)

find_package(fmt)

find_library(wiringPi_LIB wiringPi)

target_link_libraries(air fmt::fmt ${wiringPi_LIB})

file(GLOB ALL_SOURCE_FILES *.cpp *.hpp s8/*.cpp s8/*.hpp sds011/*.cpp sds011/*.hpp bme280/*.cpp bme280/*.hpp udp/*.cpp udp/*.hpp bme680/*.cpp bme680/*.hpp bme680/*.c bme680/*.h)

add_custom_target(
	format
	COMMAND /usr/bin/clang-format
	-i
	-style=file
	-assume-filename=.clang-format
	${ALL_SOURCE_FILES}
)

set(TIDY_CHECKS 
	*
	-cppcoreguidelines-pro-type-reinterpret-cast
	-cppcoreguidelines-pro-bounds-array-to-pointer-decay
	-fuchsia-default-arguments
	-readability-implicit-bool-conversion
	-llvm-header-guard
	-hicpp-no-array-decay
	-misc-definitions-in-headers
	-cppcoreguidelines-pro-type-vararg
	-hicpp-vararg
	-fuchsia-overloaded-operator
	-llvm-include-order
)
string(REPLACE ";" "," TIDY_CHECKS_STR "${TIDY_CHECKS}")

add_custom_target(
	tidy
	COMMAND /usr/bin/clang-tidy
	-checks=${TIDY_CHECKS_STR}
	-p ${PROJECT_SOURCE_DIR}
	${ALL_SOURCE_FILES}
	--
	-Ilib
	--std=c++2a
)
