#include "vfs/impl/os_fs.hpp"
#include "vfs/fs.hpp"

#include <algorithm>
#include <filesystem>
#include <memory>

#include "vfs/impl/utils.hpp"
#include "vfs/impl/vfs.hpp"

namespace vfs {

namespace impl {

namespace {

class Cursor_ {
};

}  // namespace

std::shared_ptr<Fs const> StdFs::change_root(std::filesystem::path const& p, std::filesystem::path const& temp_dir) const {
	return std::make_shared<ChRootedStdFs>(StdFs::canonical(p), "/", temp_dir);
}

std::filesystem::path ChRootedStdFs::normal_(std::filesystem::path const& p) const {
	auto const a = this->base_ / (this->cwd_ / p).relative_path();
	auto const c = std::filesystem::weakly_canonical(a);
	auto const r = c.lexically_relative(this->base_);
	if(r.empty() || (*r.begin() == ".")) {
		return this->base_;
	}
	if(*r.begin() == "..") {
		auto it = std::find_if(r.begin(), r.end(), [](auto entry) { return entry != ".."; });
		if(it == r.end()) {
			return this->base_;
		} else {
			return this->base_ / acc_paths(it, r.end());
		}
	}

	return a;
}

}  // namespace impl

std::shared_ptr<Fs> make_os_fs() {
	auto std_fs = std::make_shared<impl::StdFs>(std::filesystem::current_path());
	return std::make_shared<impl::OsFsProxy>(std::move(std_fs));
}

}  // namespace vfs
