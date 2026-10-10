
/// Runs every unit test. The tests register themselves (cairo_test_new), so
/// main only hands the command line to cairo, which also lets one choose
/// which tests to run.

#include "cairo.h"

int main(const int count, const char* arguments[]) {
	return cairo_tests_run(cairo_args_new(count, arguments));
}
