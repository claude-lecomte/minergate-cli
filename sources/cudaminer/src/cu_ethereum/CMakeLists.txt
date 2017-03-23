find_package(CUDA REQUIRED)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

file(GLOB SOURCES *.cu)
file(GLOB HEADERS *.cuh *.h)

set(CUDA_PROPAGATE_HOST_FLAGS OFF)
set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS} "-gencode=arch=compute_30,code=\"compute_30,sm_30\" -gencode=arch=compute_35,code=\"compute_35,sm_35\" -gencode=arch=compute_50,code=\"compute_50,sm_50\" -gencode=arch=compute_52,code=\"compute_52,sm_52\"")

if (MSVC)
  set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS} --std=c++11)
else()
  set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS};--disable-warnings;--ptxas-options=-v;-use_fast_math;-lineinfo)
  if (${GCC_VER} EQUAL 4)
    set(CUDA_NVCC_FLAGS ${CUDA_NVCC_FLAGS} --std=c++11)
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_FORCE_INLINES")
  endif()
endif()

LIST(APPEND CUDA_NVCC_FLAGS_RELEASE -O3)
cuda_add_library(${CU_ETHEREUM_LIB} ${SOURCES} ${HEADERS})
