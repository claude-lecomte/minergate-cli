cmake_policy(SET CMP0015 NEW)

find_package (Qt5Core REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTATICLIB")

aux_source_directory(. SRC_LIST)

include_directories(BEFORE ..)

file(GLOB HEADERS "*.h")

set (Boost_USE_STATIC_LIBS ON)
set (Boost_MULTITHREADED ON)
find_package(Boost 1.55.0 REQUIRED COMPONENTS system thread random chrono date_time regex)
include_directories(${Boost_INCLUDE_DIRS})

add_library(${ETH_DEVCORE_LIB} ${SRC_LIST} ${HEADERS})
target_link_libraries(${ETH_DEVCORE_LIB} ${Boost_THREAD_LIBRARIES})
target_link_libraries(${ETH_DEVCORE_LIB} ${Boost_SYSTEM_LIBRARIES})
target_link_libraries(${ETH_DEVCORE_LIB} ${Boost_RANDOM_LIBRARIES})
target_link_libraries(${ETH_DEVCORE_LIB} ${Boost_CHRONO_LIBRARIES})
target_link_libraries(${ETH_DEVCORE_LIB} ${Boost_DATE_TIME_LIBRARIES})
target_link_libraries(${ETH_DEVCORE_LIB} ${Boost_REGEX_LIBRARIES})

if (APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(${ETH_DEVCORE_LIB} ${CMAKE_THREAD_LIBS_INIT})
elseif (UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(${ETH_DEVCORE_LIB} ${CMAKE_THREAD_LIBS_INIT})
endif()

qt5_use_modules(${ETH_DEVCORE_LIB} Core)
