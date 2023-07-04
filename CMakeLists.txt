cmake_minimum_required(VERSION 3.21)

project(
	"vfs"
		VERSION   0.0.1
		LANGUAGES CXX
)

option(${PROJECT_NAME}_TESTS "Enable ${PROJECT_NAME} tests targets." ${PROJECT_IS_TOP_LEVEL})
option(${PROJECT_NAME}_TIDY  "Run clang-tidy for ${PROJECT_NAME}."   ${PROJECT_IS_TOP_LEVEL})

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)



add_library(
	vfs SHARED
		include/vfs/directory_entry.hpp
		include/vfs/directory_iterator.hpp
		include/vfs/fs.hpp
		include/vfs.hpp

		internal/vfs/impl/entry.hpp
		internal/vfs/impl/file.hpp
		internal/vfs/impl/fs_proxy.hpp
		internal/vfs/impl/fs.hpp
		internal/vfs/impl/os_file.hpp
		internal/vfs/impl/os_fs.hpp
		internal/vfs/impl/utils.hpp
		internal/vfs/impl/vfile.hpp
		internal/vfs/impl/vfs.hpp

		src/copy.cpp
		src/entry.cpp
		src/file.cpp
		src/fs.cpp
		src/mount.cpp
		src/os_file.cpp
		src/os_fs.cpp
		src/utils.cpp
		src/vfile.cpp
		src/vfs.cpp
)
target_include_directories(
	vfs
		PUBLIC
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
			$<INSTALL_INTERFACE:${CMAKE_INSTALL_PREFIX}/include>
			$<BUILD_INTERFACE:$<$<BOOL:$<TARGET_PROPERTY:vfs_INTERNAL>>:${CMAKE_CURRENT_SOURCE_DIR}/internal>>
		PRIVATE
			$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/internal>
)
add_library(
	vfs::vfs
		ALIAS vfs
)



if(${PROJECT_NAME}_TIDY)
	set_target_properties(
		vfs PROPERTIES
			CXX_CLANG_TIDY "clang-tidy"
	)
endif()

if(${PROJECT_NAME}_TESTS)
	add_subdirectory(tests)
endif()
