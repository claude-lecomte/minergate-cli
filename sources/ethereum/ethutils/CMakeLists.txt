find_package (Qt5Core ${ETH_QT_VERSION} REQUIRED)
set(CMAKE_AUTOMOC ON)

file(GLOB SOURCES "*.cpp")
file(GLOB HEADERS "*.h")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSTATICLIB")

set (Boost_USE_STATIC_LIBS ON)
set (Boost_MULTITHREADED ON)
find_package(Boost 1.55.0 REQUIRED COMPONENTS system thread random chrono date_time regex)

add_library(${ETH_UTILS_LIB} ${SOURCES} ${HEADERS})
include_directories(${ETH_UTILS_LIB} ${MINER_ABSTRACT_SRC_DIR} ${ETH_SRC_DIR} ${UTILS_SRC_DIR} ${Boost_INCLUDE_DIRS})

target_link_libraries(${ETH_UTILS_LIB} ${ETH_DEVCORE_LIB} ${UTILS_LIB})
#add_definitions(-DBOOST_ALL_DYN_LINK)
target_link_libraries(${ETH_UTILS_LIB} ${Boost_THREAD_LIBRARIES})
target_link_libraries(${ETH_UTILS_LIB} ${Boost_SYSTEM_LIBRARIES})
target_link_libraries(${ETH_UTILS_LIB} ${Boost_RANDOM_LIBRARIES})
target_link_libraries(${ETH_UTILS_LIB} ${Boost_CHRONO_LIBRARIES})
target_link_libraries(${ETH_UTILS_LIB} ${Boost_DATE_TIME_LIBRARIES})
target_link_libraries(${ETH_UTILS_LIB} ${Boost_REGEX_LIBRARIES})

qt5_use_modules(${ETH_UTILS_LIB} Core)
