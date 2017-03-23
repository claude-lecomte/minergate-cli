project(${MINER_IMPL_LIB})

find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)

set(CMAKE_AUTOMOC ON)

if (ARCH_64bit)
  file(GLOB_RECURSE SOURCES src/*.cpp)
  file(GLOB_RECURSE HEADERS src/miner-impl/*.h)
  set (Boost_USE_STATIC_LIBS ON)
  set (Boost_MULTITHREADED ON)
  find_package(Boost 1.55.0 REQUIRED COMPONENTS system thread random chrono date_time regex)
else()
  file(GLOB SOURCES src/*.cpp src/cryptonote/*.cpp)
  file(GLOB HEADERS src/miner-impl/*.h src/miner-impl/cryptonote/*.h)
endif()


INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${MINER_IMPL_SRC_DIR} ${UTILS_SRC_DIR} ${NETWORKUTILS_SRC_DIR} ${CRYPTO_SRC_DIR} ${CRYPTO_LITE_SRC_DIR} ${QJSONRPC_SRC_DIR}
  ${CN_SRC_DIR}/src ${MINER_ABSTRACT_SRC_DIR} )
if (ARCH_64bit)
  INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIRS} ${ETH_SRC_DIR})
endif()

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} ${UTILS_LIB} ${NETWORKUTILS_LIB} ${CRYPTO_LIB} ${CRYPTO_LITE_LIB}
  ${MINER_ABSTRACT_LIB} )

if (ARCH_64bit)
  target_link_libraries(${PROJECT_NAME} ${ETH_ETHCORE_LIB} ${ETH_ETHASH_LIB} ${ETH_DEVCORE_LIB} ${ETH_UTILS_LIB})
  target_link_libraries(${PROJECT_NAME} ${Boost_THREAD_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${Boost_SYSTEM_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${Boost_RANDOM_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${Boost_CHRONO_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${Boost_DATE_TIME_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${Boost_REGEX_LIBRARIES})
endif()

qt5_use_modules(${PROJECT_NAME} Core Network)
