
add_executable(neumo-blindscan neumo-blindscan.cc)

add_executable(neumo-tune neumo-tune.cc)

target_link_libraries(neumo-blindscan PRIVATE   ${Boost_PROGRAM_OPTIONS_LIBRARY} stdc++fs)
target_link_libraries(neumo-tune PRIVATE   ${Boost_PROGRAM_OPTIONS_LIBRARY} stdc++fs)
