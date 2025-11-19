########################################################
# cmake file for building Marlin example Package
# @author Jan Engels, Desy IT
CMAKE_MINIMUM_REQUIRED(VERSION 2.6 FATAL_ERROR)
########################################################


# project name
PROJECT( lcio_get_physics_meta )


# project version
SET( ${PROJECT_NAME}_VERSION_MAJOR 0 )
SET( ${PROJECT_NAME}_VERSION_MINOR 1 )
SET( ${PROJECT_NAME}_VERSION_PATCH 0 )



### DEPENDENCIES ############################################################

FIND_PACKAGE( ILCUTIL REQUIRED COMPONENTS ILCSOFT_CMAKE_MODULES )

FIND_PACKAGE( LCIO REQUIRED )
INCLUDE_DIRECTORIES( SYSTEM ${LCIO_INCLUDE_DIRS} )
LINK_LIBRARIES( ${LCIO_LIBRARIES} )
ADD_DEFINITIONS( ${LCIO_DEFINITIONS} )

# load default settings from ILCSOFT_CMAKE_MODULES
INCLUDE( ilcsoft_default_settings )

#FIND_PACKAGE( ROOT REQUIRED )
#INCLUDE_DIRECTORIES( SYSTEM ${ROOT_INCLUDE_DIRS} )
#LINK_LIBRARIES( ${ROOT_LIBRARIES} )
#ADD_DEFINITIONS( ${ROOT_DEFINITIONS} )

### DOCUMENTATION ###########################################################

OPTION( INSTALL_DOC "Set to OFF to skip build/install Documentation" OFF )

IF( INSTALL_DOC AND EXISTS "${PROJECT_SOURCE_DIR}/doc/CMakeLists.txt" )
    ADD_SUBDIRECTORY( ./doc )
ENDIF()



### LIBRARY #################################################################

# definitions to pass to the compiler
# ADD_DEFINITIONS( "-Wall -ansi -pedantic" )
# ADD_DEFINITIONS( "-Wno-long-long" )

# include directories
#INCLUDE_DIRECTORIES( ./include )
#INSTALL_DIRECTORY( ./include DESTINATION . FILES_MATCHING PATTERN "*.h" )

# add executable
#AUX_SOURCE_DIRECTORY( ./src library_sources )

ADD_EXECUTABLE( lcio_get_physics_meta ./src/lcio_get_physics_meta.cc )
TARGET_LINK_LIBRARIES( lcio_get_physics_meta PRIVATE LCIO::lcio )
INSTALL( TARGETS lcio_get_physics_meta DESTINATION "${CMAKE_INSTALL_BINDIR}" )

#ADD_SHARED_LIBRARY( ${PROJECT_NAME} ${library_sources} )
#INSTALL_SHARED_LIBRARY( ${PROJECT_NAME} DESTINATION lib )

# display some variables and write them to cache
DISPLAY_STD_VARIABLES()
