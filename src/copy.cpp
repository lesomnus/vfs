#include "vfs/fs.hpp"
#include "vfs/impl/os_fs.hpp"
#include "vfs/impl/vfs.hpp"

#include <cassert>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>

#include "vfs/impl/entry.hpp"
#include "vfs/impl/fs_proxy.hpp"
#include "vfs/impl/utils.hpp"

namespace fs = std::filesystem;

namespace vfs {

namespace impl {

namespace {

fs::filesystem_error err_create_symlink_to_diff_fs_() {
	return fs::filesystem_error("cannot create a symlink to different filesystem", std::make_error_code(std::errc::invalid_argument));
}

fs::filesystem_error err_create_hard_link_to_diff_fs_() {
	return fs::filesystem_error("cannot create a hard link to different filesystem", std::make_error_code(std::errc::invalid_argument));
}

bool copy_file_into_(std::shared_ptr<RegularFile const> src_r, fs::path const& src_p, Directory& dst_prev, fs::path const& dst_p, fs::copy_options opts) {
	auto const [dst_r, ok] = dst_prev.emplace_regular_file(dst_p.filename());
	auto const commit      = [&]() -> bool {
        src_r->copy_content_to(*dst_r);
        return true;
	};

	if(ok) {
		return commit();
	}

	if(!dst_r) {
		throw fs::filesystem_error("destination not a regular file", src_p, dst_p, std::make_error_code(std::errc::invalid_argument));
	}
	if(*src_r == *dst_r) {
		throw fs::filesystem_error("source and destination are same", src_p, dst_p, std::make_error_code(std::errc::file_exists));
	}
	if((opts & fs::copy_options::skip_existing) == fs::copy_options::skip_existing) {
		return false;
	}
	if((opts & fs::copy_options::overwrite_existing) == fs::copy_options::overwrite_existing) {
		return commit();
	}
	if((opts & fs::copy_options::update_existing) == fs::copy_options::update_existing) {
		if(src_r->last_write_time() < dst_r->last_write_time()) {
			return false;
		}

		return commit();
	}

	throw fs::filesystem_error("", src_p, dst_p, std::make_error_code(std::errc::file_exists));
}

void copy_into_(std::shared_ptr<File const> src, fs::path const& src_p, Directory& dst_prev, fs::path const& dst_p, fs::copy_options opts) {
	if(auto src_r = std::dynamic_pointer_cast<RegularFile const>(std::move(src)); src_r) {
		if((opts & fs::copy_options::directories_only) == fs::copy_options::directories_only) {
			return;
		}
		if((opts & fs::copy_options::create_symlinks) == fs::copy_options::create_symlinks) {
			auto const [_, ok] = dst_prev.emplace_symlink(dst_p.filename(), src_p);
			if(!ok) {
				throw fs::filesystem_error("", dst_p, std::make_error_code(std::errc::file_exists));
			}
			return;
		}
		if((opts & fs::copy_options::create_hard_links) == fs::copy_options::create_hard_links) {
			dst_prev.link(dst_p.filename(), std::const_pointer_cast<RegularFile>(std::move(src_r)));
			return;
		}

		auto next_f = dst_prev.next(dst_p.filename());
		if(auto next_d = std::dynamic_pointer_cast<Directory>(std::move(next_f)); next_d) {
			copy_file_into_(src_r, src_p, *next_d, dst_p / src_p.filename(), opts);
		} else {
			copy_file_into_(src_r, src_p, dst_prev, dst_p, opts);
		}

		return;
	}

	if(auto src_s = std::dynamic_pointer_cast<Symlink const>(std::move(src)); src_s) {
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

		return;
	}

	auto src_d = std::dynamic_pointer_cast<Directory const>(std::move(src));
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
		throw fs::filesystem_error("", dst_p, std::make_error_code(std::errc::is_a_directory));
	}

	auto const cursor = src_d->cursor();
	for(; not cursor->at_end(); cursor->increment()) {
		auto const type = cursor->file()->type();
		switch(type) {
		case fs::file_type::symlink: {
			if((opts & fs::copy_options::copy_symlinks) != fs::copy_options::copy_symlinks) {
				continue;
			}
			break;
		}
		case fs::file_type::directory: {
			if((opts & fs::copy_options::recursive) != fs::copy_options::recursive) {
				continue;
			}
			break;
		}
		};

		copy_into_(cursor->file(), src_p / cursor->name(), *dst_d, dst_p / cursor->name(), opts);
	}
}

void copy_into_(FsBase const& self, fs::path const& src, FsBase& other, fs::path const& dst, fs::copy_options opts) {
	auto const src_p = self.canonical(src);
	auto const src_f = self.file_at(src_p);
	auto const dst_p = other.weakly_canonical(dst);

	// TODO: if both src_f and dst_f are OsFile, use std::copy.
	auto dst_prev = std::dynamic_pointer_cast<Directory>(other.file_at(dst_p.parent_path()));
	if(!dst_prev) {
		throw fs::filesystem_error("", dst_p.parent_path(), std::make_error_code(std::errc::not_a_directory));
	}

	copy_into_(src_f, src_p, *dst_prev, dst_p, opts);
}

}  // namespace

void StdFs::copy_(fs::path const& src, Fs& other, fs::path const& dst, fs::copy_options opts) const {
	if(auto* os_fs = fs_cast<OsFs>(&other); os_fs) {
		auto const src_p = this->os_path_of(src);
		auto const dst_p = os_fs->os_path_of(dst);
		fs::copy(src_p, dst_p, opts);
		return;
	}

	if((opts & fs::copy_options::create_symlinks) == fs::copy_options::create_symlinks) {
		throw err_create_symlink_to_diff_fs_();
	}
	if((opts & fs::copy_options::create_hard_links) == fs::copy_options::create_hard_links) {
		throw err_create_hard_link_to_diff_fs_();
	}

	copy_into_(*this, src, fs_base(other), dst, opts);
}

void Vfs::copy(fs::path const& src, fs::path const& dst, fs::copy_options opts) {
	copy_into_(*this, src, *this, dst, opts);
}

void Vfs::copy_(fs::path const& src, Fs& other, fs::path const& dst, fs::copy_options opts) const {
	if((opts & fs::copy_options::create_symlinks) == fs::copy_options::create_symlinks) {
		throw err_create_symlink_to_diff_fs_();
	}
	if(fs_cast<OsFs>(&other) && ((opts & fs::copy_options::create_hard_links) == fs::copy_options::create_hard_links)) {
		throw err_create_hard_link_to_diff_fs_();
	}

	copy_into_(*this, src, fs_base(other), dst, opts);
}

}  // namespace impl

void Fs::copy(fs::path const& src, Fs& other, fs::path const& dst, fs::copy_options opts) const {
	if(this == &other) {
		other.copy(src, dst, opts);
		return;
	}

	this->copy_(src, other, dst, opts);
}

void Fs::copy(fs::path const& src, Fs& other, fs::path const& dst, fs::copy_options opts, std::error_code& ec) {
	impl::handle_error([&] { this->copy(src, other, dst, opts); return 0; }, ec);
}

}  // namespace vfs
