#include "vfs/impl/file.hpp"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <utility>

#include "vfs/impl/utils.hpp"

namespace fs = std::filesystem;

namespace vfs {
namespace impl {

std::uintmax_t Directory::count() const {
	std::uintmax_t cnt = 1;
	for(auto const& [_, f]: this->files) {
		cnt += f->count();
	}

	return cnt;
}

std::shared_ptr<std::istream> EmptyFile::open_read(std::ios_base::openmode mode) const {
	return std::make_shared<std::ifstream>();
}

std::shared_ptr<std::ostream> EmptyFile::open_write(std::ios_base::openmode mode) {
	return std::make_shared<std::ofstream>();
}

TempFile::TempFile(std::intmax_t owner, std::intmax_t group, fs::perms perms)
    : RegularFile(owner, group, perms) {
	auto const d = TempFile::temp_directory();
	fs::create_directories(d);

	this->sys_path_ = d / random_string(32, Alphanumeric);
	do {
		auto* f = std::fopen(this->sys_path_.c_str(), "wx");
		if(f != nullptr) {
			std::fclose(f);
			break;
		}

		this->sys_path_.replace_filename(random_string(32, Alphanumeric));
	} while(true);
}

TempFile::TempFile(TempFile&& other)
    : RegularFile(std::move(other))
    , sys_path_(std::move(other.sys_path_)) { }

TempFile::~TempFile() {
	if(this->sys_path_.empty()) {
		return;
	}

	fs::remove(this->sys_path_);
}

std::shared_ptr<std::istream> TempFile::open_read(std::ios_base::openmode mode) const {
	return std::make_shared<std::ifstream>(this->sys_path_, mode | std::ios_base::in);
}

std::shared_ptr<std::ostream> TempFile::open_write(std::ios_base::openmode mode) {
	return std::make_shared<std::ofstream>(this->sys_path_, mode | std::ios_base::out);
}

}  // namespace impl
}  // namespace vfs
