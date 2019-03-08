#ifndef ALIDETECTOR_H
#define ALIDETECTOR_H
/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */

/* $Id$ */

// 
// This is the basic detector class from which
// all ALICE detector derive.
// This class is the base for the implementation of all detectors
// in ALICE
//

#include "AliModule.h"

class AliHit;
class TTree;
class TBranch;
class AliLoader;

class AliDetector : public AliModule {

public:

  // Creators - distructors
  AliDetector(const char* name, const char *title);
  AliDetector();
  virtual ~AliDetector();

  // Inline functions
  virtual int   GetNdigits() const {return fNdigits;}
  virtual int   GetNhits()   const {return fNhits;}
  TClonesArray *Digits() const {return fDigits;}
  TClonesArray *Hits()   const {return fHits;}
  virtual  Bool_t        IsModule() const {return kFALSE;}
  virtual  Bool_t        IsDetector() const {return kTRUE;}

  Int_t         GetIshunt() const {return fIshunt;}
  void          SetIshunt(Int_t ishunt) {fIshunt=ishunt;}
  
  // Other methods
  virtual void        Publish(const char *dir, void *c, const char *name=0) const;
  virtual void        Browse(TBrowser *b);
  virtual void        FinishRun();
  virtual void        MakeBranch(Option_t *opt=" ");
  virtual void        ResetDigits();
  virtual void        ResetHits();
  virtual void        AddAlignableVolumes() const;

  virtual void        SetTreeAddress();
  virtual void        SetTimeGate(Float_t gate) {fTimeGate=gate;}
  virtual Float_t     GetTimeGate() const {return fTimeGate;}
  virtual void        StepManager() {}
  virtual void        DrawModule() const {}
  virtual AliHit*     FirstHit(Int_t track);
  virtual AliHit*     NextHit();
  virtual void        SetBufferSize(Int_t bufsize=8000) {fBufferSize = bufsize;}  
  virtual TBranch*    MakeBranchInTree(TTree *tree, const char* cname, void* address, Int_t size=32000, const char *file=0);
  virtual TBranch*    MakeBranchInTree(TTree *tree, const char* cname, const char* name, void* address, Int_t size=32000, Int_t splitlevel=99, const char *file=0);
  
  void MakeTree(Option_t *option); //skowron
  virtual void        RemapTrackHitIDs(Int_t *) {}
  
  virtual AliLoader* MakeLoader(const char* topfoldername); //builds standard getter (AliLoader type)
  void    SetLoader(AliLoader* loader){fLoader = loader;}
  AliLoader* GetLoader() const {return fLoader;} //skowron
    // Data members
protected:      
  
  Float_t       fTimeGate;    //Time gate in seconds

  Int_t         fIshunt;      //1 if the hit is attached to the primary
  Int_t         fNhits;       //!Number of hits
  Int_t         fNdigits;     //!Number of digits
  Int_t         fBufferSize;  //!buffer size for Tree detector branches
  Int_t         fMaxIterHit;  //!Limit for the hit iterator
  Int_t         fCurIterHit;  //!Counter for the hit iterator
  TClonesArray *fHits;        //!List of hits for one track only
  TClonesArray *fDigits;      //!List of digits for this detector

  AliLoader*  fLoader;//! pointer to getter for this module skowron

 private:
  AliDetector(const AliDetector &det);
  AliDetector &operator=(const AliDetector &det);

  ClassDef(AliDetector,5)  //Base class for ALICE detectors
};
#endif
