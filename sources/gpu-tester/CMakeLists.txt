project(${GPU_TESTER_BIN})

find_package(Qt5Core REQUIRED)
set(CMAKE_AUTOMOC ON)

set(TESTER_MAJOR_VERSION "1")
set(TESTER_MINOR_VERSION "0")

add_definitions(-DTESTER_MAJOR_VERSION=\"${TESTER_MAJOR_VERSION}\")
add_definitions(-DTESTER_MINOR_VERSION=\"${TESTER_MINOR_VERSION}\")

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR} ${CUDAMINER_IMPL_SRC_DIR} ${OPENCLMINER_IMPL_SRC_DIR}
  ${MINER_ABSTRACT_SRC_DIR} ${CRYPTO_SRC_DIR} ${UTILS_SRC_DIR} ${CUDA})

if (ARCH_64bit)
  include_directories(${ETH_SRC_DIR})
  set (Boost_USE_STATIC_LIBS ON)
  set (Boost_MULTITHREADED ON)
  find_package(Boost 1.55.0 REQUIRED COMPONENTS system thread random chrono date_time regex)
  include_directories(${Boost_INCLUDE_DIRS})
  find_package (Threads)
endif()

find_package(CUDA REQUIRED)
find_package(OpenCL 2.0)
include_directories(${OpenCL_INCLUDE_DIRS} ${CUDA_INCLUDE_DIRS})

if(MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /FORCE:MULTIPLE /MAP")
endif(MSVC)

file(GLOB SOURCES src/*.cpp src/*.h)

add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(${PROJECT_NAME} ${CUDA_MINER_LIB} ${OCL_MINER_LIB} ${CRYPTO_LIB} ${ETH_UTILS_LIB})

if (ARCH_64bit)
  if (NOT WIN32)
    target_link_libraries(${PROJECT_NAME} pthread)
  endif()
  target_link_libraries(${PROJECT_NAME} ${ETH_UTILS_LIB})
  target_link_libraries(${PROJECT_NAME} ${Boost_THREAD_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${Boost_SYSTEM_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${Boost_RANDOM_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${Boost_CHRONO_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${Boost_DATE_TIME_LIBRARIES})
  target_link_libraries(${PROJECT_NAME} ${Boost_REGEX_LIBRARIES})

  target_link_libraries ( ${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} )
endif()

if (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "\$ORIGIN/lib" BUILD_WITH_INSTALL_RPATH 1)
endif()

if(MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /FORCE:MULTIPLE /STACK:10485760 /MAP")
elseif(NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:\$ORIGIN/lib")
endif(MSVC)

qt5_use_modules(${PROJECT_NAME} Core)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION .)
