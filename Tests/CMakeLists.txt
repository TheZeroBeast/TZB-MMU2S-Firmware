cmake_minimum_required(VERSION 3.1)

set (CMAKE_CXX_STANDARD 11)

project(cmake_test)

# Prepare "Catch" library for other executables
set(CATCH_INCLUDE_DIR ../Catch2)
add_library(Catch INTERFACE)
target_include_directories(Catch INTERFACE ${CATCH_INCLUDE_DIR})

# Make test executable
add_executable(tests
	tests.cpp
	Example_test.cpp
	../MM-control-01/permanent_storage.cpp
	permanent_storage_test.cpp
)

target_link_libraries(tests Catch)
target_include_directories(tests PRIVATE .)
