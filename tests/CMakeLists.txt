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



if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	file(
		DOWNLOAD
			https://raw.githubusercontent.com/bilke/cmake-modules/877bab9dd1b17468c5d939cacaa2ad7ba99d1977/CodeCoverage.cmake
	    	${PROJECT_BINARY_DIR}/tests/CodeCoverage.cmake
	)
	include(${PROJECT_BINARY_DIR}/tests/CodeCoverage.cmake)

	string(REPLACE " " ";" COVERAGE_COMPILER_FLAG_LIST ${COVERAGE_COMPILER_FLAGS})

	target_compile_options(
		vfs PUBLIC
			${COVERAGE_COMPILER_FLAG_LIST}
	)
	target_link_libraries(
		vfs PUBLIC
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
endif()

add_custom_target(
	test-all
		COMMAND ${CMAKE_CTEST_COMMAND}
)

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
	add_dependencies(coverage test-${NAME})
endmacro()

vfs_SIMPLE_TEST(copy)
vfs_SIMPLE_TEST(directory_entry)
vfs_SIMPLE_TEST(entry)
vfs_SIMPLE_TEST(mount)
vfs_SIMPLE_TEST(os_file)
vfs_SIMPLE_TEST(os_fs)
vfs_SIMPLE_TEST(vfile)
vfs_SIMPLE_TEST(vfs)

vfs_SIMPLE_TEST(example)
