cmake_minimum_required(VERSION 2.8)
project(sylar)

include(cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")

include_directories(.)
include_directories(/usr/local/include)

link_directories(/usr/local/lib)

set(LIB_SRC
        sylar/log.cc
        sylar/config.cc
        sylar/thread.cc
        sylar/mutex.cc
        sylar/noncopyable.h
        sylar/macro.h
        sylar/fiber.h
        sylar/fiber.cc
        sylar/scheduler.h
        sylar/scheduler.cc
        sylar/iomanager.h
        sylar/iomanager.cc
        sylar/timer.h
        sylar/timer.cc
        sylar/hook.h
        sylar/hook.cc
        sylar/fdmanager.h
        sylar/fdmanager.cc
        sylar/socket.h
        sylar/socket.cc
        sylar/address.h
        sylar/address.cc
        sylar/endian.h
        sylar/util.h
        sylar/util.cc
        sylar/stream.h
        sylar/stream.cc
        sylar/tcp_server.h
        sylar/tcp_server.cc
        sylar/uri.h
        sylar/uri.cc
        sylar/bytearray.h
        sylar/bytearray.cc
        sylar/http/http-parser/http_parser.c
        sylar/http/http.cc
        sylar/http/http_connection.cc
        sylar/http/http_parser.cc
        sylar/http/http_server.cc
        sylar/http/http_session.cc
        sylar/http/servlet.cc
        sylar/streams/socket_stream.cc)

set(LIB_LIB
        sylar
        dl
        pthread
        yaml-cpp
        )

add_library(sylar SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(sylar)  #__FILE__
#add_library(sylar_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES (syalr_static PROPERTIES OUTPUT_NAME "sylar")

add_executable(test tests/test.cc)
add_dependencies(test sylar)
force_redefine_file_macro_for_sources(test)  #__FILE__
target_link_libraries(test ${LIB_LIB})

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config sylar)
force_redefine_file_macro_for_sources(test_config)  #__FILE__
target_link_libraries(test_config sylar ${LIB_LIB})

add_executable(test_thread tests/test_thread.cc)
add_dependencies(test_thread sylar)
force_redefine_file_macro_for_sources(test_thread)  #__FILE__
target_link_libraries(test_thread sylar ${LIB_LIB})

add_executable(test_util tests/test_util.cc)
add_dependencies(test_util sylar)
force_redefine_file_macro_for_sources(test_util)  #__FILE__
target_link_libraries(test_util sylar ${LIB_LIB})

add_executable(test_fiber tests/test_fiber.cc)
add_dependencies(test_fiber sylar)
force_redefine_file_macro_for_sources(test_fiber)  #__FILE__
target_link_libraries(test_fiber sylar ${LIB_LIB})

add_executable(test_scheduler tests/test_scheduler.cc)
add_dependencies(test_scheduler sylar)
force_redefine_file_macro_for_sources(test_scheduler)  #__FILE__
target_link_libraries(test_scheduler sylar ${LIB_LIB})

add_executable(test_iomanager tests/test_iomanager.cc)
add_dependencies(test_iomanager sylar)
force_redefine_file_macro_for_sources(test_iomanager)  #__FILE__
target_link_libraries(test_iomanager sylar ${LIB_LIB})

add_executable(test_timer tests/test_timer.cc)
add_dependencies(test_timer sylar)
force_redefine_file_macro_for_sources(test_timer)  #__FILE__
target_link_libraries(test_timer sylar ${LIB_LIB})

add_executable(test_hook tests/test_hook.cc)
add_dependencies(test_hook sylar)
force_redefine_file_macro_for_sources(test_hook)  #__FILE__
target_link_libraries(test_hook sylar ${LIB_LIB})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
