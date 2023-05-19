#include "vfs/fs.hpp"

#include <filesystem>
#include <memory>

#include "vfs/impl/os_fs.hpp"

namespace vfs {

std::shared_ptr<Fs> make_os_fs() {
	return std::make_shared<impl::SysFs>(std::filesystem::current_path());
}

}  // namespace vfs
