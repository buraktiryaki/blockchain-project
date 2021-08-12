cmake_minimum_required (VERSION 3.16)
project(gpuden)

find_package(Threads REQUIRED)
find_package(OpenCL REQUIRED)

message(${OpenCL_LIBRARIES})
message(${OpenCL_INCLUDE_DIRS})

if (UNIX)
    set(LINK OpenCL::OpenCL Threads::Threads ${CMAKE_DL_LIBS})
endif (UNIX)
if(WIN32)
    set(LINK OpenCL::OpenCL)
endif(WIN32)

add_executable(gpuden main.cpp)
target_link_libraries(gpuden ${LINK})

