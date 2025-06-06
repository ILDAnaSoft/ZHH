########################################################
# cmake file for building Marlin example Package
# @author Jan Engels, Desy IT
CMAKE_MINIMUM_REQUIRED(VERSION 3.20 FATAL_ERROR)
########################################################


# project name
PROJECT( ZHHKinfitProcessors )


# project version
SET( ${PROJECT_NAME}_VERSION_MAJOR 0 )
SET( ${PROJECT_NAME}_VERSION_MINOR 4 )
SET( ${PROJECT_NAME}_VERSION_PATCH 2 )



### DEPENDENCIES ############################################################

FIND_PACKAGE( ILCUTIL REQUIRED COMPONENTS ILCSOFT_CMAKE_MODULES )

# load default settings from ILCSOFT_CMAKE_MODULES
INCLUDE( ilcsoft_default_settings )


FIND_PACKAGE( Marlin 1.0 REQUIRED ) # minimum required Marlin version
INCLUDE_DIRECTORIES(SYSTEM ${Marlin_INCLUDE_DIRS} )
LINK_LIBRARIES( ${Marlin_LIBRARIES} )
ADD_DEFINITIONS( ${Marlin_DEFINITIONS} )

FIND_PACKAGE( MarlinKinfit REQUIRED )
INCLUDE_DIRECTORIES(SYSTEM ${MarlinKinfit_INCLUDE_DIRS} )
LINK_LIBRARIES( ${MarlinKinfit_LIBRARIES} )

FIND_PACKAGE( CLHEP REQUIRED )
INCLUDE_DIRECTORIES(SYSTEM ${CLHEP_INCLUDE_DIRS} )
LINK_LIBRARIES( ${CLHEP_LIBRARIES} )

FIND_PACKAGE( GSL 1.12 REQUIRED )
INCLUDE_DIRECTORIES(SYSTEM ${GSL_INCLUDE_DIRS} )
LINK_LIBRARIES( ${GSL_LIBRARIES} )

FIND_PACKAGE( MarlinUtil REQUIRED )
INCLUDE_DIRECTORIES(SYSTEM ${MarlinUtil_INCLUDE_DIRS} )
LINK_LIBRARIES( ${MarlinUtil_LIBRARIES} )

FIND_PACKAGE( ROOT 5.0 )
IF( ROOT_FOUND )
    INCLUDE_DIRECTORIES(SYSTEM ${ROOT_INCLUDE_DIRS} )
    LINK_LIBRARIES( ${ROOT_LIBRARIES} )
    ADD_DEFINITIONS( -DMARLIN_USE_ROOT )
    MESSAGE( STATUS "ROOT -- found" )
ELSE()
    MESSAGE( STATUS "ROOT -- not found" )
ENDIF()

FIND_PACKAGE( AIDA )
IF( AIDA_FOUND )
    INCLUDE_DIRECTORIES(SYSTEM ${AIDA_INCLUDE_DIRS} )
    LINK_LIBRARIES( ${AIDA_LIBRARIES} )
    ADD_DEFINITIONS( -DMARLIN_USE_AIDA )
    MESSAGE( STATUS "AIDA -- found" )
ELSE()
    MESSAGE( STATUS "AIDA -- not found" )
ENDIF()


### DOCUMENTATION ###########################################################

OPTION( INSTALL_DOC "Set to OFF to skip build/install Documentation" OFF )

IF( INSTALL_DOC AND EXISTS "${PROJECT_SOURCE_DIR}/doc/CMakeLists.txt" )
    ADD_SUBDIRECTORY( ./doc )
ENDIF()



### LIBRARY #################################################################

# definitions to pass to the compiler
#ADD_DEFINITIONS( "-Wall -ansi -pedantic" )
#ADD_DEFINITIONS( "-Wno-long-long" )

# include directories
INCLUDE_DIRECTORIES( ./include )
INSTALL_DIRECTORY( ./include DESTINATION . FILES_MATCHING PATTERN "*.h" )

# add library
AUX_SOURCE_DIRECTORY( ./src library_sources )
ADD_SHARED_LIBRARY( ${PROJECT_NAME} ${library_sources} )
INSTALL_SHARED_LIBRARY( ${PROJECT_NAME} DESTINATION lib )

# display some variables and write them to cache
DISPLAY_STD_VARIABLES()

# generate and install following configuration files
GENERATE_PACKAGE_CONFIGURATION_FILES( ZHHKinfitProcessorsConfig.cmake ZHHKinfitProcessorsConfigVersion.cmake ZHHKinfitProcessorsLibDeps.cmake )

