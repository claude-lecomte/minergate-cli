project(${MINER_CLI_BIN})

find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)
set(CMAKE_AUTOMOC ON)

if(MSVC)
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /STACK:10485760")
endif()

INCLUDE_DIRECTORIES(${MINER_CORE_SRC_DIR})
INCLUDE_DIRECTORIES(${UTILS_SRC_DIR})
INCLUDE_DIRECTORIES(${NETWORKUTILS_SRC_DIR})
INCLUDE_DIRECTORIES(${MINER_ABSTRACT_SRC_DIR})
INCLUDE_DIRECTORIES(${MINER_MODELS_SRC_DIR})
include_directories(${CN_SRC_DIR}/src)
if(NOT WIN32)
  INCLUDE_DIRECTORIES(${CRASH_REPORTS_SRC_DIR}/handler/src)
else()
  INCLUDE_DIRECTORIES(${BREAKPAD_SRC_DIR})
endif()

if (ARCH_64bit)
  include_directories(${ETH_SRC_DIR})
  set (Boost_USE_STATIC_LIBS ON)
  set (Boost_MULTITHREADED ON)
  FIND_PACKAGE (Threads)

  find_package(Boost 1.55.0 REQUIRED COMPONENTS system thread random chrono date_time regex)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

if(NOT APPLE)
  find_package(CUDA REQUIRED)
  find_package(OpenCL 2.0)
  INCLUDE_DIRECTORIES(${MINER_IMPL_SRC_DIR})
  INCLUDE_DIRECTORIES(${OPENCLMINER_IMPL_SRC_DIR} ${CUDAMINER_IMPL_SRC_DIR} ${OpenCL_INCLUDE_DIRS} ${CUDA_INCLUDE_DIRS})
endif()

if(WIN32)
  set(WIN32_RESOURCE resources/minergate.rc)
endif(WIN32)

if(MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /FORCE:MULTIPLE /MAP")
endif(MSVC)

if(APPLE)
  set(CMAKE_MACOSX_RPATH 1)
endif()

if (APPLE OR NOT ARCH_64bit)
  file(GLOB SOURCES src/main.cpp)
else()
  file(GLOB SOURCES src/*.cpp)
  file(GLOB HEADERS src/*.h)
endif()

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} ${MINER_CORE_LIB} ${BCRYPT_LIB} ${UTILS_LIB} ${NETWORKUTILS_LIB}
  ${MINER_ABSTRACT_LIB} ${MINER_IMPL_LIB} ${ETH_UTILS_LIB})

target_link_libraries ( ${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT} )

if (ARCH_64bit)
  target_link_libraries(${PROJECT_NAME} ${ETH_UTILS_LIB})
  if (NOT WIN32)
  target_link_libraries(${PROJECT_NAME} pthread)
  endif()
endif()

if (NOT APPLE)
  target_link_libraries(${PROJECT_NAME} ${CUDA_MINER_LIB} ${OCL_MINER_LIB})
endif()

if(APPLE)
  set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "@executable_path/Frameworks" BUILD_WITH_INSTALL_RPATH 1)
  set_target_properties(${PROJECT_NAME} PROPERTIES SKIP_BUILD_RPATH ON)
elseif(NOT WIN32 AND NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "\$ORIGIN/lib" BUILD_WITH_INSTALL_RPATH 1)
endif(APPLE)

if(MSVC)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /FORCE:MULTIPLE /STACK:10485760 /MAP")
elseif(NOT APPLE AND NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_RPATH}:\$ORIGIN/lib")
endif(MSVC)

qt5_use_modules(${PROJECT_NAME} Core Network)

# Install
if (NOT WIN32 AND NOT APPLE)
  find_program(RPMBUILD rpmbuild)
  if(NOT RPMBUILD)
    set(PACKAGE_SYSTEM deb)
  else()
    set(PACKAGE_SYSTEM rpm)
  endif()

  set(VER_NAME "${MINER_MAJOR_VERSION}.${MINER_MINOR_VERSION}-gcc${GCC_VER}")
#  if (${GCC_VER} EQUAL 4)
#    set(CPACK_PACKAGE_FILE_NAME "minergate-cli-legacy")
#  else()
    set(CPACK_PACKAGE_FILE_NAME "minergate-cli")
#  endif()
else()
  set(VER_NAME "${MINER_MAJOR_VERSION}.${MINER_MINOR_VERSION}")
endif()

file(GLOB TRANSLATIONS ${CMAKE_SOURCE_DIR}/translations/*.qm)
set(CPACK_PACKAGE_NAME "MinerGate-cli")
set(CPACK_PACKAGE_VENDOR "Minergate Inc")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Miner")
set(CPACK_PACKAGE_VERSION_MAJOR ${MINER_MAJOR_VERSION})
set(CPACK_PACKAGE_VERSION_MINOR ${MINER_MINOR_VERSION})
set(CPACK_PACKAGE_VERSION ${VER_NAME})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "MinerGate-cli")
set(CPACK_PACKAGE_EXECUTABLES "minergate-cli" "MinerGate-cli")
set(CPACK_MONOLITHIC_INSTALL 1)
set(CPACK_CREATE_DESKTOP_LINKS "minergate")


if ((${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo") AND (NOT ${SKIP_DEBUG_COPY}))
  #coping binary into folder with debug version
  set(OUT_DEBUG_BIN_DIR ${BIN_DEBUG_DIR}/${CMAKE_SYSTEM_NAME}/${PACKAGE_SYSTEM}/${VER_NAME}-cli)
  if (NOT ARCH_64bit)
    set(OUT_DEBUG_BIN_DIR ${OUT_DEBUG_BIN_DIR}_32)
  endif()
  file(MAKE_DIRECTORY ${OUT_DEBUG_BIN_DIR})

  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> ${OUT_DEBUG_BIN_DIR})
endif()

if(WIN32)
  install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION .)
  file(GLOB_RECURSE DLLS_BASE ${WINDEPS_DIR}/dlls/*.dll)
  #install(FILES ${DLLS} DESTINATION .)
  install(FILES ${DLLS_BASE} DESTINATION .)

  set(CPACK_INSTALLED_DIRECTORIES ${WINDEPS_DIR}/dirs/platforms ./platforms)

  set(CPACK_GENERATOR ZIP)

elseif(APPLE)
    message("not an installer for mac")
  #            set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/${PROJECT_NAME})
#    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_BINARY_DIR})

#    install(FILES ${MACDEPS_DIR}/platforms/libqcocoa.dylib DESTINATION lib/platforms
#      PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)



  #            install(CODE "
  #                SET(BU_CHMOD_BUNDLE_ITEMS ON)
  #                include(BundleUtilities)
  #                message(\"CMIP: \" \${CMAKE_INSTALL_PREFIX})
  #                fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}\" \"\" \"$ENV{QTDIR}/lib\")
  #                VERIFY_APP(\"\${CMAKE_INSTALL_PREFIX}/bin/${PROJECT_NAME}\")
  #            ")

  #            set(CPACK_GENERATOR Bundle)

  #           set(CPACK_BUNDLE_ICON ${CMAKE_SOURCE_DIR}/images/minergate.icns)
  #           set(CPACK_BUNDLE_NAME MinerGate)
  #           set(CPACK_BUNDLE_PLIST ${CMAKE_BINARY_DIR}/Info.plist)
  #           set(CPACK_BUNDLE_STARTUP_COMMAND ${MACDEPS_DIR}/start)
  #           configure_file(${MACDEPS_DIR}/Info.plist.in ${CMAKE_BINARY_DIR}/Info.plist)
else()
  set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/minergate-cli")
  set(CPACK_INSTALLED_DIRECTORIES ${LINDEPS_DIR}/${PACKAGE_SYSTEM}${GCC_VER}/lib;${CPACK_PACKAGING_INSTALL_PREFIX}/lib;${LINDEPS_DIR}/${PACKAGE_SYSTEM}${GCC_VER}/platforms;${CPACK_PACKAGING_INSTALL_PREFIX}/platforms)
  install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION .)
  install(FILES ${LINDEPS_DIR}/minergate-cli DESTINATION /usr/bin)

  if(NOT RPMBUILD)
    set(CPACK_GENERATOR DEB)
    set(CPACK_SYSTEM_NAME amd64)
    set(CPACK_PACKAGE_CONTACT "https://minergate.com")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR} <support@minergate.com>")
    set(CPACK_DEBIAN_PACKAGE_SECTION Office)
    set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "Mine CryptoNote based and other popular cryptocurrencies with the easiest miner ever.
      User-friendly miner supports Ethereum, Bytecoin, Monero, Quazarcoin, DarkNote, Fantomcoin, MonetaVerde, Aeon coin, Dashcoin and Infinium-8.
      Thank you for mining with us!")
#    if (GCC_VER EQUAL 4)
      set(CPACK_DEBIAN_PACKAGE_DEPENDS "libxcb1,libx11-xcb1,libxcb-keysyms1,libxcb-image0,libxcb-shm0,libxcb-icccm4,libxcb-sync1,libxcb-render-util0,libpcre3")
#    else()
#      set(CPACK_DEBIAN_PACKAGE_DEPENDS "libxcb1 (>= 1.5), libpcre3")
#    endif()
  else()
    set(CPACK_GENERATOR RPM)
    set(CPACK_SYSTEM_NAME x86_64)
    set(CPACK_PACKAGE_CONTACT "https://minergate.com")
    set(CPACK_RPM_PACKAGE_VENDOR "${CPACK_PACKAGE_VENDOR} <support@minergate.com>")
    set(CPACK_RPM_PACKAGE_GROUP Office)
    set(CPACK_RPM_PACKAGE_SUMMARY "Mine CryptoNote based and other popular cryptocurrencies with the easiest miner ever.")
    set(CPACK_RPM_PACKAGE_DESCRIPTION "User-friendly miner supports Ethereum, Bytecoin, Monero, Quazarcoin, Litecoin, DarkNote, Fantomcoin,
      MonetaVerde, Aeon coin, Dashcoin and Infinium-8. Thank you for mining with us!")
    set(CPACK_RPM_PACKAGE_DEPENDS "libxcb,libXrender,xcb-util-wm,xcb-util,xcb-util-image,xcb-util-keysyms")
  endif()
endif(WIN32)

include(CPack)
