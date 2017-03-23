if(NOT CRYPTO_LIB)
	set(CRYPTO_LIB crypto)
endif()

file(GLOB_RECURSE CRYPTO crypto/*)
source_group(${CRYPTO_LIB} FILES ${CRYPTO})
add_library(${CRYPTO_LIB} STATIC ${CRYPTO})

