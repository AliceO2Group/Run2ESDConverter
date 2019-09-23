#ifndef ALIMODULE_H
#define ALIMODULE_H
/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */

/* $Id$ */

//
// This is the basic class for any
// ALICE detector module, whether it is 
// sensitive or not. Detector classes depend
// on this.
//

#include <TNamed.h>
#include <TString.h>
#include <TClonesArray.h>

#include "AliTriggerDetector.h"

class TBrowser;
class TArrayI;
class TFile;
class TTree;
class TVirtualMC;
class AliLoader;
class AliTrackReference;
class AliDigitizer;
class AliDigitizationInput;
class AliVertexer;
class AliTracker;
class AliESD;
class AliRunLoader;
class AliRawReader;


class AliModule : public TNamed {
public:

  // Creators - distructors
  AliModule(const char* name, const char *title);
  AliModule();
  virtual ~AliModule();

  // Inline functions
  virtual  Int_t         GetNdigits() const {return 0;}
  virtual  Int_t         GetNhits()  const {return 0;}
  virtual  TArrayI      *GetIdtmed()   const {return fIdtmed;}
  virtual  TClonesArray *Digits() const {return 0;}
  virtual  TClonesArray *Hits()   const {return 0;}
  virtual  TObjArray    *Points() const {return 0;}
  virtual  Int_t         GetIshunt() const {return 0;}
  virtual  void          SetIshunt(Int_t) {}
  virtual  Bool_t        IsActive() const {return fActive;}
  virtual  void          SetActive(Bool_t v=kTRUE) {fActive = v;}
  virtual  Bool_t        IsFolder() const {return kTRUE;}
  virtual  Int_t&        LoMedium() {return fLoMedium;}
  virtual  Int_t&        HiMedium() {return fHiMedium;}
  virtual  Bool_t        IsModule() const {return kTRUE;}
  virtual  Bool_t        IsDetector() const {return kFALSE;}

  // Module composition
  virtual void AliMaterial(Int_t imat, const char* name, Float_t a, 
			   Float_t z, Float_t dens, Float_t radl,
			   Float_t absl, Float_t *buf=0, Int_t nwbuf=0) const;
  virtual void AliGetMaterial(Int_t imat, TString& name, Float_t &a, 
			      Float_t &z, Float_t &dens, Float_t &radl,
			      Float_t &absl) const;
  virtual void AliMixture(Int_t imat, const char *name, Float_t *a,
                          Float_t *z, Float_t dens, Int_t nlmat,
                          Float_t *wmat) const;
  virtual void AliMedium(Int_t numed, const char *name, Int_t nmat,
                          Int_t isvol, Int_t ifield, Float_t fieldm,
                          Float_t tmaxfd, Float_t stemax, Float_t deemax,
                          Float_t epsil, Float_t stmin, Float_t *ubuf=0,
                          Int_t nbuf=0) const;
  void AliMatrix(Int_t &nmat, Double_t theta1, Double_t phi1,
                 Double_t theta2, Double_t phi2, Double_t theta3,
                 Double_t phi3) const;
  
  // Virtual methods
  virtual void  BuildGeometry() {};
  virtual Int_t IsVersion() const =0;
  
  
  // Other methods
  virtual void        AddDigit(Int_t*, Int_t*){
  Error("AddDigit","Digits cannot be added to module %s\n",fName.Data());}
  virtual void        AddHit(Int_t, Int_t*, Float_t *) {
  Error("AddDigit","Hits cannot be added to module %s\n",fName.Data());}
  virtual void        Hits2SDigits() {}
  virtual AliDigitizer* CreateDigitizer(AliDigitizationInput* /*manager*/) const 
    {return NULL;}
  virtual AliTriggerDetector* CreateTriggerDetector() const
    { AliTriggerDetector* det = new AliTriggerDetector(); det->SetName(GetName()); return det;}
  virtual void        SDigits2Digits() {}
  virtual void        Hits2Digits() {}
  virtual void        Digits2Reco() {}
  virtual void        Digits2Raw();
  virtual void        Raw2Digits()  {}
  virtual Bool_t      Raw2SDigits(AliRawReader*) {return kFALSE;}
  virtual void        QADataMaker (const char *) {} 
  virtual void        Browse(TBrowser *) {} //PH Do we need it?
  virtual void        CreateGeometry() {}
  virtual void        CreateMaterials() {}
  virtual void        DefineOpticalProperties() {}
  virtual void        AddAlignableVolumes() const;
  virtual void        PreTrack(){}
  virtual void        PostTrack(){}
  virtual void        FinishEvent() {}
  virtual void        FinishRun() {}
  virtual void        FinishPrimary() {}
  virtual void        Init() {}
  virtual void        LoadPoints(Int_t ) {}
  virtual void        UpdateInternalGeometry() {}



  virtual void        MakeBranch(Option_t * /*opt =" "*/) {}
  virtual void        MakeTree(Option_t *) {}//skowron 

  virtual AliLoader*  MakeLoader(const char* topfoldername);  
  virtual AliLoader*  GetLoader() const {return 0x0;} //skowron
  

  virtual void        Paint(Option_t *) {} //PH Do we need it?
  virtual void        ResetDigits() {}
  virtual void        ResetSDigits() {}
  virtual void        ResetHits() {}
  virtual void        SetTimeGate(Float_t) {}
  virtual Float_t     GetTimeGate() const {return 1.e10;}
  virtual void        StepManager() {}
  virtual void        DisableStepManager() {fEnable = kFALSE;}
  virtual Bool_t      StepManagerIsEnabled() const {return fEnable;}
  virtual void        SetBufferSize(Int_t) {}  
  virtual Float_t     ZMin() const;
  virtual Float_t     ZMax() const;
// Track reference related
  virtual  AliTrackReference * AddTrackReference(Int_t label, Int_t id = -999);
  TTree* TreeTR();  //shorcut method for accessing treeTR from folder
  void                SetRunLoader(AliRunLoader* runLoader) 
    {fRunLoader = runLoader;}
  
// Quality Assurance methods
  virtual void CheckQA()    { ; }
  AliDigitizationInput* GetDigitizationInput() const {return fDigInput;}
  //  
  static void SetDensityFactor(Float_t density) { fgDensityFactor = density; }
  static Float_t GetDensityFactor() { return fgDensityFactor; }

  void CacheVMCInstance(TVirtualMC *mc) { fMC = mc; }
  
protected:      

  // Data members
  TVirtualMC   *fMC;          //! cached MC instance (to avoid calls to thread_local gMC=TVirtualMC::GetMC())

  TArrayI      *fIdtmed;      //!List of tracking medium numbers
  TArrayI      *fIdmate;      //!List of material numbers
  Int_t         fLoMedium;   //!Minimum tracking medium ID for this Module
  Int_t         fHiMedium;   //!Maximum tracking medium ID for this Module

  Bool_t        fActive;      //Detector activity flag
  Bool_t        fEnable;      //StepManager enabling flag
  Int_t         fMaxIterTrackRef;     //!for track refernce iterator routines
  Int_t         fCurrentIterTrackRef; //!for track refernce iterator routines

  AliRunLoader* fRunLoader;   //!local pointer to run loader
  AliDigitizationInput* fDigInput; //! input for sdigits -> digits
  static Float_t fgDensityFactor; //! factor that is multiplied to all material densities (ONLY for systematic studies)
 private:
  AliModule(const AliModule&);
  AliModule& operator=(const AliModule&);

  ClassDef(AliModule, 10)  //Base class for ALICE Modules
};
#endif
