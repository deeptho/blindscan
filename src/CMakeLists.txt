
add_executable(blindscan blindscan.cc)

target_link_libraries(blindscan PRIVATE   ${Boost_PROGRAM_OPTIONS_LIBRARY} stdc++fs)
