cmake_minimum_required(VERSION 3.21)
project(NotepadNextThemeTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Test)
enable_testing()

add_executable(ThemeSettingsTest ThemeSettingsTest.cpp
    ../../src/ApplicationSettings.cpp ../../src/ApplicationSettings.h)
target_include_directories(ThemeSettingsTest PRIVATE ../../src)
target_link_libraries(ThemeSettingsTest PRIVATE Qt6::Core Qt6::Widgets Qt6::Test)
add_test(NAME ThemeSettingsTest COMMAND ThemeSettingsTest)
