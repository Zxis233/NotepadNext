cmake_minimum_required(VERSION 3.21)
project(NotepadNextEncodingTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Core5Compat Test)
enable_testing()

add_executable(FileEncodingTest FileEncodingTest.cpp ../../src/FileEncoding.cpp)
target_include_directories(FileEncodingTest PRIVATE ../../src)
target_link_libraries(FileEncodingTest PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Test)
add_test(NAME FileEncodingTest COMMAND FileEncodingTest)

add_executable(FileEncodingHistoryTest FileEncodingHistoryTest.cpp
    ../../src/FileEncodingHistory.cpp ../../src/FileEncoding.cpp)
target_include_directories(FileEncodingHistoryTest PRIVATE ../../src)
target_link_libraries(FileEncodingHistoryTest PRIVATE Qt6::Core Qt6::Core5Compat Qt6::Test)
add_test(NAME FileEncodingHistoryTest COMMAND FileEncodingHistoryTest)
