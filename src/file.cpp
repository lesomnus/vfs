#include "vfs/impl/file.hpp"

#include <filesystem>
#include <memory>
#include <utility>

namespace fs = std::filesystem;

namespace vfs {
namespace impl {

void RegularFile::copy_content_to(RegularFile& other) const {
	*other.open_write() << this->open_read()->rdbuf();
}

RegularFile& RegularFile::operator=(RegularFile const& other) {
	other.copy_content_to(*this);
	this->owner(other.owner());
	this->group(other.group());
	this->perms(other.perms(), fs::perm_options::replace);

	return *this;
}

Directory::Iterator::Iterator(std::shared_ptr<Cursor> cursor)
    : cursor_(std::move(cursor)) {
	if(this->cursor_->at_end()) {
		return;
	}

	this->value_ = std::make_pair(this->cursor_->name(), this->cursor_->file());
}

Directory::Iterator& Directory::Iterator::operator++() {
	if(!this->cursor_) {
		return *this;
	}

	this->cursor_->increment();
	if(this->cursor_->at_end()) {
		this->cursor_.reset();
	} else {
		this->value_ = std::make_pair(this->cursor_->name(), this->cursor_->file());
	}

	return *this;
}

}  // namespace impl
}  // namespace vfs
