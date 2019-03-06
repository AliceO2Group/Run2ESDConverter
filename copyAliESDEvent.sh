#!/bin/bash -ex

SEEDS="AliESDEvent AliExternalTrackParam AliVTrack AliVEvent AliTOFHeader AliESDRun AliTimeStamp AliESDHeader AliDigit AliBitPacking AliESDZDC AliVZDC AliDigitizationInput AliStream AliFMDFloatMap AliPIDResponse AliSimulation"
ALIROOT_ROOT=${ALIROOT_ROOT:-../AliRoot}
O2_ROOT=${O2_ROOT:-../O2}

#SEEDS="AliESDEvent TTreeStream AliStream AliDigit"
TARGET_DIR=Run2DataModel/src
mkdir -p $TARGET_DIR
mkdir -p Converter/src/Framework

cp ${O2_ROOT}/Framework/Core/src/TableBuilder.cxx Converter/src
cp ${O2_ROOT}/Framework/Core/include/Framework/TableBuilder.h Converter/src/Framework
cp ${O2_ROOT}/Framework/Core/include/Framework/RootTableBuilderHelpers.h Converter/src
cp ${O2_ROOT}/Framework/Core/include/Framework/TableBuilder.h Converter/src/Framework

copy_seed() {
  local s=$1
  find $ALIROOT_ROOT -name $s.h -exec cp {} $TARGET_DIR \;
  find $ALIROOT_ROOT -name $s.cxx -exec cp {} $TARGET_DIR \;
  MOREFILES=$(cat $TARGET_DIR/$s.cxx | grep -e "#[ ]*include \"" | sed -e's/.* "//;s/".*//g' | grep -v "^T" | sed -e 's/.h$//')
  MOREFILES="$MOREFILES $(cat $TARGET_DIR/$s.cxx | grep -e "#[ ]*include <Ali" | sed -e's/.* <//;s/>.*//g' | grep -v "^T" | sed -e 's/.h$//')"
  MOREFILES="$MOREFILES $(cat $TARGET_DIR/$s.h | grep -e "#[ ]*include \"" | sed -e's/.* "//;s/"//g' | sed -e 's/.h$//')"
  MOREFILES="$MOREFILES $(cat $TARGET_DIR/$s.h | grep -e "#[ ]*include <Ali" | sed -e's/.* <//;s/>//g' | sed -e 's/.h$//')"
  echo $MOREFILES
  for m in $MOREFILES; do
    if [ -f $TARGET_DIR/$m.h ]; then
      continue
    fi
    if [ -f $TARGET_DIR/$m.cxx ]; then
      continue
    fi
    copy_seed $m
    SRCS="$SRCS $m.cxx"
    HDRS="$HDRS $m.h"
  done
}

for s in $SEEDS ; do
  copy_seed $s
done

cat << \EOF >Run2DataModel/files.cmake
# Copyright CERN and copyright holders of ALICE O2. This software is
# distributed under the terms of the GNU General Public License v3 (GPL
# Version 3), copied verbatim in the file "COPYING".
#
# See http://alice-o2.web.cern.ch/ for full licensing information.
#
# In applying this license CERN does not waive the privileges and immunities
# granted to it by virtue of its status as an Intergovernmental Organization
# or submit itself to any jurisdiction.

set(SOURCES
  @SRCS@
   )

set(HEADERS
  @HDRS@
  )
EOF

LINKDEFS="ESD STEERBase STEER HLTbase CDB STAT RAWDatarec RAWDatabase HMPIDbase HMPIDrec RAWDatasim"
for x in $LINKDEFS; do
  find ${ALIROOT_ROOT} -name ${x}LinkDef.h -exec cp {} $TARGET_DIR \;
done

cat << \EOF > $TARGET_DIR/Run2ConversionLinkdef.h
#include "ESDLinkDef.h"
#include "STEERBaseLinkDef.h"
#include "STEERLinkDef.h"
#include "HLTbaseLinkDef.h"
#include "CDBLinkDef.h"
#include "STATLinkDef.h"
#include "RAWDatarecLinkDef.h"
#include "RAWDatabaseLinkDef.h"
#include "HMPIDbaseLinkDef.h"
#include "HMPIDrecLinkDef.h"
#include "RAWDatasimLinkDef.h"
EOF

SRCS=`find $TARGET_DIR -name *.cxx | sed -e 's/Run2DataModel\///g' | grep -v AliHLT`
HDRS=`find $TARGET_DIR -name *.h | sed -e 's/Run2DataModel\///g' | grep -v AliHLT | grep -i -v LinkDef.h | grep -v Run2ESDConversionHelpers.h`
echo $SRCS
echo $HDRS
perl -p -i -e "s|[\@]SRCS[\@]|${SRCS}|" Run2DataModel/files.cmake
perl -p -i -e "s|[\@]HDRS[\@]|${HDRS}|" Run2DataModel/files.cmake
perl -p -i -e 's/<AliTimeStamp.h>/"AliTimeStamp.h"/' $TARGET_DIR/AliESDRun.h
perl -p -i -e 's/<AliVZDC.h>/"AliVZDC.h"/' $TARGET_DIR/AliESDZDC.h
rm -fr $TARGET_DIR/AliHLT*
if grep AliHLTSimulation.h Run2DataModel/src/AliSimulation.cxx; then
ed Run2DataModel/src/AliSimulation.cxx << \EOF
,s/.*AliHLTSimulation.h.*//
/AliSimulation::CreateHLT/+2
/AliSimulation::CreateHLT/+2,/^}/-1d
/AliSimulation::RunHLT/+2
/AliSimulation::RunHLT/+2,/^}/-1d
wq
EOF
fi
