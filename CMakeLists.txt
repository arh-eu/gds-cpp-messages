cmake_minimum_required(VERSION 3.10)

project(gds_lib)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

file(GLOB SOURCES "src/gds_types.cpp" "src/gds_connection.cpp")
file(GLOB HEADERS "src/gds_connection.hpp" "src/gds_types.hpp" "src/semaphore.hpp" "src/gds_uuid.hpp")

add_library(gds STATIC ${SOURCES})

install(FILES ${HEADERS} DESTINATION ${PROJECT_SOURCE_DIR}/output/include/)
install(TARGETS gds DESTINATION ${PROJECT_SOURCE_DIR}/output/lib/)
