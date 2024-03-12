cmake_minimum_required(VERSION 2.8)
project(sylar)

#include (/usr/local/bin/cmake/utils.cmake)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined")
set(LIB_SRC 
        sylar/log.cc
        sylar/util.cc
        sylar/config.cc
        sylar/thread.cc
        sylar/fiber.cc
        sylar/scheduler.cc
        sylar/iomanager.cc
        sylar/mutex.cc
        sylar/timer.cc
        sylar/hook.cc
        sylar/fd_manager.cc
        sylar/address.cc
        sylar/socket.cc
        sylar/bytearray.cc
        sylar/http/http.cc
        sylar/http/http11_parser.rl.cc
        sylar/http/httpclient_parser.rl.cc
        sylar/http/http_parser.cc
        sylar/tcp_server.cc
        sylar/stream.cc
        sylar/socket_stream.cc
        sylar/http/http_session.cc
        sylar/http/http_server.cc
        sylar/http/servlet.cc
        sylar/http/http_connection.cc
        sylar/uri.rl.cc
        sylar/daemon.cc
)



include_directories(.)
include_directories(/usr/local/include)#yaml的路径
link_directories(/usr/local/lib)




add_library(sylar SHARED ${LIB_SRC})
#force_redefine_file_macro_for_sources(sylar)     #重定义__FILE__

#add_library(sylar_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES(sylar_static PROPERTIES OUTPUT_NAME "sylar")

set(LIB_LIB
    dl
    sylar
    pthread 
    yaml-cpp
    )

add_executable(test tests/test.cc)
add_dependencies(test sylar)
#force_redefine_file_macro_for_sources(test)     #重定义__FILE__
target_link_libraries(test  ${LIB_LIB}) 
#force_redefine_file_macro_for_sources(test_uri) #__FILE__

add_executable(test_config tests/test_config.cc )
add_dependencies(test_config sylar)
#force_redefine_file_macro_for_sources(test_config)     #重定义__FILE__
target_link_libraries(test_config sylar ${LIB_LIB})

add_executable(test_thread tests/test_thread.cc )
add_dependencies(test_thread sylar)
#force_redefine_file_macro_for_sources(test_thread)     #重定义__FILE__
target_link_libraries(test_thread sylar ${LIB_LIB})

add_executable(test_util tests/test_util.cc )
add_dependencies(test_util sylar)
#force_redefine_file_macro_for_sources(test_util)     #重定义__FILE__
target_link_libraries(test_util sylar ${LIB_LIB})

add_executable(test_fiber tests/test_fiber.cc )
add_dependencies(test_fiber sylar)
#force_redefine_file_macro_for_sources(test_fiber)     #重定义__FILE__
target_link_libraries(test_fiber sylar ${LIB_LIB})

add_executable(test_scheduler tests/test_scheduler.cc )
add_dependencies(test_scheduler sylar)
#force_redefine_file_macro_for_sources(test_scheduler)     #重定义__FILE__
target_link_libraries(test_scheduler sylar ${LIB_LIB})

add_executable(test_iomanager tests/test_iomanager.cc )
add_dependencies(test_iomanager sylar)
#force_redefine_file_macro_for_sources(test_iomanager)     #重定义__FILE__
target_link_libraries(test_iomanager sylar ${LIB_LIB})

add_executable(test_hook tests/test_hook.cc )
add_dependencies(test_hook sylar)
#force_redefine_file_macro_for_sources(test_hook)     #重定义__FILE__
target_link_libraries(test_hook sylar ${LIB_LIB})

add_executable(test_address tests/test_address.cc )
add_dependencies(test_address sylar)
#force_redefine_file_macro_for_sources(test_address)     #重定义__FILE__
target_link_libraries(test_address sylar ${LIB_LIB})

add_executable(test_socket tests/test_socket.cc )
add_dependencies(test_socket sylar)
#force_redefine_file_macro_for_sources(test_socket)     #重定义__FILE__
target_link_libraries(test_socket sylar ${LIB_LIB})

# add_executable(test_bytearray tests/test_bytearray.cc )
# add_dependencies(test_bytearray sylar)
# #force_redefine_file_macro_for_sources(test_bytearray)     #重定义__FILE__
# target_link_libraries(test_bytearray sylar ${LIB_LIB})

add_executable(test_http tests/test_http.cc )
add_dependencies(test_http  sylar)
#force_redefine_file_macro_for_sources(test_http )     #重定义__FILE__
target_link_libraries(test_http  sylar ${LIB_LIB})

add_executable(test_http_parser tests/test_http_parser.cc )
add_dependencies(test_http_parser sylar)
#force_redefine_file_macro_for_sources(test_http_parser)     #重定义__FILE__
target_link_libraries(test_http_parser  sylar ${LIB_LIB})

add_executable(test_tcp_server tests/test_tcp_server.cc )
add_dependencies(test_tcp_server sylar)
#force_redefine_file_macro_for_sources(test_tcp_server)     #重定义__FILE__
target_link_libraries(test_tcp_server  sylar ${LIB_LIB})

add_executable(echo_server example/echo_server.cc )
add_dependencies(echo_server sylar)
#force_redefine_file_macro_for_sources(echo_server)     #重定义__FILE__
target_link_libraries(echo_server  sylar ${LIB_LIB})

add_executable(test_http_server tests/test_http_server )
add_dependencies(test_http_server sylar)
#force_redefine_file_macro_for_sources(test_http_server)     #重定义__FILE__
target_link_libraries(test_http_server sylar ${LIB_LIB})

add_executable(test_http_connection tests/test_http_connection )
add_dependencies(test_http_connection sylar)
#force_redefine_file_macro_for_sources(test_http_connection)     #重定义__FILE__
target_link_libraries(test_http_connection sylar ${LIB_LIB})

add_executable(test_uri tests/test_uri )
add_dependencies(test_uri sylar)
#force_redefine_file_macro_for_sources(test_uri)     #重定义__FILE__
target_link_libraries(test_uri sylar ${LIB_LIB})

add_executable(test_daemon tests/test_daemon )
add_dependencies(test_daemon sylar)
#force_redefine_file_macro_for_sources(test_daemon)     #重定义__FILE__
target_link_libraries(test_daemon sylar ${LIB_LIB})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)