cmake_minimum_required(VERSION 3.7.2...4.3)

if(${CMAKE_VERSION} VERSION_LESS "3.12.0")
    project(ch341eeprom)
else()
    project(ch341eeprom C)
endif()

set(VERSION "0.5")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(SOURCES
    ch341eeprom.c
    ch341funcs.c
)

set(HEADERS
    ch341eeprom.h
)

option(BUILD_MKTESTIMG "Build test image generator mktestimg" OFF)

set(LIBUSB_INCLUDE_DIR "" CACHE PATH "Path to libusb include directory (optional)")
set(LIBUSB_LIBRARY "" CACHE FILEPATH "Path to libusb library file (optional)")

if(LIBUSB_INCLUDE_DIR AND LIBUSB_LIBRARY)
    set(LIBUSB_FOUND TRUE)
    set(LIBUSB_INCLUDE_DIRS ${LIBUSB_INCLUDE_DIR})
    set(LIBUSB_LIBRARIES ${LIBUSB_LIBRARY})
    message(STATUS "Using manually specified libusb: include=${LIBUSB_INCLUDE_DIRS}, lib=${LIBUSB_LIBRARIES}")
else()
    find_package(PkgConfig)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(LIBUSB libusb-1.0)
        if(LIBUSB_FOUND)
            message(STATUS "Found libusb via pkg-config: ${LIBUSB_LIBRARIES}")
        endif()
    endif()

    if(NOT LIBUSB_FOUND)
        # Fallback
        find_path(LIBUSB_INCLUDE_DIRS NAMES libusb.h PATH_SUFFIXES libusb-1.0)
        find_library(LIBUSB_LIBRARIES NAMES usb-1.0 libusb-1.0)
        if(LIBUSB_INCLUDE_DIRS AND LIBUSB_LIBRARIES)
            set(LIBUSB_FOUND TRUE)
            message(STATUS "Found libusb via standard search: include=${LIBUSB_INCLUDE_DIRS}, lib=${LIBUSB_LIBRARIES}")
        else()
            message(FATAL_ERROR "Could not find libusb-1.0. Please install libusb-1.0 development package or set LIBUSB_INCLUDE_DIR and LIBUSB_LIBRARY in CMake cache.")
        endif()
    endif()
endif()

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})
if(BUILD_MKTESTIMG)
    add_executable(mktestimg mktestimg.c)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE ${LIBUSB_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${LIBUSB_LIBRARIES})
if(DEFINED LIBUSB_CFLAGS_OTHER)
    target_compile_options(${PROJECT_NAME} PRIVATE ${LIBUSB_CFLAGS_OTHER})
endif()

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin COMPONENT runtime)

install(FILES README.md DESTINATION share/doc/${PROJECT_NAME} COMPONENT doc OPTIONAL)
