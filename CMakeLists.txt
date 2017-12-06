cmake_minimum_required(VERSION 3.1)

project(glade)

set(CMAKE_CXX_STANDARD 14)
# Compile as strictly as possible under GCC.
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
		-Wall -Wextra -Winit-self -Wuninitialized -Wmissing-declarations \
		-pedantic")
endif()
enable_testing()

set(CMAKE_BINARY_DIR ${PROJECT_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR})
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR})

# Import packes.
include(GNUInstallDirs)
find_package(Boost 1.59 REQUIRED COMPONENTS unit_test_framework REQUIRED)

# The header-only library.
add_library(GladeLib INTERFACE)

target_include_directories(
	GladeLib INTERFACE
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
	${PROJECT_INCLUDE_DIR}
)

message(STATUS "test ${CMAKE_INSTALL_BINDIR}")
message(STATUS "test ${CMAKE_INSTALL_INCLUDEDIR}")
install(
	TARGETS GladeLib
	EXPORT GladeLibConfig
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(
	EXPORT GladeLibConfig
	DESTINATION share/GladeLib/cmake
)

export(TARGETS GladeLib FILE GladeLibConfig.cmake)

# Unit tests.
add_executable(OrthtreeTest test/orthtree_test.cpp)

target_compile_options(
	OrthtreeTest PRIVATE
	-DBOOST_TEST_DYN_LINK
)
target_include_directories(
	OrthtreeTest PRIVATE
	${Boost_INCLUDE_DIRS}
)
target_link_libraries(
	OrthtreeTest
	GladeLib
	${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
)
add_test(NAME OrthtreeTest COMMAND OrthtreeTest)

