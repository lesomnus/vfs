#include "vfs/impl/file.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "vfs/impl/utils.hpp"

namespace fs = std::filesystem;

namespace vfs {
namespace impl {

RegularFile& RegularFile::operator=(RegularFile const& other) {
	*this->open_write() << other.open_read()->rdbuf();
	this->owner(other.owner());
	this->group(other.group());
	this->perms(other.perms(), fs::perm_options::replace);

	return *this;
}

TempFile::TempFile() {
	auto const d = TempFile::temp_directory();
	fs::create_directories(d);

	this->path_ = d / random_string(32, Alphanumeric);
	do {
		auto* f = std::fopen(this->path_.c_str(), "wx");
		if(f != nullptr) {
			std::fclose(f);
			break;
		}

		this->path_.replace_filename(random_string(32, Alphanumeric));
	} while(true);
}

TempFile::~TempFile() {
	if(this->path_.empty()) {
		return;
	}

	fs::remove(this->path_);
}

std::shared_ptr<std::istream> TempFile::open_read(std::ios_base::openmode mode) const {
	return std::make_shared<std::ifstream>(this->path_, mode | std::ios_base::in);
}

std::shared_ptr<std::ostream> TempFile::open_write(std::ios_base::openmode mode) {
	return std::make_shared<std::ofstream>(this->path_, mode | std::ios_base::out);
}

}  // namespace impl
}  // namespace vfs
