include(FetchContent)

FetchContent_Declare(
	Catch2
	GIT_REPOSITORY https://github.com/catchorg/Catch2.git
	GIT_TAG        v3.3.2
)
FetchContent_MakeAvailable(
	Catch2
)



enable_testing()
include(Catch)



add_custom_target(
	test-all
		COMMAND ${CMAKE_CTEST_COMMAND}
)

macro(vfs_SIMPLE_TEST NAME)
	add_executable(
		${NAME}-test
			include/testing/suites/file.hpp
			include/testing/suites/fs.hpp
			include/testing/utils.hpp

			src/${NAME}.cpp
	)
	target_include_directories(
		${NAME}-test PRIVATE
			include
	)
	target_link_libraries(
		${NAME}-test PRIVATE
			Catch2::Catch2WithMain
			vfs
	)
	set_target_properties(
		${NAME}-test PROPERTIES
			vfs_INTERNAL 1
	)
	catch_discover_tests(${NAME}-test)
	
	add_dependencies(
		test-all
			${NAME}-test
	)
endmacro()

vfs_SIMPLE_TEST(directory_entry)
vfs_SIMPLE_TEST(entry)
vfs_SIMPLE_TEST(mount)
vfs_SIMPLE_TEST(os_file)
vfs_SIMPLE_TEST(os_fs)
vfs_SIMPLE_TEST(vfile)
vfs_SIMPLE_TEST(vfs)

vfs_SIMPLE_TEST(example)
