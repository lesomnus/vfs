#include <iostream>
#include <memory>
#include <string>

#include <vfs/fs.hpp>
#include <vfs/impl/utils.hpp>

#include "testing.hpp"

namespace testing {

std::shared_ptr<vfs::Fs> cd_temp_dir(vfs::Fs& fs) {
	auto const p = fs.temp_directory_path() / "vfs-test" / vfs::impl::random_string(8, vfs::impl::Alphanumeric);
	fs.create_directories(p);

	auto rst = fs.current_path(p);
	return rst;
}

std::string read_all(std::istream& in) {
	return std::string(std::istreambuf_iterator(in), {});
}

}  // namespace testing
