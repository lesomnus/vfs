#include "vfs/fs.hpp"

#include <filesystem>
#include <memory>

#include "vfs/impl/sys_fs.hpp"

namespace vfs {

std::shared_ptr<Fs> make_sys_fs() {
	return std::make_shared<impl::SysFs>(std::filesystem::current_path());
}

}  // namespace vfs
