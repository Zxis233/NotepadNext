cmake_minimum_required(VERSION 3.21)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(NotepadNextTabTests LANGUAGES CXX)
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_AUTOMOC ON)
    set(CPM_SOURCE_CACHE "${CMAKE_CURRENT_SOURCE_DIR}/../../.cpm-cache" CACHE PATH "CPM source cache directory")
    find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test)
    enable_testing()
    include("../../cmake/Ads.cmake")
else()
    find_package(Qt6 REQUIRED COMPONENTS Test)
endif()

add_executable(WrappedTabsTest WrappedTabsTest.cpp
    ../../src/ApplicationSettings.cpp ../../src/ApplicationSettings.h)
# Keep standard output available for CTest when using Qt Widgets on Windows.
set_target_properties(WrappedTabsTest PROPERTIES WIN32_EXECUTABLE FALSE)
target_include_directories(WrappedTabsTest PRIVATE ../../src ../../src/widgets)
target_compile_definitions(WrappedTabsTest PRIVATE
    NOTEPADNEXT_TEST_STYLESHEET="${CMAKE_CURRENT_SOURCE_DIR}/../../src/stylesheets/npp.css")
target_link_libraries(WrappedTabsTest PRIVATE Qt6::Widgets Qt6::Test qtadvanceddocking-qt6)
add_test(NAME WrappedTabsTest COMMAND WrappedTabsTest -o -,txt
    -o "${CMAKE_CURRENT_BINARY_DIR}/WrappedTabsTest.junit.xml,junitxml")
set_tests_properties(WrappedTabsTest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
