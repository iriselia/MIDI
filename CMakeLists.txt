## MIDI Project CMake script
##
project( MIDI )
cmake_minimum_required( VERSION 2.8 )

if(WIN32)
add_definitions( "-DPLATFORM_WINDOWS" )
endif(WIN32)

##----------------------------------------
## End of manual configuration section
## try to minimize tampering with code below unless you know how CMake works
##----------------------------------------

SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${MIDI_SOURCE_DIR}/bin CACHE PATH "Single Directory for all executables." )
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${MIDI_SOURCE_DIR}/bin CACHE PATH "Single Directory for all executables." )
SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${MIDI_SOURCE_DIR}/bin_debug CACHE PATH "Single Directory for all executables." )
SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${MIDI_SOURCE_DIR}/lib CACHE PATH "Single Directory for all static libraries." )
SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${MIDI_SOURCE_DIR}/lib CACHE PATH "Single Directory for all static libraries." )
SET( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${MIDI_SOURCE_DIR}/lib CACHE PATH "Single Directory for all static libraries." )
SET( CMAKE_PDB_OUTPUT_DIRECTORY ${MIDI_SOURCE_DIR}/bin_debug/pdb CACHE PATH "Single Directory for all static libraries." )
SET( CMAKE_INCLUDE_CURRENT_DIR ON )

    # create_source_group(relativeSourcePath sourceGroupName files)
    #
    # Creates a source group with the specified name relative to the relative path
    # specified.
    #
    # Parameters:
    #    - sourceGroupName: Name of the source group to create.
    #    - relativeSourcePath: Relative path to the files.
    #    - sourceFiles: Files to add to the source group.
    #
    # For example if you have the following directory structure:
    #
    #    - ExampleApplication
    #        - include
    #            - Main.h
    #                - Window
    #                    Window.h
    #        - source
    #            - Main.cpp
    #                - Window
    #                    Window.cpp
    #
    # You can get your list of files and call create_source_group the following way
    #
    #    file(GLOB_RECURSE my_source_files ${CMAKE_CURRENT_SOURCE_DIR}/source/*)
    #    create_source_group("Source Files"
	#	"${CMAKE_CURRENT_SOURCE_DIR}/source" ${my_source_files})
    #    file(GLOB_RECURSE my_header_files ${CMAKE_CURRENT_SOURCE_DIR}/include/*)
    #    create_source_group("Header Files"
	#	"${CMAKE_CURRENT_SOURCE_DIR}/include" ${my_header_files})
    #    add_executable(ExampleApplication ${my_source_files} ${my_header_files})
    #
    # Then the generated solution would look like this
    #
    #    - ExampleApplication (project)
    #        - Header Files
    #            - Main.h
    #                - Window
    #                    Window.h
    #        - Source Files
    #            - Main.cpp
    #                - Window
    #                    Window.cpp
    #
	
    function(create_source_group sourceGroupName relativeSourcePath sourceFiles)
	    FOREACH(currentSourceFile ${ARGN})
            FILE(RELATIVE_PATH folder ${relativeSourcePath} ${currentSourceFile})
            get_filename_component(filename ${folder} NAME)
            string(REPLACE ${filename} "" folder ${folder})
            if(NOT folder STREQUAL "")
                string(REGEX REPLACE "/+$" "" folderlast ${folder})
                string(REPLACE "/" "\\" folderlast ${folderlast})
                SOURCE_GROUP("${sourceGroupName}\\${folderlast}" FILES ${currentSourceFile})
            endif(NOT folder STREQUAL "")
        ENDFOREACH(currentSourceFile ${ARGN})
		
        FOREACH(currentSourceFile ${sourceFiles})
            FILE(RELATIVE_PATH folder ${relativeSourcePath} ${currentSourceFile})
            get_filename_component(filename ${folder} NAME)
            string(REPLACE ${filename} "" folder ${folder})
            if(NOT folder STREQUAL "")
                string(REGEX REPLACE "/+$" "" folderlast ${folder})
                string(REPLACE "/" "\\" folderlast ${folderlast})
                SOURCE_GROUP("${sourceGroupName}\\${folderlast}" FILES ${currentSourceFile})
            endif(NOT folder STREQUAL "")
        ENDFOREACH(currentSourceFile ${sourceFiles})
    endfunction(create_source_group)

#--- 3rd Party library ---
add_subdirectory( MIDIPlayer )
