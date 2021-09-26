cmake_minimum_required(VERSION 3.12.1)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-z,defs -fuse-ld=gold")
project(blindscan VERSION 1.0.0)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
option(USE_CLANG "build application with clang" ON) # ON is the default
set (CMAKE_CXX_STANDARD 17)
if(USE_CLANG)
    SET (CMAKE_CXX_COMPILER clang++)
endif(USE_CLANG)

get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
if(LIB64)
  set(LIBSUFFIX 64)
else()
  set(LIBSUFFIX "")
endif()

#add_compile_options (-fsanitize=address -fPIC -fsized-deallocation)
# -DHAS_UNCAUGHT_EXCEPTIONS=1 for date module
add_compile_options (-fPIC -fsized-deallocation -DHAS_UNCAUGHT_EXCEPTIONS=1 -D DATA_DIR=${CMAKE_SOURCE_DIR}/data)
#add_link_options( -lasan)

set(CMAKE_BUILD_TYPE Debug)

# Set the install prefix
#set(CMAKE_INSTALL_PREFIX "/usr")
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "../x86_64/")
endif()

string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")


add_subdirectory(src)
