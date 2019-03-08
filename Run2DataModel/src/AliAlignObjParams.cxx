/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

//-----------------------------------------------------------------
//   Implementation of the alignment object class through
//   the concrete representation of alignment object class
//   AliAlignObjParams derived from the base class AliAlignObj
//-----------------------------------------------------------------

#include "AliLog.h"
#include "AliAlignObj.h"
#include "AliAlignObjParams.h"

ClassImp(AliAlignObjParams)

//_____________________________________________________________________________
AliAlignObjParams::AliAlignObjParams() : AliAlignObj()
{
  // default constructor
  //
  fTranslation[0]=fTranslation[1]=fTranslation[2]=0.;
  fRotation[0]=fRotation[1]=fRotation[2]=0.;
}

//_____________________________________________________________________________
AliAlignObjParams::AliAlignObjParams(const char* symname, UShort_t volUId, Double_t x, Double_t y, Double_t z, Double_t psi, Double_t theta, Double_t phi, Bool_t global) : AliAlignObj(symname,volUId)
{
  // standard constructor with 3 translation + 3 rotation parameters
  // If the user explicitly sets the global variable to kFALSE then the
  // parameters are interpreted as giving the local transformation.
  // This requires to have a gGeoMenager active instance, otherwise the
  // constructor will fail (no object created)
  // 
  if(global){
    SetPars(x, y, z, psi, theta, phi);
  }else{
    if(!SetLocalPars(x,y,z,psi,theta,phi)) { AliFatal("Alignment object creation failed (TGeo instance needed)!"); }
  }
}

//_____________________________________________________________________________
AliAlignObjParams::AliAlignObjParams(const char* symname, UShort_t volUId, TGeoMatrix& m, Bool_t global) : AliAlignObj(symname,volUId)
{
  // standard constructor with TGeoMatrix
  // If the user explicitly sets the global variable to kFALSE then the
  // parameters are interpreted as giving the local transformation.
  // This requires to have a gGeoMenager active instance, otherwise the
  // constructor will fail (no object created)
  //

  if (!SetMatrix(m)) { AliFatal("Alignment object creation failed (can't extract roll-pitch-yall angles from the matrix)!");}

  if (!global) {
    if (!SetLocalPars(fTranslation[0],fTranslation[1],fTranslation[2],fRotation[0],fRotation[1],fRotation[2])) { AliFatal ("Alignment object creation failed (TGeo instance needed)!"); }
  }
}

//_____________________________________________________________________________
AliAlignObjParams::AliAlignObjParams(const AliAlignObj& theAlignObj) :
  AliAlignObj(theAlignObj)
{
  // copy constructor
  //
  Double_t tr[3];
  theAlignObj.GetTranslation(tr);
  SetTranslation(tr[0],tr[1],tr[2]);
  Double_t rot[3];
  if (theAlignObj.GetAngles(rot))
    SetRotation(rot[0],rot[1],rot[2]);
  else
    fRotation[0]=fRotation[1]=fRotation[2]=0.;
}

//_____________________________________________________________________________
AliAlignObjParams &AliAlignObjParams::operator =(const AliAlignObj& theAlignObj)
{
  // assignment operator
  //
  if(this==&theAlignObj) return *this;
  ((AliAlignObj *)this)->operator=(theAlignObj);

  Double_t tr[3];
  theAlignObj.GetTranslation(tr);
  SetTranslation(tr[0],tr[1],tr[2]);
  Double_t rot[3];
  if (theAlignObj.GetAngles(rot))
    SetRotation(rot[0],rot[1],rot[2]);

  return *this;
}

//_____________________________________________________________________________
AliAlignObjParams::~AliAlignObjParams()
{
  // default destructor
  //
}

//_____________________________________________________________________________
void AliAlignObjParams::SetTranslation(const TGeoMatrix& m)
{
  // set the translation parameters extracting them from the matrix
  // passed as argument
  // 
  if(m.IsTranslation()){
    const Double_t* tr = m.GetTranslation();
    fTranslation[0]=tr[0];  fTranslation[1]=tr[1]; fTranslation[2]=tr[2];
  }else{
    fTranslation[0] = fTranslation[1] = fTranslation[2] = 0.;
  }
}

//_____________________________________________________________________________
Bool_t AliAlignObjParams::SetRotation(const TGeoMatrix& m)
{
  // set the rotation parameters extracting them from the matrix
  // passed as argument
  // 
  if(m.IsRotation()){
    const Double_t* rot = m.GetRotationMatrix();
    return MatrixToAngles(rot,fRotation);
  }else{
    fRotation[0] = fRotation[1] = fRotation[2] = 0.;
    return kTRUE;
  }
}

//_____________________________________________________________________________
void AliAlignObjParams::GetMatrix(TGeoHMatrix& m) const
{
  // get the transformation matrix from the data memebers parameters
  m.SetTranslation(&fTranslation[0]);
  Double_t rot[9];
  AnglesToMatrix(fRotation,rot);
  m.SetRotation(rot);
}

//_____________________________________________________________________________
AliAlignObj& AliAlignObjParams::Inverse() const
{
  // Return a temporary "inverse" of the alignment object, that is return
  // an object with inverted transformation matrix.
  //
   static AliAlignObjParams a;
   a = *this;

   TGeoHMatrix m;
   GetMatrix(m);
   a.SetMatrix(m.Inverse());

   return a;
}
