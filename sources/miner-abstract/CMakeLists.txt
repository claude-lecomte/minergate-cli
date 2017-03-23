project(${MINER_ABSTRACT_LIB})

find_package(Qt5Core REQUIRED)

set(CMAKE_AUTOMOC ON)

file(GLOB SOURCES src/*.cpp)
file(GLOB HEADERS src/miner-abstract/*.h)

if (APPLE)
  list(REMOVE_ITEM SOURCES ${MINER_ABSTRACT_SRC_DIR}/abstractgpumanager.cpp)
  list(REMOVE_ITEM SOURCES ${MINER_ABSTRACT_SRC_DIR}/abstractgpuminer.cpp)
  list(REMOVE_ITEM SOURCES ${MINER_ABSTRACT_SRC_DIR}/gpuminerworker.cpp)

  list(REMOVE_ITEM HEADERS ${MINER_ABSTRACT_SRC_DIR}/miner-abstract/abstractgpumanager.h)
  list(REMOVE_ITEM HEADERS ${MINER_ABSTRACT_SRC_DIR}/miner-abstract/abstractgpumanager_p.h)
  list(REMOVE_ITEM HEADERS ${MINER_ABSTRACT_SRC_DIR}/miner-abstract/abstractgpuminer.h)
  list(REMOVE_ITEM HEADERS ${MINER_ABSTRACT_SRC_DIR}/miner-abstract/abstractgpuminer_p.h)
  list(REMOVE_ITEM HEADERS ${MINER_ABSTRACT_SRC_DIR}/miner-abstract/gpuminerworker.h)
endif()

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR} ${MINER_ABSTRACT_SRC_DIR} ${UTILS_SRC_DIR})

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

add_definitions(-DMINER_ABSTRACT_EXPORT)
target_link_libraries(${PROJECT_NAME} ${UTILS_LIB})

qt5_use_modules(${PROJECT_NAME} Core)
