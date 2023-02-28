cmake_minimum_required(VERSION 3.16)
project(ytorrent)
add_compile_options(-Wall -Wextra -Wpedantic -march=native -ftrapv -fsanitize=address -O0 -g3) #-Wconversion -Wshadow 
add_link_options(-fsanitize=address)

add_executable(${PROJECT_NAME}
    src/torrent/peer.cpp
    src/torrent/torrent.cpp
    src/torrent/tracker.cpp
    src/net/acceptor.cpp
    src/net/connection.cpp
    src/utils/utils.cpp
    main.cpp
)

# set(CMAKE_BUILD_TYPE Debug)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
# target_include_directories(${PROJECT_NAME} PUBLIC "/home/yavor/ytorrent/dependencies/boost_1_71_0")
find_package(Boost COMPONENTS program_options REQUIRED)
target_link_libraries(${PROJECT_NAME}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
)

target_link_libraries(${PROJECT_NAME} 
    Threads::Threads 
    "/home/yavor/ytorrent/dependencies/cpp-bencoding/install/lib/libbencoding.a"
)


# !!!!!! https://stackoverflow.com/a/1065729/18301773 !!!!!!
# find_package(Boost COMPONENTS system filesystem REQUIRED)
# target_link_libraries(${PROJECT_NAME} 
#   ${Boost_FILESYSTEM_LIBRARY}
#   ${Boost_SYSTEM_LIBRARY}
# )

target_include_directories(${PROJECT_NAME} PUBLIC "/home/yavor/ytorrent/dependencies/cpp-bencoding/install/include")
target_include_directories(${PROJECT_NAME} PUBLIC "/home/yavor/ytorrent/src")
