#include "vfs/impl/os_file.hpp"
#include "vfs/impl/os_fs.hpp"
#include "vfs/impl/vfile.hpp"
#include "vfs/impl/vfs.hpp"

#include <cassert>
#include <filesystem>
#include <memory>
#include <utility>

#include "vfs/impl/entry.hpp"
#include "vfs/impl/fs_proxy.hpp"

namespace fs = std::filesystem;

namespace vfs {
namespace impl {

namespace {

// `original` possibly `nullptr`.
std::shared_ptr<MountPoint> make_mount_point_(std::shared_ptr<File> attachment, std::shared_ptr<File> original) {
	assert(fs::file_type::symlink != attachment->type());

	if(original != nullptr && original->type() == fs::file_type::symlink) {
		throw std::logic_error("symlink cannot be a mount point");
	}

	switch(attachment->type()) {
	case fs::file_type::regular: {
		return std::make_shared<MountedRegularFile>(
		    std::dynamic_pointer_cast<RegularFile>(std::move(attachment)),
		    std::dynamic_pointer_cast<RegularFile>(std::move(original)));
	}
	case fs::file_type::directory: {
		return std::make_shared<MountedDirectory>(
		    std::dynamic_pointer_cast<Directory>(std::move(attachment)),
		    std::dynamic_pointer_cast<Directory>(std::move(original)));
	}

	default:
		throw std::runtime_error("mount for given type is not implemented");
	}

	// unreachable.
}

fs::filesystem_error err_mount_point_does_not_exist(fs::path const& p) {
	return fs::filesystem_error("mount point does not exist", p, std::make_error_code(std::errc::no_such_file_or_directory));
}

fs::filesystem_error err_not_a_mount_point(fs::path const& p) {
	return fs::filesystem_error("not a mount point", p, std::make_error_code(std::errc::invalid_argument));
}

void test_mount_point_(fs::path const& p, fs::file_type mount_point_type, fs::file_type source_type) {
	if(mount_point_type == fs::file_type::not_found) {
		throw err_mount_point_does_not_exist(p);
	}
	if(mount_point_type != source_type) {
		if(mount_point_type == fs::file_type::directory) {
			throw fs::filesystem_error("", p, std::make_error_code(std::errc::not_a_directory));
		}
		if(source_type == fs::file_type::directory) {
			throw fs::filesystem_error("mount point is not a directory", p, std::make_error_code(std::errc::not_a_directory));
		}

		throw fs::filesystem_error("file type is different with the mount point", p, std::make_error_code(std::errc::invalid_argument));
	}
}

std::shared_ptr<Directory> get_cwd_(Fs& other) {
	if(auto* fs_proxy = dynamic_cast<FsProxy*>(&other); fs_proxy) {
		return get_cwd_(fs_proxy->underlying_fs());
	}
	if(auto* os_fs = dynamic_cast<OsFs*>(&other); os_fs) {
		auto cwd = os_fs->current_os_path();
		return std::make_shared<OsDirectory>(std::move(cwd));
	}
	if(auto* vfs = dynamic_cast<Vfs*>(&other); vfs) {
		return vfs->current_working_directory_entry()->typed_file();
	}

	throw std::logic_error("unknown filesystem");
}

}  // namespace

//
// Next file should not be a Symlink and it must be checked by caller who calls mount_next or unmount_next.
//

void OsDirectory::mount_next(std::string const& name, std::shared_ptr<File> file) {
	auto const next_p = this->path_ / name;
	auto const status = fs::status(next_p);
	test_mount_point_(next_p, status.type(), file->type());

	std::shared_ptr<MountPoint> mount_point;

	auto& mount_points = this->context_->mount_points;
	if(auto it = mount_points.find(next_p); it == mount_points.end()) {
		mount_point = make_mount_point_(file, nullptr);
	} else {
		mount_point = make_mount_point_(file, it->second);
	}
	mount_points.insert(std::make_pair(next_p, std::move(mount_point)));
}

void OsDirectory::unmount_next(std::string const& name) {
	auto const next_p = this->path_ / name;

	auto& mount_points = this->context_->mount_points;
	auto  it           = mount_points.find(next_p);
	if(it == mount_points.end()) {
		throw err_not_a_mount_point(next_p);
	}

	auto original = it->second->original();
	if(original == nullptr) {
		mount_points.erase(it);
	} else {
		auto mount_point = std::dynamic_pointer_cast<MountPoint>(std::move(original));

		// Only the first MountPoint holds nullptr as the original,
		// and starting from the second MountPoint, it always holds MountPoint as the original.
		assert(nullptr != mount_point);

		it->second = std::move(mount_point);
	}
}

void OsFs::unmount(fs::path const& target) {
	// OsFs cannot have a mount point since it is converted to Vfs when mounted.
	throw fs::filesystem_error("not a mount point", target, std::make_error_code(std::errc::invalid_argument));
}

std::shared_ptr<Vfs> StdFs::make_mount(fs::path const& target, Fs& other) {
	auto original_p = fs::canonical(this->normal_(target));
	if(fs::is_symlink(original_p)) {
		auto t = original_p / fs::read_symlink(original_p);
		return this->make_mount(t, other);
	}

	auto attachment = get_cwd_(other);
	auto parent     = std::make_shared<OsDirectory>(original_p.parent_path());
	parent->mount_next(original_p.filename(), std::move(attachment));

	auto root = std::make_shared<OsDirectory>(parent->context(), this->base_path());
	auto vfs  = std::make_shared<Vfs>(DirectoryEntry::make(this->base_path().filename(), nullptr, std::move(root)), this->temp_directory_os_path());

	vfs = std::static_pointer_cast<Vfs>(vfs->current_path(this->current_os_path()));
	return vfs;
}

void OsFsProxy::mount(fs::path const& target, Fs& other) {
	if(auto os_fs = std::dynamic_pointer_cast<OsFs>(this->fs_); os_fs) {
		this->fs_ = os_fs->make_mount(target, other);
	} else {
		this->fs_->mount(target, other);
	}
}

void VDirectory::mount_next(std::string const& name, std::shared_ptr<File> file) {
	auto const it = this->files_.find(name);
	if(it == this->files_.end()) {
		throw err_mount_point_does_not_exist("");
	}

	auto& next = it->second;
	test_mount_point_("", next->type(), file->type());

	next = make_mount_point_(file, std::move(next));
}

void VDirectory::unmount_next(std::string const& name) {
	auto const it = this->files_.find(name);
	if(it == this->files_.end()) {
		throw err_mount_point_does_not_exist("");
	}

	auto& next = it->second;

	auto mount_point = std::dynamic_pointer_cast<MountPoint>(next);
	if(mount_point == nullptr) {
		throw err_not_a_mount_point("");
	}

	next = mount_point->original();
	assert(next != nullptr);
}

void Vfs::mount(fs::path const& target, Fs& other) {
	auto original   = this->navigate(target)->follow_chain();
	auto attachment = get_cwd_(other);

	original->prev()->typed_file()->mount_next(original->name(), std::move(attachment));
}

void Vfs::unmount(fs::path const& target) {
	auto d = this->navigate(target)->follow_chain();

	auto mount_point = std::reinterpret_pointer_cast<MountPoint>(d->file());
	if(not mount_point) {
		throw err_not_a_mount_point(d->path());
	}

	d->prev()->typed_file()->unmount_next(d->name());
}

}  // namespace impl
}  // namespace vfs
