set(LIBRARY ethash)

if (CPPETHEREUM)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif ()

if (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set(CMAKE_BUILD_TYPE Release)
endif()

if (NOT MSVC)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
endif()

add_definitions(-DETHASH_PRINT_CRITICAL_OUTPUT)

set(FILES 	util.h
            io.c
            internal.c
            ethash.h
            endian.h
            compiler.h
            fnv.h
            data_sizes.h
      sha3_cryptopp.cpp
      sha3_cryptopp.h)

if (MSVC)
  list(APPEND FILES util_win32.c io_win32.c mmap_win32.c)
else()
  list(APPEND FILES io_posix.c)
endif()

include_directories( ${CRYPTOPP_SRC_DIR}/.. )

add_library(${LIBRARY} ${FILES})

TARGET_LINK_LIBRARIES(${LIBRARY} ${CRYPTOPP_LIB})
