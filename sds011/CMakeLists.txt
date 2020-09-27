cmake_minimum_required (VERSION 3.5)

project(sds011 CXX)

add_compile_options(-Wall -Wextra -Werror -std=c++1z)

add_executable(sds011 main.cpp sds011.cpp)

find_package(fmt)

target_link_libraries(sds011 fmt::fmt)

file(GLOB ALL_SOURCE_FILES *.cpp *.hpp)

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