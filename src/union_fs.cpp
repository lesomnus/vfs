#include <filesystem>
#include <memory>
#include <utility>

#include "vfs/impl/union_file.hpp"
#include "vfs/impl/vfs.hpp"

namespace fs = std::filesystem;

namespace vfs {

std::shared_ptr<Fs> make_union_fs(Fs& upper, Fs const& lower) {
	auto up_d = impl::fs_base(upper).cwd();
	auto lo_d = impl::fs_base(lower).cwd();

	auto d = std::make_shared<impl::DirectoryEntry>("/", nullptr, std::make_shared<impl::UnionDirectory>(std::move(up_d), std::move(lo_d)));
	return std::make_shared<impl::Vfs>(std::move(d), upper.temp_directory_path());
}

}  // namespace vfs
