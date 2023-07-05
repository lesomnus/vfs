#include "vfs/fs.hpp"
#include "vfs/impl/fs.hpp"

#include <filesystem>
#include <memory>

#include "vfs/directory_iterator.hpp"

#include "vfs/impl/utils.hpp"

namespace fs = std::filesystem;

namespace vfs {

directory_iterator& directory_iterator::increment(std::error_code& ec) {
	impl::handle_error([&] { this->operator++(); return 0; }, ec);
	return *this;
}

recursive_directory_iterator& recursive_directory_iterator::increment(std::error_code& ec) {
	impl::handle_error([&] { this->operator++(); return 0; }, ec);
	return *this;
}

directory_iterator Fs::iterate_directory(fs::path const& p, fs::directory_options opts) const {
	return {*this, p, opts};
}

directory_iterator Fs::iterate_directory(fs::path const& p, fs::directory_options opts, std::error_code& ec) const {
	return {*this, p, opts, ec};
}

directory_iterator Fs::iterate_directory(fs::path const& p, std::error_code& ec) const {
	return {*this, p, fs::directory_options::none, ec};
}

recursive_directory_iterator Fs::iterate_directory_recursively(fs::path const& p, fs::directory_options opts) const {
	return {*this, p, opts};
}

recursive_directory_iterator Fs::iterate_directory_recursively(fs::path const& p, fs::directory_options opts, std::error_code& ec) const {
	return {*this, p, opts, ec};
}

recursive_directory_iterator Fs::iterate_directory_recursively(fs::path const& p, std::error_code& ec) const {
	return {*this, p, fs::directory_options::none, ec};
}

std::shared_ptr<Fs::Cursor> Fs::cursor_(fs::path const& p, fs::directory_options opts, std::error_code& ec) const {
	return impl::handle_error([&] { return this->cursor_(p, opts); }, ec);
}

std::shared_ptr<Fs::RecursiveCursor> Fs::recursive_cursor_(fs::path const& p, fs::directory_options opts, std::error_code& ec) const {
	return impl::handle_error([&] { return this->recursive_cursor_(p, opts); }, ec);
}

void Fs::Cursor::increment(std::error_code& ec) {
	impl::handle_error([&] { this->increment(); return 0; }, ec);
}

void Fs::RecursiveCursor::pop(std::error_code& ec) {
	impl::handle_error([&] { this->pop(); return 0; }, ec);
}

}  // namespace vfs
