#include <jgrandson.h>
#include <iostream>

static void parse_json() {
	jg::Root root;
	root.parse_file("foo.json");
	auto obj = root.get_obj();
	auto arr = obj["strings?"].get_arr(
		2,
		100,
		"Foo requires an array of at least 1 string and 1 bool",
		"Foo can't handle more than 100 values"
	);
	std::vector<std::string> strings;
	std::vector<bool> bools;
	for (auto const & elem : arr) {
		switch (elem.get_json_type()) {
		case JG_TYPE_BOOL: {
			auto b = elem.get_bool();
			std::cout << (b ? "true" : "false") << "\n";
			bools.push_back(b);
			continue;
		} case JG_TYPE_STR: {
			auto str = elem.get_str();
			std::cout << str << "\n";
			strings.push_back(str);
			continue;
		} default:
			throw jg::Err("The \"strings?\" arr is only expected to consist of "
				"strings and/or booleans. (Don't ask me why!)");
		}
	}
	auto child_obj = obj["I am a 🔑"].get_obj_defa(
		4,
		"No keys are recognized other than \"id\", \"размер\", "
			"\"short_flo\", and \"long_flo\"."
	);
	auto id = child_obj["id"].get_uint64_defa(42, 1, UINT64_MAX,
		"0 is a reserved value.");
	auto dimension = child_obj["размер"].get_sizet_defa(24);
	auto short_flo = child_obj["short_flo"].get_float_defa(0.12345f);
	auto long_flo = child_obj["long_flo"].get_long_double_defa(-1.2345);
	std::cout << "id: " << id << ", dimension: " << dimension <<
		", short_flo: " << short_flo << ", long_flo: " << long_flo << "\n";
}

int main() {
#if defined(_WIN32) || defined(_WIN64)
    if (!::SetConsoleOutputCP(CP_UTF8)) {
        std::cout << "Failed to ::SetConsoleOutputCP(CP_UTF8).\n";
        return EXIT_FAILURE;
    }
#endif
	try {
		parse_json();
	} catch (jg::Err const & e) {
		std::cerr << e.what() << "\n";
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}