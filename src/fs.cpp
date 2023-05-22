#include "vfs/fs.hpp"

#include <filesystem>
#include <memory>

#include "vfs/directory_iterator.hpp"

#include "vfs/impl/utils.hpp"

namespace vfs {

directory_iterator Fs::iterate_directory(std::filesystem::path const& p, std::filesystem::directory_options opts) const {
	return directory_iterator(*this, p, opts);
}

directory_iterator Fs::iterate_directory(std::filesystem::path const& p, std::filesystem::directory_options opts, std::error_code& ec) const {
	return directory_iterator(*this, p, opts, ec);
}

directory_iterator Fs::iterate_directory(std::filesystem::path const& p, std::error_code& ec) const {
	return directory_iterator(*this, p, std::filesystem::directory_options::none, ec);
}

recursive_directory_iterator Fs::iterate_directory_recursively(std::filesystem::path const& p, std::filesystem::directory_options opts) const {
	return recursive_directory_iterator(*this, p, opts);
}

recursive_directory_iterator Fs::iterate_directory_recursively(std::filesystem::path const& p, std::filesystem::directory_options opts, std::error_code& ec) const {
	return recursive_directory_iterator(*this, p, opts, ec);
}

recursive_directory_iterator Fs::iterate_directory_recursively(std::filesystem::path const& p, std::error_code& ec) const {
	return recursive_directory_iterator(*this, p, std::filesystem::directory_options::none, ec);
}

std::shared_ptr<Fs::Cursor> Fs::cursor_(std::filesystem::path const& p, std::filesystem::directory_options opts, std::error_code& ec) const {
	return impl::handle_error([&] { return this->cursor_(p, opts); }, ec);
}

std::shared_ptr<Fs::RecursiveCursor> Fs::recursive_cursor_(std::filesystem::path const& p, std::filesystem::directory_options opts, std::error_code& ec) const {
	return impl::handle_error([&] { return this->recursive_cursor_(p, opts); }, ec);
}

void Fs::Cursor::increment(std::error_code& ec) {
	impl::handle_error([&] { this->increment(); return 0; }, ec);
}

void Fs::RecursiveCursor::pop(std::error_code& ec) {
	impl::handle_error([&] { this->pop(); return 0; }, ec);
}

}  // namespace vfs
