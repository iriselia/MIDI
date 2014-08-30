project( MIDIPLAYER )
cmake_minimum_required( VERSION 2.8 )

file(GLOB_RECURSE MY_SRC ${CMAKE_CURRENT_SOURCE_DIR}/src *.cpp)
create_source_group("Source Files" "${CMAKE_CURRENT_SOURCE_DIR}/src" ${MY_SRC})
file(GLOB_RECURSE MY_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
create_source_group("Header Files" "${CMAKE_CURRENT_SOURCE_DIR}/include" ${MY_HEADERS})

set (MY_INCLUDE_DIRS "")
foreach (_headerFile ${MY_HEADERS})
    get_filename_component(_dir ${_headerFile} PATH)
    list (APPEND MY_INCLUDE_DIRS ${_dir})
endforeach()
list(REMOVE_DUPLICATES MY_INCLUDE_DIRS)
include_directories( ${MY_INCLUDE_DIRS} )


set(MIDIPLAYER_INCLUDE_DIRS ${MY_INCLUDE_DIRS})
#------ additional includes ------
include_directories(
#
)

#------ link directories listed ------
#link_directories( ${HGE_SOURCE_DIR}/lib )
link_libraries( "Winmm.lib" )

#------ target -----
#add_executable (MIDIPLAYER WIN32 ${MIDIPLAYER_SOURCES} ${MIDIPLAYER_HEADERS})
add_executable (MIDIPLAYER WIN32 ${MY_SRC} ${MY_HEADERS})

if(WIN32)
   set_target_properties(MIDIPLAYER PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
   set_target_properties(MIDIPLAYER PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
   set_target_properties(MIDIPLAYER PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:CONSOLE")
   set_target_properties(MIDIPLAYER PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE")
   set_target_properties(MIDIPLAYER PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE")
   set_target_properties(MIDIPLAYER PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:CONSOLE")
endif(WIN32)

install (TARGETS MIDIPLAYER
         RUNTIME DESTINATION ${MIDIPLAYER_SOURCE_DIR}/bin)

if( MSVC )
    SET_TARGET_PROPERTIES( MIDIPLAYER PROPERTIES LINK_FLAGS "/NODEFAULTLIB:libci" )
endif()
