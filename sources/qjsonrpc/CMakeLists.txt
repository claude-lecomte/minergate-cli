project(${QJSONRPC_LIB})

find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)

set(CMAKE_AUTOMOC ON)

file(GLOB SOURCES src/*.cpp)
file(GLOB HEADERS src/qjsonrpc/*.h)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

add_definitions(-DQJSONRPC_EXPORT)

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

qt5_use_modules(${PROJECT_NAME} Core Network)
