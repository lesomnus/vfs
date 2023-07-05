#include "vfs/impl/utils.hpp"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <numeric>
#include <random>
#include <string>
#include <string_view>

namespace vfs {
namespace impl {

namespace {

std::mt19937& random_engine_() {
	static std::mt19937 engine(std::random_device{}());
	return engine;
}

}  // namespace

std::string random_string(std::size_t len, std::string_view char_set) {
	std::uniform_int_distribution<std::size_t> distribution{0, char_set.size() - 1};

	std::string rst(len, char_set.at(0));
	for(auto& c: rst) {
		c = char_set[distribution(random_engine_())];
	}

	return rst;
}

std::filesystem::path acc_paths(std::filesystem::path::const_iterator first, std::filesystem::path::const_iterator last) {
	return std::accumulate(first, last, std::filesystem::path{}, std::divides{});
}

std::string to_string(std::filesystem::file_type t) {
	using type = std::filesystem::file_type;

	switch(t) {
	case type::regular: {
		return "regular";
	}
	case type::directory: {
		return "directory";
	}
	case type::symlink: {
		return "symlink";
	}

	default: {
		return "unknown";
	}
	}
}

}  // namespace impl
}  // namespace vfs
