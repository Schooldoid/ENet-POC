cmake_minimum_required(VERSION 3.26)
project(Client)
project(Server)

add_executable(Client)
add_executable(Server)

file(GLOB_RECURSE SOURCES_CLIENT CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Client/*.cpp")
file(GLOB_RECURSE SOURCES_SERVER CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/Server/*.cpp")

target_sources(Client PRIVATE ${SOURCES_CLIENT})
target_sources(Server PRIVATE ${SOURCES_SERVER})

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(third-party/enet-1.3.17)

# target_include_directories(Client PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include/")

target_link_libraries(Client PRIVATE enet)
target_link_libraries(Server PRIVATE enet)