#include "vfs/fs.hpp"

#include <filesystem>
#include <memory>

#include "vfs/directory_iterator.hpp"

#include "vfs/impl/utils.hpp"

namespace vfs {

directory_iterator Fs::iterate_directory(std::filesystem::path const& p) const {
	return directory_iterator(*this, p);
}

directory_iterator Fs::iterate_directory() const {
	return iterate_directory(this->current_path());
}

std::shared_ptr<Fs::Cursor> Fs::iterate_directory_(std::filesystem::path const& p, std::filesystem::directory_options opts, std::error_code& ec) const {
	return impl::handle_error([&] { return this->iterate_directory_(p, opts); }, ec);
}

}  // namespace vfs
