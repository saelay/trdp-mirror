# CMakeLists.txt
#
# $Id: CMakeLists.txt $
#

project(trdp-wireshark-plugin C)

cmake_minimum_required(VERSION 2.6)  
set(CMAKE_BACKWARDS_COMPATIBILITY 2.6)  
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/../../cmake/modules)  
set(CMAKE_INSTALL_LIBDIR ~/.wireshark)

INCLUDE(UseMakeDissectorReg)  

set(GLIB2_MIN_VERSION 2.4.0)  

find_package(GLIB2)  
include_directories (${GLIB2_INCLUDE_DIRS})  

find_package(Wireshark)  
include_directories (${WIRESHARK_INCLUDE_DIRS})  

set(LINK_MODE_LIB SHARED)  
set(LINK_MODE_MODULE MODULE)  

set(DISSECTOR_SRC
	packet-trdp_spy.c
	parsebody.c
)

set(PLUGIN_FILES
	plugin.c
	${DISSECTOR_SRC}
)

set(CLEAN_FILES
	${PLUGIN_FILES}
)

if (WERROR)
	set_source_files_properties(
		${CLEAN_FILES}
		PROPERTIES
		COMPILE_FLAGS -Werror
	)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

register_dissector_files(plugin.c
	plugin
	${DISSECTOR_SRC}
)

add_library(trdp_spy ${LINK_MODE_MODULE}
	${PLUGIN_FILES}
)
set_target_properties(trdp_spy PROPERTIES PREFIX "")
set_target_properties(trdp_spy PROPERTIES LINK_FLAGS "${WS_LINK_FLAGS}")

target_link_libraries(trdp_spy epan)

install(TARGETS trdp_spy
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/@CPACK_PACKAGE_NAME@/plugins/${CPACK_PACKAGE_VERSION} NAMELINK_SKIP
	RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}/@CPACK_PACKAGE_NAME@/plugins/${CPACK_PACKAGE_VERSION}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/@CPACK_PACKAGE_NAME@/plugins/${CPACK_PACKAGE_VERSION}
)
