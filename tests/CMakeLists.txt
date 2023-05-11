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



add_custom_target(all-test)

macro(vfs_SIMPLE_TEST NAME)
	add_executable(
		${NAME}-test
			include/testing/suites/fs.hpp

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
		all-test
			${NAME}-test
	)
endmacro()

vfs_SIMPLE_TEST(entry)
vfs_SIMPLE_TEST(vfs)
