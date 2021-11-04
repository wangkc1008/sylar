cmake_minimum_required(VERSION 2.8)
project(sylar)

# include(cmake/utils.cmake)
set(CMAKE_VERBOSE_MAKEFILE ON)
# set(CMAKE_CXX_FLAGS "$ENV(CXXFLAGS) -rdynamic -O3 -g -std=c++11 -Wall")
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O3 -g -std=c++11 -Wall -Wno-deprected -Werror -Wno-unused-function")

include_directories(.)
# include_directories(/home/wangkc/demo/yaml-cpp-master/include)
link_directories(/home/wangkc/demo/yaml-cpp-master/build)

set(LIB_SRC 
    sylar/log.cc
    sylar/util.cc
    sylar/config.cc
    sylar/thread.cc)

add_library(sylar SHARED ${LIB_SRC})

find_library(PTHREAD pthread)
find_library(YAMLCPP yaml-cpp)

set(LIBS
    sylar
    pthread
    yaml-cpp)

add_executable(test tests/test.cc)
add_dependencies(test sylar)
target_link_libraries(test ${LIBS})

add_executable(test_config tests/test_config.cc)
add_dependencies(test_config sylar)
target_link_libraries(test_config ${LIBS})

add_executable(test_thread tests/test_thread.cc)
add_dependencies(test_thread sylar)
target_link_libraries(test_thread ${LIBS})

add_executable(test_util tests/test_util.cc)
add_dependencies(test_util sylar)
target_link_libraries(test_util ${LIBS})

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)