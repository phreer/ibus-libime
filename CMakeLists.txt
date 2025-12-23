cmake_minimum_required(VERSION 3.10)

file(READ "${CMAKE_SOURCE_DIR}/VERSION" PROJECT_VERSION)
string(STRIP "${PROJECT_VERSION}" PROJECT_VERSION)
project(ibus-libime VERSION ${PROJECT_VERSION})

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find required packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(IBUS REQUIRED ibus-1.0)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
pkg_check_modules(GLIBMM REQUIRED glibmm-2.4)

# Find LibIME
find_package(LibIMECore REQUIRED)
find_package(LibIMEPinyin REQUIRED)

# Include directories
include_directories(
    ${IBUS_INCLUDE_DIRS}
    ${GLIB2_INCLUDE_DIRS}
    ${GLIBMM_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Source files
set(SOURCES
    src/main.cpp
    src/ibus_engine.cpp
    src/pinyin_engine.cpp
    src/configs.cpp
)

# Create executable
add_executable(ibus-engine-libime ${SOURCES})

# Link libraries
target_link_libraries(ibus-engine-libime
    ${IBUS_LIBRARIES}
    ${GLIB2_LIBRARIES}
    ${GLIBMM_LIBRARIES}
    LibIME::Core
    LibIME::Pinyin
)

# Install
install(TARGETS ibus-engine-libime
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}
)

# Install IBus component file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/data/libime.xml.in
    ${CMAKE_CURRENT_BINARY_DIR}/libime.xml
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libime.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/ibus/component
)
