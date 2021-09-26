
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  message("HERE")
  SET(CMAKE_INSTALL_PREFIX ../x86_64/target/ CACHE PATH comment FORCE)
  #SET(CMAKE_INSTALL_RPATH "../../x86_64/target/lib/")
  #SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

SET(CMAKE_INSTALL_RPATH "$ORIGIN/.")
SET(CMAKE_BUILD_RPATH "$ORIGIN/.")

option(USE_CLANG "build application with clang" ON) # ON is the default
set (CMAKE_CXX_STANDARD 17)
if(USE_CLANG)
    SET (CMAKE_CXX_COMPILER clang++)
endif(USE_CLANG)


add_executable(neumo-blindscan neumo-blindscan.cc)

add_executable(neumo-tune neumo-tune.cc)

target_link_libraries(neumo-blindscan PRIVATE   ${Boost_PROGRAM_OPTIONS_LIBRARY} stdc++fs)
target_link_libraries(neumo-tune PRIVATE   ${Boost_PROGRAM_OPTIONS_LIBRARY} stdc++fs)

install (TARGETS neumo-blindscan neumo-tune)
