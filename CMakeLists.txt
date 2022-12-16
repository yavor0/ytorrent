cmake_minimum_required(VERSION 3.16)
project(ytorrent)
add_executable(${PROJECT_NAME} 
    main.cpp
    src/utils/Utils.cpp
    src/torrent/TorrentMeta.cpp
    src/torrent/Tracker.cpp
    )

set(CMAKE_BUILD_TYPE Debug)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC "/home/yavor/ytorrent/dependencies/boost_1_80_0")
target_link_libraries(${PROJECT_NAME} 
    Threads::Threads 
    "/home/yavor/ytorrent/dependencies/cpp-bencoding/install/lib/libbencoding.a")

target_include_directories(${PROJECT_NAME} PUBLIC "/home/yavor/ytorrent/dependencies/cpp-bencoding/install/include")
target_include_directories(${PROJECT_NAME} PUBLIC "/home/yavor/ytorrent/src/torrent")
target_include_directories(${PROJECT_NAME} PUBLIC "/home/yavor/ytorrent/src/utils")