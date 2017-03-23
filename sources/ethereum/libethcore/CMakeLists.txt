cmake_policy(SET CMP0015 NEW)
#set(CMAKE_AUTOMOC ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTATICLIB")

find_package (Qt5Core REQUIRED)

set (Boost_USE_STATIC_LIBS ON)
set (Boost_MULTITHREADED ON)
find_package(Boost 1.55.0 REQUIRED COMPONENTS system thread random chrono date_time regex)

aux_source_directory(. SRC_LIST)

include_directories(BEFORE ..)
include_directories(${Boost_INCLUDE_DIRS})

if (CPUID_FOUND)
    include_directories(${Cpuid_INCLUDE_DIRS})
endif ()

file(GLOB HEADERS "*.h")

add_library(${ETH_ETHCORE_LIB} ${SRC_LIST} ${HEADERS})

target_link_libraries(${ETH_ETHCORE_LIB} ethash)
#add_definitions(-DBOOST_ALL_DYN_LINK)
target_link_libraries(${ETH_ETHCORE_LIB} ${Boost_THREAD_LIBRARIES})
target_link_libraries(${ETH_ETHCORE_LIB} ${Boost_SYSTEM_LIBRARIES})
target_link_libraries(${ETH_ETHCORE_LIB} ${Boost_RANDOM_LIBRARIES})
target_link_libraries(${ETH_ETHCORE_LIB} ${Boost_CHRONO_LIBRARIES})
target_link_libraries(${ETH_ETHCORE_LIB} ${Boost_DATE_TIME_LIBRARIES})
target_link_libraries(${ETH_ETHCORE_LIB} ${Boost_REGEX_LIBRARIES})

qt5_use_modules(${ETH_ETHCORE_LIB} Core)
