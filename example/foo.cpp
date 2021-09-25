#include <jgrandson.h>
#include <iostream>

static void parse_json() {
	jg::Root root;
	root.parse_file("foo.json");
	auto arr = root.get_arr();
}

int main() {
	try {
		parse_json();
	} catch (jg::err const & e) {
		std::cerr << e.what() << "\n";
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}