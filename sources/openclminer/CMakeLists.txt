project(${OCL_MINER_LIB})

set(CMAKE_AUTOMOC ON)
find_package(Qt5Core REQUIRED)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/src)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
INCLUDE_DIRECTORIES(${MINER_ABSTRACT_SRC_DIR})
INCLUDE_DIRECTORIES(${MINER_IMPL_SRC_DIR})
INCLUDE_DIRECTORIES(${UTILS_SRC_DIR})
INCLUDE_DIRECTORIES(${NETWORKUTILS_SRC_DIR})

find_package(OpenCL 2.0)

INCLUDE_DIRECTORIES(${OPENCL_INCLUDE_DIRS})
INCLUDE_DIRECTORIES(${CRYPTO_SRC_DIR} ${CN_SRC_DIR}/src)
INCLUDE_DIRECTORIES(${OpenCL_INCLUDE_DIRS})

add_custom_command (
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/cl_cryptonight.h
  COMMAND ${CMAKE_COMMAND} ARGS
  -DBIN2H_SOURCE_FILE="${CMAKE_CURRENT_SOURCE_DIR}/src/cryptonote/cryptonight.cl"
  -DBIN2H_VARIABLE_NAME=cl_cryptonight
  -DBIN2H_HEADER_FILE="${CMAKE_CURRENT_BINARY_DIR}/cl_cryptonight.h"
  -P "${CMAKE_CURRENT_SOURCE_DIR}/bin2h.cmake"
  COMMENT "Generating OpenCL Cryptonight implementation"
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/cryptonote/cryptonight.cl
)
add_custom_target(clbin2h_cryptonight ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/cl_cryptonight.h ${CMAKE_CURRENT_SOURCE_DIR}/src/cryptonote/cryptonight.cl)

if (ARCH_64bit)
  # A custom command and target to turn the OpenCL kernel into a byte array header
  # The normal build depends on it properly and if the kernel file is changed, then
  # a rebuild of libethash-cl should be triggered
  add_custom_command (
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/ethash_cl_miner_kernel.h
    COMMAND ${CMAKE_COMMAND} ARGS
    -DBIN2H_SOURCE_FILE="${CMAKE_CURRENT_SOURCE_DIR}/src/ethereum/ethash_cl_miner_kernel.cl"
    -DBIN2H_VARIABLE_NAME=ethash_cl_miner_kernel
    -DBIN2H_HEADER_FILE="${CMAKE_CURRENT_BINARY_DIR}/ethash_cl_miner_kernel.h"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/bin2h.cmake"
    COMMENT "Generating OpenCL Ethereum implementation"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/ethereum/ethash_cl_miner_kernel.cl
  )
  add_custom_target(clbin2h_eth ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/ethash_cl_miner_kernel.h ${CMAKE_CURRENT_SOURCE_DIR}/src/ethereum/ethash_cl_miner_kernel.cl)

  set (Boost_USE_STATIC_LIBS ON)
  set (Boost_MULTITHREADED ON)
  set (Boost_USE_MULTITHREADED ON)
  find_package(Boost 1.55.0 REQUIRED COMPONENTS system thread random chrono date_time regex)
  include_directories(${Boost_INCLUDE_DIRS})
  INCLUDE_DIRECTORIES(${ETH_SRC_DIR})

  file(GLOB SOURCES src/*.cpp src/cryptonote/*.cpp src/ethereum/*.cpp)
  file(GLOB HEADERS src/openclminer/*.h src/cryptonote/*.h src/ethereum/*.h ${CMAKE_CURRENT_BINARY_DIR}/ethash_cl_miner_kernel.h ${CMAKE_CURRENT_BINARY_DIR}/cl_cryptonight.h)
else()
  file(GLOB SOURCES src/*.cpp src/cryptonote/*.cpp)
  file(GLOB HEADERS src/openclminer/*.h src/cryptonote/*.h ${CMAKE_CURRENT_BINARY_DIR}/cl_cryptonight.h)
endif()

add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})
target_link_libraries(${PROJECT_NAME} ${OpenCL_LIBRARIES} ${MINER_ABSTRACT_LIB} ${UTILS_LIB} ${NETWORKUTILS_LIB})

if (ARCH_64bit)
  target_link_libraries(${PROJECT_NAME} ${ETH_ETHASH_LIB} ${ETH_ETHCORE_LIB} ${ETH_UTILS_LIB})
  target_link_libraries(${PROJECT_NAME} ${Boost_THREAD_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${Boost_SYSTEM_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${Boost_RANDOM_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${Boost_CHRONO_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${Boost_DATE_TIME_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${Boost_REGEX_LIBRARIES})

  target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
endif()

qt5_use_modules(${PROJECT_NAME} Core)
