
add_executable(neumo-blindscan neumo-blindscan.cc)

target_link_libraries(neumo-blindscan PRIVATE   ${Boost_PROGRAM_OPTIONS_LIBRARY} stdc++fs)
