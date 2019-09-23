#ifndef ALIMATHBASE_H
#define ALIMATHBASE_H
/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */


 
#include "TObject.h"
#include "TVectorD.h"
#include "TMatrixD.h"
#include "TGraph2D.h"
#include "TGraph.h"

class TH1F;
class TH3;

 
class AliMathBase : public TObject
{
 public:
  AliMathBase();
  virtual ~AliMathBase();
  static void    EvaluateUni(const Int_t nvectors, const Double_t *data, Double_t &mean, Double_t &sigma, const Int_t hSub);
  static void    EvaluateUniExternal(Int_t nvectors, Double_t *data, Double_t &mean, Double_t &sigma, Int_t hh, Float_t externalfactor=1);
  static Int_t  Freq(Int_t n, const Int_t *inlist, Int_t *outlist, Bool_t down);    
  static void TruncatedMean(TH1F * his, TVectorD *param, Float_t down=0, Float_t up=1.0, Bool_t verbose=kFALSE);
  static void LTM(TH1F * his, TVectorD *param=0 , Float_t fraction=1,  Bool_t verbose=kFALSE);
  static Double_t  FitGaus(TH1F* his, TVectorD *param=0, TMatrixD *matrix=0, Float_t xmin=0, Float_t xmax=0,  Bool_t verbose=kFALSE);
  static Double_t  FitGaus(Float_t *arr, Int_t nBins, Float_t xMin, Float_t xMax, TVectorD *param=0, TMatrixD *matrix=0, Bool_t verbose=kFALSE);
  static Float_t  GetCOG(Short_t *arr, Int_t nBins, Float_t xMin, Float_t xMax, Float_t *rms=0, Float_t *sum=0);

  static Double_t TruncatedGaus(Double_t mean, Double_t sigma, Double_t cutat);
  static Double_t TruncatedGaus(Double_t mean, Double_t sigma, Double_t leftCut, Double_t rightCut);

  static TGraph2D *  MakeStat2D(TH3 * his, Int_t delta0, Int_t delta1, Int_t type);
  static TGraph *  MakeStat1D(TH3 * his, Int_t delta1, Int_t type);

  static Double_t ErfcFast(Double_t x);                           // Complementary error function erfc(x)
  static Double_t ErfFast(Double_t x) {return 1-ErfcFast(x);}     // Error function erf(x)

  //
  // TestFunctions:
  //
 static  void TestGausFit(Int_t nhistos=5000);

  //
  // Distributions
  //

  static Double_t Gamma(Double_t k=0);

  //
  // Other useful functions
  //
  static UInt_t NumberOfSetBits(UInt_t i) {
    // Function that calculates the Hamming weight of an UInt_t
    // see
    // https://en.wikipedia.org/wiki/Hamming_weight
    // https://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
    i = i - ((i >> 1) & 0x55555555);
    i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
    return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
  }

  static Float_t TruncateFloatFraction(Float_t x, UInt_t mask = 0xFFFFFF00){
    // Mask the less significant bits in the float fraction (1 bit sign, 8 bits exponent, 23 bits fraction), see
    // https://en.wikipedia.org/wiki/Single-precision_floating-point_format
    // mask 0xFFFFFF00 means 23 - 8 = 15 bits in the fraction
    union { Float_t y; UInt_t iy;} myu;
    myu.y = x;
    myu.iy &= mask;
    return myu.y;
  }

 ClassDef(AliMathBase,0) // Various mathematical tools for physics analysis - which are not included in ROOT TMath
 
};
#endif
