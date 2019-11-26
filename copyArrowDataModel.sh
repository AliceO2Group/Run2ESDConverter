#!/bin/sh -ex

O2_ROOT=${O2_ROOT:-../O2}

cp $O2_ROOT/Framework/Core/include/Framework/TableBuilder.h Converter/src/Framework
cp $O2_ROOT/Framework/Core/include/Framework/ASoA.h Converter/src/Framework
cp $O2_ROOT/Framework/Core/include/Framework/AnalysisDataModel.h Converter/src/Framework
cp $O2_ROOT/Framework/Core/include/Framework/RootTableBuilderHelpers.h Converter/src/Framework
cp $O2_ROOT/Framework/Core/include/Framework/Expressions.h Converter/src/Framework
cp $O2_ROOT/Framework/Core/include/Framework/BasicOps.h Converter/src/Framework
cp $O2_ROOT/Framework/Core/src/TableBuilder.cxx Converter/src/
cp $O2_ROOT/Framework/Foundation/include/Framework/FunctionalHelpers.h Converter/src/Framework
cp $O2_ROOT/Framework/Foundation/include/Framework/CompilerBuiltins.h Converter/src/Framework
cp $O2_ROOT/Framework/Foundation/include/Framework/Traits.h Converter/src/Framework
