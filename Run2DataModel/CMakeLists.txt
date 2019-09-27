# Copyright CERN and copyright holders of ALICE O2. This software is
# distributed under the terms of the GNU General Public License v3 (GPL
# Version 3), copied verbatim in the file "COPYING".
#
# See http://alice-o2.web.cern.ch/ for full licensing information.
#
# In applying this license CERN does not waive the privileges and immunities
# granted to it by virtue of its status as an Intergovernmental Organization
# or submit itself to any jurisdiction.

include(files.cmake)

get_directory_property(include_dirs INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/src")
get_target_property(arrow_inc_dir Arrow::Arrow INTERFACE_INCLUDE_DIRECTORIES)
list(APPEND include_dirs "${arrow_inc_dir}")
list(REMOVE_DUPLICATES include_dirs)
include_directories(${include_dirs})

set(dict "Run2ESDConverterDict")
set(dict_src ${CMAKE_CURRENT_BINARY_DIR}/${dict}.cxx)
set_source_files_properties(${dict_src} PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast")
set_source_files_properties(${dict_src} PROPERTIES GENERATED TRUE)

root_generate_dictionary("Run2ESDConverterDict" ${HEADERS} LINKDEF src/Run2ConversionLinkdef.h)

add_library(Run2ESDConverter SHARED
  ${SOURCES}
    Run2ESDConverterDict
  )

#install(
#  FILES ${CMAKE_CURRENT_BINARY_DIR}/lib${dict}_rdict.pcm ${CMAKE_CURRENT_BINARY_DIR}/lib${dict}.rootmap
#  DESTINATION ${CMAKE_INSTALL_LIBDIR}
#)

target_link_libraries(
  Run2ESDConverter
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
)

# Install library and binaries
install(
  TARGETS Run2ESDConverter
  LIBRARY DESTINATION lib
)
