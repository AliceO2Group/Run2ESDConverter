#!/bin/sh -ex

O2_ROOT=${O2_ROOT:-../O2}

cp $O2_ROOT/Framework/Core/include/Framework/TableBuilder.h Converter/src/Framework
cp $O2_ROOT/Framework/Core/include/Framework/ASoA.h Converter/src/Framework
cp $O2_ROOT/Framework/Core/include/Framework/AnalysisDataModel.h Converter/src/Framework
cp $O2_ROOT/Framework/Core/include/Framework/RootTableBuilderHelpers.h Converter/src/Framework
cp $O2_ROOT/Framework/Core/src/TableBuilder.cxx Converter/src/
