##########################################################
cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
# updated CMakeLists file to have everything in one place
##########################################################

PROJECT( ZHHProcessors )

# project version
SET( ${PROJECT_NAME}_VERSION_MAJOR 0 )
SET( ${PROJECT_NAME}_VERSION_MINOR 1 )
SET( ${PROJECT_NAME}_VERSION_PATCH 0 )

### DEPENDENCIES ############################################################

FIND_PACKAGE( ILCUTIL REQUIRED COMPONENTS ILCSOFT_CMAKE_MODULES HINTS $ENV{ilcutil} )

# load default settings from ILCSOFT_CMAKE_MODULES
INCLUDE( ilcsoft_default_settings )

set(CMAKE_CXX_STANDARD 17 CACHE STRING "")

if(NOT CMAKE_CXX_STANDARD MATCHES "17|20")
  message(FATAL_ERROR "Unsupported C++ standard: ${CMAKE_CXX_STANDARD}")
endif()

# Prevent CMake falls back to the latest standard the compiler does support
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Disables the use of compiler-specific extensions, hence makes sure the code
# works for a wider range of compilers
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(LCIO 2.21 REQUIRED)
find_package(Marlin 1.17 REQUIRED) # use latest marlin
find_package(ROOT REQUIRED COMPONENTS RIO Tree Physics)
find_package(LCFIVertex REQUIRED )
find_package(MarlinUtil 1.0 REQUIRED ) # minimum required MarlinUtil version
find_package(AIDA REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(Physsim REQUIRED)

# for kinfits
find_package(MarlinKinfit REQUIRED)
find_package(CLHEP REQUIRED)
find_package(GSL 1.12 REQUIRED)

include(GNUInstallDirs)

# Collect all the source files from the subdirectories
set(sources
  AddNeutralPFOCovMat/src/AddNeutralPFOCovMat.cc
  ChargedPFOCorrection/src/ChargedPFOCorrection.cc
  CheatedMCOverlayRemoval/src/CheatedMCOverlayRemoval.cc
  ExpandJetProcessor/src/ExpandJetProcessor.cc
  FinalStateRecorder/src/FinalStateResolver.cc
  FinalStateRecorder/src/FinalStateRecorder.cc
  HdecayMode/src/HdecayMode.cc
  JetErrorAnalysis/src/JetErrorAnalysis.cc
  JetTaggingComparison/src/JetTaggingComparison.cc
  LeptonErrorAnalysis/src/LeptonErrorAnalysis.cc
  LeptonPairing/src/Utilities.cc
  LeptonPairing/src/ZHHUtilities.cc
  LeptonPairing/src/LeptonPairing.cc
  MergePIDProcessor/src/MergePIDProcessor.cc
  Misclustering/src/Misclustering.cc
  EventObservables/src/EventObservablesFromZZ.cc
  EventObservables/src/EventObservablesBase.cc
  EventObservables/src/EventObservablesLL.cc
  EventObservables/src/EventObservablesVV.cc
  EventObservables/src/EventObservablesQQ.cc
  TruthRecoComparison/src/TruthRecoComparison.cc
  ZHHKinfitProcessors/src/ZHHllbbbbKinFit.cc
  ZHHKinfitProcessors/src/ZHHqqbbbbKinFit.cc
  ZHHKinfitProcessors/src/ZHHvvbbbbKinFit.cc
  ZHHKinfitProcessors/src/ZHHllbbbbKinFitv1.cc
  ZHHKinfitProcessors/src/ZHHqqbbbbKinFitv1.cc
  ZHHKinfitProcessors/src/ZHHvvbbbbKinFitv1.cc
  ZHHKinfitProcessors/src/ZHHKinFit.cc
  ZinvisibleErrorAnalysis/src/ZinvisibleErrorAnalysis.cc
  PreSelection/src/PreSelection.cc
)

add_library(ZHHProcessors SHARED ${sources})
target_link_libraries(ZHHProcessors PRIVATE
  ${Marlin_LIBRARIES}
  ${MarlinUtil_LIBRARIES}
  ${AIDA_LIBRARIES}
  ${MarlinKinfit_LIBRARIES}
  ${CLHEP_LIBRARIES}
  ${GSL_LIBRARIES}
  Physsim::Physsim
  LCIO::lcio
  nlohmann_json::nlohmann_json
  ${ROOT_LIBRARIES}
)
target_include_directories(ZHHProcessors PRIVATE
  ${Marlin_INCLUDE_DIRS}
  ${MarlinUtil_INCLUDE_DIRS}
  ${AIDA_INCLUDE_DIRS}
  ${MarlinKinfit_INCLUDE_DIRS}
  ${LCFIVertex_INCLUDE_DIRS}
  ${CLHEP_LIBRARIES}
  ${GSL_INCLUDE_DIRS}
  ${ROOT_INCLUDE_DIRS}
  ${PHYSSIM_INCLUDE_DIRS}
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/CommonTools/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/AddNeutralPFOCovMat/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ChargedPFOCorrection/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/CheatedMCOverlayRemoval/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ExpandJetProcessor/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/FinalStateRecorder/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/HdecayMode/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/JetErrorAnalysis/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/JetTaggingComparison/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/LeptonErrorAnalysis/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/LeptonPairing/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/MergePIDProcessor/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Misclustering/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/EventObservables/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/TruthRecoComparison/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ZHHKinfitProcessors/include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ZinvisibleErrorAnalysis/include>
)
target_compile_definitions(ZHHProcessors ${MarlinUtil_DEFINITIONS} PRIVATE "MARLIN_USE_ROOT")
target_compile_definitions(ZHHProcessors ${MarlinUtil_DEFINITIONS} PRIVATE "MARLIN_USE_AIDA")

install(TARGETS ZHHProcessors
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT shlib
)
