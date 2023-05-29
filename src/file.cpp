#include "vfs/impl/file.hpp"

#include <filesystem>
#include <fstream>

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

}  // namespace impl
}  // namespace vfs
