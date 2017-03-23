if(NOT CRYPTO_LITE_LIB)
	set(CRYPTO_LITE_LIB crypto-lite)
endif()

file(GLOB_RECURSE CRYPTO_LITE crypto/*)
source_group(${CRYPTO_LITE_LIB} FILES ${CRYPTO_LITE})
add_library(${CRYPTO_LITE_LIB} STATIC ${CRYPTO_LITE})

