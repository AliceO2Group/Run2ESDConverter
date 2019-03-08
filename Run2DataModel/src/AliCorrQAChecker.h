#ifndef ALICORRQACHECKER_H
#define ALICORRQACHECKER_H
/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */


/* $Id: AliCorrQAChecker.h 27115 2008-07-04 15:12:14Z hristov $ */

/*
  Checks the quality assurance. 
  By comparing with reference data
  Y. Schutz CERN July 2007
*/


// --- ROOT system ---
class TFile ; 
class TH1F ; 
class TH1I ; 

// --- Standard library ---

// --- AliRoot header files ---
#include "AliQACheckerBase.h"
class AliCorrLoader ; 

class AliCorrQAChecker: public AliQACheckerBase {

public:
  AliCorrQAChecker() : AliQACheckerBase("Corr","Corr Quality Assurance Data Maker") {;}          // ctor
  virtual ~AliCorrQAChecker() {;} // dtor

  virtual void   Run(AliQAv1::ALITASK_t /*tsk*/, TNtupleD ** /*nt*/, AliDetectorRecoParam * /*recoParam*/) ;


private:
  AliCorrQAChecker(const AliCorrQAChecker& qac); // Not implemented
  AliCorrQAChecker& operator=(const AliCorrQAChecker& qac); // Not implemented
  Double_t * CheckN(AliQAv1::ALITASK_t index, TNtupleD ** nData, AliDetectorRecoParam * recoParam) ; 

  ClassDef(AliCorrQAChecker,1)  // description 

};

#endif // AliCORRQAChecker_H
