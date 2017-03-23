project(${NETWORKUTILS_LIB})

find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)

set(CMAKE_AUTOMOC ON)

file(GLOB SOURCES src/*.cpp)
file(GLOB HEADERS src/networkutils/*.h)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${UTILS_SRC_DIR})
INCLUDE_DIRECTORIES(${MINER_CORE_SRC_DIR})
INCLUDE_DIRECTORIES(${QJSONRPC_SRC_DIR})

include_directories(${CN_SRC_DIR}/src)

add_definitions(-DNETWORKUTILS_EXPORT)

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} ${UTILS_LIB} ${QJSONRPC_LIB})

qt5_use_modules(${PROJECT_NAME} Core Network)
