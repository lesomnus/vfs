#include "vfs/fs.hpp"
#include "vfs/impl/os_fs.hpp"
#include "vfs/impl/vfs.hpp"

#include <cassert>
#include <filesystem>
#include <memory>
#include <string>

#include "vfs/impl/entry.hpp"
#include "vfs/impl/fs_proxy.hpp"

namespace fs = std::filesystem;

namespace vfs {

namespace impl {

namespace {

void copy_into_(File const& src, fs::path const& src_p, Directory& dst_prev, fs::path const& dst_p, fs::copy_options opts) {
	if(auto const* src_r = dynamic_cast<RegularFile const*>(&src); src_r) {
		if((opts & fs::copy_options::directories_only) == fs::copy_options::directories_only) {
			return;
		}
		// if((opts & fs::copy_options::create_symlinks) == fs::copy_options::create_symlinks) {
		// 	this->create_symlink(src, dst);
		// 	return;
		// }
		// if((opts & fs::copy_options::create_hard_links) == fs::copy_options::create_hard_links) {
		// 	throw std::runtime_error("not implemented");
		// }

		std::shared_ptr<RegularFile> next_r;
		if(auto next_f = dst_prev.next(dst_p.filename()); !next_f) {
			auto [r, ok] = dst_prev.emplace_regular_file(dst_p.filename());
			assert(ok);
			next_r = std::move(r);
		} else if(auto r = std::dynamic_pointer_cast<RegularFile>(std::move(next_f)); r) {
			// TODO: implement copy_file
			next_r = std::move(r);
		} else if(auto d = std::dynamic_pointer_cast<Directory>(std::move(next_f)); d) {
			auto [r, ok] = d->emplace_regular_file(src_p.filename());
			if(!r) {
				throw fs::filesystem_error("", dst_p / src_p.filename(), std::make_error_code(std::errc::file_exists));
			}

			next_r = std::move(r);
		} else {
			throw fs::filesystem_error("", dst_p, std::make_error_code(std::errc::file_exists));
		}

		src_r->copy_content_to(*next_r);
		return;
	}

	if(auto const* src_s = dynamic_cast<Symlink const*>(&src); src_s) {
		if((opts & fs::copy_options::skip_symlinks) == fs::copy_options::skip_symlinks) {
			return;
		}
		if((opts & fs::copy_options::copy_symlinks) != fs::copy_options::copy_symlinks) {
			throw fs::filesystem_error("cannot copy symlink", src_p, dst_p, std::make_error_code(std::errc::invalid_argument));
		}

		auto const [s, ok] = dst_prev.emplace_symlink(dst_p.filename(), src_s->target());
		if(!s || !ok) {
			throw fs::filesystem_error("cannot create symlink", dst_p, std::make_error_code(std::errc::file_exists));
		}
	}

	auto const* src_d = dynamic_cast<Directory const*>(&src);
	if(!src_d) {
		throw fs::filesystem_error("source is not a not a regular file, a directory, or a symlink", src_p, std::make_error_code(std::errc::invalid_argument));
	}
	if((opts & fs::copy_options::create_symlinks) == fs::copy_options::create_symlinks) {
		throw fs::filesystem_error("", src_p, std::make_error_code(std::errc::is_a_directory));
	}
	if(!(((opts & fs::copy_options::recursive) == fs::copy_options::recursive) || (opts == fs::copy_options::none))) {
		return;
	}

	auto [dst_d, ok] = dst_prev.emplace_directory(dst_p.filename());
	if(!dst_d) {
		throw fs::filesystem_error("", dst_p, std::make_error_code(std::errc::file_exists));
	}

	auto const cursor = src_d->cursor();
	while(not cursor->at_end()) {
		copy_into_(*cursor->file(), src_p / cursor->name(), *dst_d, dst_p / cursor->name(), opts);
	}
}

}  // namespace

void StdFs::copy_(fs::path const& src, Fs& other, fs::path const& dst, fs::copy_options opts) const {
	if(auto* os_fs = fs_cast<OsFs>(&other); os_fs) {
		auto const src_p = this->os_path_of(src);
		auto const dst_p = os_fs->os_path_of(dst);
		fs::copy(src_p, dst_p, opts);
		return;
	}

	auto const src_p = this->canonical(src);
	auto const src_f = this->file_at(src_p);
	auto const dst_p = other.weakly_canonical(dst);

	auto dst_prev = std::dynamic_pointer_cast<Directory>(fs_base(other).file_at(dst_p.parent_path()));
	if(!dst_prev) {
		throw fs::filesystem_error("", dst_p.parent_path(), std::make_error_code(std::errc::not_a_directory));
	}

	copy_into_(*src_f, src_p, *dst_prev, dst_p, opts);
}

void Vfs::copy_(fs::path const& src, Fs& other, fs::path const& dst, fs::copy_options opts) const {
}

}  // namespace impl

void Fs::copy(fs::path const& src, Fs& other, fs::path const& dst, fs::copy_options opts) const {
	if(this == &other) {
		other.copy(src, dst, opts);
		return;
	}

	this->copy_(src, other, dst, opts);
}

}  // namespace vfs
