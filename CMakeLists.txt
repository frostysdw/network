cmake_minimum_required(VERSION 3.5.0)
project(networkio VERSION 0.1.0 LANGUAGES C CXX)

add_executable(io io/io.cpp)
add_executable(select io/select.cpp)
add_executable(poll io/poll.cpp)
add_executable(epoll io/epoll.cpp)

add_executable(reactor reactor.cpp)


