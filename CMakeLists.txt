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
pkg_check_modules(GLIB2 REQUIRED glib-2.0>=2.66 gio-2.0)
pkg_check_modules(GLIBMM REQUIRED glibmm-2.4)
pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)

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
    src/english_dictionary.cpp
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

add_executable(ibus-setup-libime src/setup.cpp src/configs.cpp)
target_link_libraries(ibus-setup-libime PkgConfig::GTK3)

# Install
install(TARGETS ibus-engine-libime ibus-setup-libime
    RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}
)

configure_file(data/ibus-setup-libime.desktop.in
    ${CMAKE_CURRENT_BINARY_DIR}/ibus-setup-libime.desktop @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ibus-setup-libime.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

# Install IBus component file
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/data/libime.xml.in
    ${CMAKE_CURRENT_BINARY_DIR}/libime.xml
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libime.xml
    DESTINATION ${CMAKE_INSTALL_DATADIR}/ibus/component
)

install(FILES data/english-frequency.txt data/english-README.txt
              data/english-LICENSE.txt data/english-technical.txt
    DESTINATION ${CMAKE_INSTALL_DATADIR}/ibus-libime)

include(CTest)
if(BUILD_TESTING)
    add_executable(test-configs tests/test_configs.cpp src/configs.cpp)
    target_link_libraries(test-configs ${GLIB2_LIBRARIES})
    add_test(NAME configs COMMAND test-configs)
    add_executable(test-english-dictionary
        tests/test_english_dictionary.cpp src/english_dictionary.cpp)
    target_link_libraries(test-english-dictionary ${GLIB2_LIBRARIES})
    add_test(NAME english-dictionary COMMAND test-english-dictionary
        ${CMAKE_CURRENT_SOURCE_DIR}/data/english-frequency.txt)
    add_executable(test-pinyin-english tests/test_pinyin_english.cpp
        src/ibus_engine.cpp src/pinyin_engine.cpp src/configs.cpp
        src/english_dictionary.cpp)
    target_link_libraries(test-pinyin-english
        ${IBUS_LIBRARIES} ${GLIB2_LIBRARIES} ${GLIBMM_LIBRARIES}
        LibIME::Core LibIME::Pinyin)
    foreach(mode enabled disabled missing history-disabled learning-disabled runtime-settings)
        add_test(NAME pinyin-english-${mode} COMMAND test-pinyin-english
            ${CMAKE_CURRENT_SOURCE_DIR}/data ${mode})
    endforeach()
    foreach(mode persistence shutdown-normal shutdown-sigint shutdown-sigterm)
        add_test(NAME pinyin-${mode} COMMAND test-pinyin-english
            ${CMAKE_CURRENT_SOURCE_DIR}/data ${mode})
        set_tests_properties(pinyin-${mode} PROPERTIES TIMEOUT 30)
    endforeach()
endif()
