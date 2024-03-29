#pragma once

#include <concepts>
#include <cstddef>
#include <filesystem>
#include <string_view>
#include <system_error>

namespace vfs {
namespace impl {

constexpr std::string_view Alphanumeric = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

[[nodiscard]] std::string random_string(std::size_t len, std::string_view char_set);

[[nodiscard]] std::filesystem::path acc_paths(std::filesystem::path::const_iterator first, std::filesystem::path::const_iterator last);

template<std::invocable<> F, typename R = std::invoke_result_t<F>>
auto handle_error(F const& f, std::error_code& ec, R v = R{}) noexcept -> R {
	try {
		auto rst = f();
		ec.clear();
		return rst;
	} catch(std::filesystem::filesystem_error const& err) {
		ec = err.code();
		return v;
	}
}

[[nodiscard]] std::string to_string(std::filesystem::file_type t);

// Use to avoid LWG 3657.
struct PathHash {
	std::size_t operator()(std::filesystem::path const& path) const {
		return std::filesystem::hash_value(path);
	}
};

}  // namespace impl
}  // namespace vfs
