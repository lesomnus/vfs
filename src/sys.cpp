#include "vfs/sys.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <system_error>

namespace vfs {

std::shared_ptr<std::iostream> SysFs::open(char const* filename, std::ios_base::openmode mode) {
	return std::make_shared<std::fstream>(filename, mode);
}

bool SysFs::copy_file(std::filesystem::path const& from, std::filesystem::path const& to, std::filesystem::copy_options options) {
	return std::filesystem::copy_file(from, to, options);
}

bool SysFs::copy_file(std::filesystem::path const& from, std::filesystem::path const& to, std::filesystem::copy_options options, std::error_code& ec) {
	return std::filesystem::copy_file(from, to, options, ec);
}

bool exists(std::filesystem::file_status s) noexcept {
	return std::filesystem::exists(s);
}

bool exists(std::filesystem::path const& p) {
	return std::filesystem::exists(p);
}

bool exists(std::filesystem::path const& p, std::error_code& ec) noexcept {
	return std::filesystem::exists(p, ec);
}

}  // namespace vfs
