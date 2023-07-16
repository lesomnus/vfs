find_package(Catch2 CONFIG REQUIRED)

enable_testing()
include(Catch)



add_library(
	testing
		include/testing.hpp

		src/testing.cpp
)
target_include_directories(
	testing PUBLIC
		include
)
target_link_libraries(
	testing PUBLIC
		vfs
)
set_target_properties(
	testing PROPERTIES
		vfs_INTERNAL 1
)



add_custom_target(test-all)

add_custom_target(
	run-test
		COMMAND ${CMAKE_CTEST_COMMAND}
)
add_dependencies(run-test test-all)



if(${PROJECT_IS_TOP_LEVEL} AND CMAKE_BUILD_TYPE STREQUAL "Debug")
	file(
		DOWNLOAD
			https://raw.githubusercontent.com/bilke/cmake-modules/877bab9dd1b17468c5d939cacaa2ad7ba99d1977/CodeCoverage.cmake
	    	${PROJECT_BINARY_DIR}/tests/CodeCoverage.cmake
	)

	SET(HAVE_fprofile_abs_path FALSE) # To avoid clang-tidy unknown compiler argument error.
	include(${PROJECT_BINARY_DIR}/tests/CodeCoverage.cmake)
	append_coverage_compiler_flags_to_target(vfs)

	target_link_libraries(
		vfs PRIVATE
			gcov
	)

	setup_target_for_coverage_lcov(
		NAME       coverage
		EXECUTABLE ctest --test-dir "${PROJECT_BINARY_DIR}/tests"
		EXCLUDE
			"build/*"
			"tests/*"
			"${CMAKE_INSTALL_PREFIX}/*"
	)
	add_dependencies(coverage test-all)
endif()

macro(vfs_SIMPLE_TEST NAME)
	add_executable(
		test-${NAME}
			include/testing/suites/file.hpp
			include/testing/suites/fs.hpp

			src/${NAME}.cpp
	)
	target_include_directories(
		test-${NAME} PRIVATE
			include
	)
	target_link_libraries(
		test-${NAME} PRIVATE
			Catch2::Catch2WithMain
			testing
	)
	set_target_properties(
		test-${NAME} PROPERTIES
			OUTPUT_NAME  ${NAME}
			vfs_INTERNAL 1
	)
	catch_discover_tests(test-${NAME})
	
	add_dependencies(test-all test-${NAME})
endmacro()

vfs_SIMPLE_TEST(copy)
vfs_SIMPLE_TEST(directory_entry)
vfs_SIMPLE_TEST(entry)
vfs_SIMPLE_TEST(mem_file)
vfs_SIMPLE_TEST(mem_fs)
vfs_SIMPLE_TEST(mount)
vfs_SIMPLE_TEST(os_file)
vfs_SIMPLE_TEST(os_fs)
vfs_SIMPLE_TEST(union_file)
vfs_SIMPLE_TEST(vfile)
vfs_SIMPLE_TEST(vfs)

vfs_SIMPLE_TEST(example)
