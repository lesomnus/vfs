#include <filesystem>
#include <memory>
#include <utility>

#include "vfs/impl/mem_file.hpp"
#include "vfs/impl/vfs.hpp"

namespace fs = std::filesystem;

namespace vfs {

std::shared_ptr<Fs> make_mem_fs(fs::path const& temp_dir) {
	auto d = std::make_shared<impl::DirectoryEntry>("/", nullptr, std::make_shared<impl::MemDirectory>());
	return std::make_shared<impl::Vfs>(std::move(d), temp_dir);
}

}  // namespace vfs
