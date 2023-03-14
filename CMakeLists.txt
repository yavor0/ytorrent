cmake_minimum_required(VERSION 3.16)
project(ytorrent)
add_compile_options(
    # -Wall -Wextra -Wpedantic -Wunused-result -march=native -ftrapv
    # -fsanitize=thread
    # -fsanitize=address,
    # -fsanitize-recover=address,
    # -O0 -g3
    ) #-Wconversion -Wshadow
add_link_options(
    # -fsanitize=thread
    # -fsanitize=address,
    # -fsanitize-recover=address,
)
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
find_package(Boost COMPONENTS program_options REQUIRED)
target_link_libraries(${PROJECT_NAME}
    ${Boost_PROGRAM_OPTIONS_LIBRARY}
)

target_link_libraries(${PROJECT_NAME} 
    Threads::Threads 
    "${CMAKE_SOURCE_DIR}/dependencies/cpp-bencoding/install/lib/libbencoding.a"
)

target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/dependencies/cpp-bencoding/install/include")
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_SOURCE_DIR}/src")
