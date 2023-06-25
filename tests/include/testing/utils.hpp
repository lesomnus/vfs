#include <memory>

#include <vfs/fs.hpp>

namespace testing {

std::shared_ptr<vfs::Fs> cd_temp_dir(vfs::Fs& fs);

}  // namespace testing
