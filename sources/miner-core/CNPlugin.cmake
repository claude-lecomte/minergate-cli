
if(MSVC)
	add_definitions("/DRUNTIME_AES /D__AES__ /D_WIN32_WINNT=0x0600")
	foreach(VAR CMAKE_C_FLAGS_DEBUG CMAKE_CXX_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELEASE CMAKE_C_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_MINSIZEREL)
#			string(REPLACE "/MD" "/MT" ${VAR} "${${VAR}}")
	endforeach()
endif(MSVC)


