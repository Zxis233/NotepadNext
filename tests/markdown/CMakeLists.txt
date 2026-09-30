cmake_minimum_required(VERSION 3.21)
project(NotepadNextMarkdownTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
enable_testing()

add_subdirectory(../../thirdparty/lexilla lexilla)
target_include_directories(lexilla PRIVATE ../../thirdparty/scintilla/include)
add_executable(MarkdownLexerTest MarkdownLexerTest.cpp
    ../../thirdparty/lexilla/test/TestDocument.cxx)
target_include_directories(MarkdownLexerTest PRIVATE
    ../../thirdparty/scintilla/include ../../thirdparty/lexilla/test)
target_link_libraries(MarkdownLexerTest PRIVATE lexilla)
add_test(NAME MarkdownLexerTest COMMAND MarkdownLexerTest)
