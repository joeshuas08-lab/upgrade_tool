cmake_minimum_required(VERSION 3.16)

project(upgrade_tool LANGUAGES CXX)

include(ExternalProject)
include(FetchContent)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "This build configuration supports Linux only.")
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB ROOT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB LINUX_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/os/linux/*.cpp")

add_executable(upgrade_tool
    ${ROOT_SOURCES}
    ${LINUX_SOURCES}
)

target_link_options(upgrade_tool PRIVATE -static -static-libgcc -static-libstdc++)

target_include_directories(upgrade_tool
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/os/linux"
)

find_package(Threads REQUIRED)

set(UPGRADE_TOOL_THIRD_PARTY_DIR "${CMAKE_CURRENT_BINARY_DIR}/third_party")
set(UPGRADE_TOOL_LIBUDEV_ZERO_DIR "${UPGRADE_TOOL_THIRD_PARTY_DIR}/libudev-zero-src")
set(UPGRADE_TOOL_LIBUDEV_ZERO_ARCHIVE "${UPGRADE_TOOL_LIBUDEV_ZERO_DIR}/libudev.a")

FetchContent_Declare(libudev_zero_source
    URL https://github.com/illiliti/libudev-zero/archive/refs/tags/1.0.3.tar.gz
    SOURCE_DIR "${UPGRADE_TOOL_LIBUDEV_ZERO_DIR}"
)
FetchContent_Populate(libudev_zero_source)

find_path(LIBUSB_INCLUDE_DIR
    NAMES libusb.h
    PATH_SUFFIXES libusb-1.0
)

find_path(LIBUDEV_INCLUDE_DIR
    NAMES libudev.h udev.h
    PATHS
        "${UPGRADE_TOOL_LIBUDEV_ZERO_DIR}"
)

ExternalProject_Add(libudev_zero_project
    SOURCE_DIR "${UPGRADE_TOOL_LIBUDEV_ZERO_DIR}"
    CONFIGURE_COMMAND ""
    BUILD_COMMAND ${CMAKE_MAKE_PROGRAM} libudev.a
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE TRUE
)

set(ORIGINAL_CMAKE_FIND_LIBRARY_SUFFIXES "${CMAKE_FIND_LIBRARY_SUFFIXES}")
set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
find_library(LIBUSB_LIBRARY
    NAMES usb-1.0 libusb-1.0
)
find_library(LIBUDEV_LIBRARY
    NAMES udev libudev
    PATHS
        "${UPGRADE_TOOL_LIBUDEV_ZERO_DIR}"
    NO_DEFAULT_PATH
)
set(CMAKE_FIND_LIBRARY_SUFFIXES "${ORIGINAL_CMAKE_FIND_LIBRARY_SUFFIXES}")

if(NOT LIBUDEV_LIBRARY)
    set(LIBUDEV_LIBRARY "${UPGRADE_TOOL_LIBUDEV_ZERO_ARCHIVE}")
endif()

if(NOT LIBUSB_INCLUDE_DIR OR NOT LIBUSB_LIBRARY)
    message(FATAL_ERROR "Static dependency not found. Ensure libusb-1.0.a is available for static linking.")
endif()

if(NOT LIBUDEV_INCLUDE_DIR OR NOT LIBUDEV_LIBRARY)
    message(FATAL_ERROR "Static dependency not found. Failed to fetch or prepare libudev-zero sources.")
endif()

target_include_directories(upgrade_tool PRIVATE "${LIBUSB_INCLUDE_DIR}")
target_include_directories(upgrade_tool PRIVATE "${LIBUDEV_INCLUDE_DIR}")

target_link_libraries(upgrade_tool PRIVATE "${LIBUSB_LIBRARY}")
target_link_libraries(upgrade_tool PRIVATE "${LIBUDEV_LIBRARY}")
add_dependencies(upgrade_tool libudev_zero_project)

target_link_libraries(upgrade_tool PRIVATE Threads::Threads)

include(CheckLibraryExists)
check_library_exists(rt shm_open "" HAVE_LIBRT_SHM_OPEN)
if(HAVE_LIBRT_SHM_OPEN)
    target_link_libraries(upgrade_tool PRIVATE rt)
endif()
