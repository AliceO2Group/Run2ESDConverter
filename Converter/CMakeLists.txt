# Copyright CERN and copyright holders of ALICE O2. This software is
# distributed under the terms of the GNU General Public License v3 (GPL
# Version 3), copied verbatim in the file "COPYING".
#
# See http://alice-o2.web.cern.ch/ for full licensing information.
#
# In applying this license CERN does not waive the privileges and immunities
# granted to it by virtue of its status as an Intergovernmental Organization
# or submit itself to any jurisdiction.

get_directory_property(include_dirs INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${CMAKE_CURRENT_SOURCE_DIR}/src")
list(APPEND include_dirs "${CMAKE_CURRENT_SOURCE_DIR}/../Run2DataModel/src")
get_target_property(arrow_inc_dir Arrow::Arrow INTERFACE_INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${arrow_inc_dir}")
list(REMOVE_DUPLICATES include_dirs)
include_directories(${include_dirs})

add_executable(run2ESD2Run3AOD
    src/TableBuilder.cxx
    src/run2ESD2Run3AOD.cxx
    src/Run3AODConverter.cxx
  )

add_executable(Run3AODDumpSchema
    src/Run3AODDumpSchema.cxx
  )

#install(
#  FILES ${CMAKE_CURRENT_BINARY_DIR}/lib${dict}_rdict.pcm ${CMAKE_CURRENT_BINARY_DIR}/lib${dict}.rootmap
#  DESTINATION ${CMAKE_INSTALL_LIBDIR}
#)

target_link_libraries(
  run2ESD2Run3AOD
  PUBLIC
    ROOT::Core
    Arrow::Arrow
    ROOT::RIO
    ROOT::MathCore
    ROOT::Matrix
    ROOT::Geom
    ROOT::Net
    ROOT::Hist
    ROOT::Gpad
    ROOT::Tree
    ROOT::EG
    ROOT::Physics
    ROOT::VMC
    ROOT::Minuit
    ROOT::XMLParser
    Run2ESDConverter
)

target_link_libraries(
  Run3AODDumpSchema
  PUBLIC
    Arrow::Arrow
)

# Install library and binaries
install(
  TARGETS Run2ESDConverter run2ESD2Run3AOD Run3AODDumpSchema
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

