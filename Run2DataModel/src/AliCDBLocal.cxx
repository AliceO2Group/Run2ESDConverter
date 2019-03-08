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

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                             //
// AliCDBLocal										       //
// access class to a DataBase in a local storage  			                       //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////

#include <cstdlib>
#include <stdexcept>
#include <fstream>

#include <TSystem.h>
#include <TObjString.h>
#include <TRegexp.h>
#include <TFile.h>
#include <TKey.h>

#include "AliCDBLocal.h"
#include "AliCDBEntry.h"
#include "AliFileUtilities.h"
#include "AliLog.h"
using namespace std;

ClassImp(AliCDBLocal)

//_____________________________________________________________________________
AliCDBLocal::AliCDBLocal(const char* baseDir):
  fBaseDirectory(baseDir) 
{
  // constructor

  AliDebug(1, Form("fBaseDirectory = %s",fBaseDirectory.Data()));

  // check baseDire: trying to cd to baseDir; if it does not exist, create it
  void* dir = gSystem->OpenDirectory(baseDir);
  if (dir == NULL) {
    if (gSystem->mkdir(baseDir, kTRUE)) {
       AliError(Form("Can't open directory <%s>!", baseDir)); //!!!!!!!! to be commented out for testing
    }

  } else {
    AliDebug(2,Form("Folder <%s> found",fBaseDirectory.Data()));
    gSystem->FreeDirectory(dir);
  }
  fType="local";
  fBaseFolder = fBaseDirectory;
}

//_____________________________________________________________________________
AliCDBLocal::~AliCDBLocal() {
// destructor

}


//_____________________________________________________________________________
Bool_t AliCDBLocal::FilenameToId(const char* filename, AliCDBRunRange& runRange,
    Int_t& version, Int_t& subVersion) {
  // build AliCDBId from filename numbers

  Ssiz_t mSize;

  // valid filename: Run#firstRun_#lastRun_v#version_s#subVersion.root
  TRegexp keyPattern("^Run[0-9]+_[0-9]+_v[0-9]+_s[0-9]+.root$");
  keyPattern.Index(filename, &mSize);
  if (!mSize) {
    AliDebug(2, Form("Bad filename <%s>.", filename));
    return kFALSE;
  }

  TString idString(filename);
  idString.Resize(idString.Length() - sizeof(".root") + 1);

  TObjArray* strArray = (TObjArray*) idString.Tokenize("_");

  TString firstRunString(((TObjString*) strArray->At(0))->GetString());
  runRange.SetFirstRun(atoi(firstRunString.Data() + 3));
  runRange.SetLastRun(atoi(((TObjString*) strArray->At(1))->GetString()));

  TString verString(((TObjString*) strArray->At(2))->GetString());
  version = atoi(verString.Data() + 1);

  TString subVerString(((TObjString*) strArray->At(3))->GetString());
  subVersion = atoi(subVerString.Data() + 1);

  delete strArray;

  return kTRUE;
}


//_____________________________________________________________________________
Bool_t AliCDBLocal::IdToFilename(const AliCDBId& id, TString& filename) const {
// build file name from AliCDBId data (run range, version, subVersion)

  AliDebug(1, Form("fBaseDirectory = %s",fBaseDirectory.Data()));

  if (!id.GetAliCDBRunRange().IsValid()) {
    AliDebug(2,Form("Invalid run range <%d, %d>.", 
          id.GetFirstRun(), id.GetLastRun()));
    return kFALSE;
  }

  if (id.GetVersion() < 0) {
    AliDebug(2,Form("Invalid version <%d>.", id.GetVersion()));
    return kFALSE;
  }

  if (id.GetSubVersion() < 0) {
    AliDebug(2,Form("Invalid subversion <%d>.", id.GetSubVersion()));
    return kFALSE;
  }

  filename = Form("Run%d_%d_v%d_s%d.root", id.GetFirstRun(), id.GetLastRun(),
      id.GetVersion(), id.GetSubVersion());

  filename.Prepend(fBaseDirectory +'/' + id.GetPath() + '/');

  return kTRUE;
}

//_____________________________________________________________________________
Bool_t AliCDBLocal::PrepareId(AliCDBId& id) {
// prepare id (version, subVersion) of the object that will be stored (called by PutEntry)

  TString dirName = Form("%s/%s", fBaseDirectory.Data(), id.GetPath().Data());

  // go to the path; if directory does not exist, create it
  void* dirPtr = gSystem->OpenDirectory(dirName);
  if (!dirPtr) {
    gSystem->mkdir(dirName, kTRUE);
    dirPtr = gSystem->OpenDirectory(dirName);

    if (!dirPtr) {
      AliError(Form("Can't create directory <%s>!", 
            dirName.Data()));
      return kFALSE;
    }
  }

  const char* filename;
  AliCDBRunRange aRunRange; // the runRange got from filename
  AliCDBRunRange lastRunRange(-1,-1); // highest runRange found
  Int_t aVersion, aSubVersion; // the version subVersion got from filename
  Int_t lastVersion = 0, lastSubVersion = -1; // highest version and subVersion found

  if (!id.HasVersion()) { // version not specified: look for highest version & subVersion

    while ((filename = gSystem->GetDirEntry(dirPtr))) { // loop on the files

      TString aString(filename);
      if (aString == "." || aString == "..") continue;

      if (!FilenameToId(filename, aRunRange, aVersion, 
            aSubVersion)) {
        AliDebug(2,Form(
              "Bad filename <%s>! I'll skip it.", 
              filename));
        continue;
      }

      if (!aRunRange.Overlaps(id.GetAliCDBRunRange())) continue;
      if(aVersion < lastVersion) continue;
      if(aVersion > lastVersion) lastSubVersion = -1;
      if(aSubVersion < lastSubVersion) continue;
      lastVersion = aVersion;
      lastSubVersion = aSubVersion;
      lastRunRange = aRunRange;
    }

    id.SetVersion(lastVersion);
    id.SetSubVersion(lastSubVersion + 1);

  } else { // version specified, look for highest subVersion only

    while ((filename = gSystem->GetDirEntry(dirPtr))) { // loop on the files

      TString aString(filename);
      if (aString == "." || aString == "..") {
        continue;
      }

      if (!FilenameToId(filename, aRunRange, aVersion, 
            aSubVersion)) {
        AliDebug(2,Form(
              "Bad filename <%s>!I'll skip it.",
              filename));	
        continue;
      }

      if (aRunRange.Overlaps(id.GetAliCDBRunRange()) 
          && aVersion == id.GetVersion()
          && aSubVersion > lastSubVersion) {
        lastSubVersion = aSubVersion;
        lastRunRange = aRunRange;
      }

    }

    id.SetSubVersion(lastSubVersion + 1);
  }

  gSystem->FreeDirectory(dirPtr);

  TString lastStorage = id.GetLastStorage();
  if(lastStorage.Contains(TString("grid"), TString::kIgnoreCase) &&
      id.GetSubVersion() > 0 ){
    AliError(Form("Grid to Local Storage error! local object with version v%d_s%d found:",id.GetVersion(), id.GetSubVersion()-1));
    AliError(Form("This object has been already transferred from Grid (check v%d_s0)!",id.GetVersion()));
    return kFALSE;
  }

  if(lastStorage.Contains(TString("new"), TString::kIgnoreCase) &&
      id.GetSubVersion() > 0 ){
    AliDebug(2, Form("A NEW object is being stored with version v%d_s%d",
          id.GetVersion(),id.GetSubVersion()));
    AliDebug(2, Form("and it will hide previously stored object with v%d_s%d!",
          id.GetVersion(),id.GetSubVersion()-1));
  }

  if(!lastRunRange.IsAnyRange() && !(lastRunRange.IsEqual(& id.GetAliCDBRunRange()))) 
    AliWarning(Form("Run range modified w.r.t. previous version (Run%d_%d_v%d_s%d)",
          lastRunRange.GetFirstRun(), lastRunRange.GetLastRun(), 
          id.GetVersion(), id.GetSubVersion()-1));

  return kTRUE;
}


//_____________________________________________________________________________
AliCDBId* AliCDBLocal::GetId(const AliCDBId& query) {
  // look for filename matching query (called by GetEntryId)

  // if querying for fRun and not specifying a version, look in the fValidFileIds list
  if(!AliCDBManager::Instance()->GetCvmfsOcdbTag().IsNull() && query.GetFirstRun() == fRun && !query.HasVersion()) {
  //if(query.GetFirstRun() == fRun && !query.HasVersion()) {
    // get id from fValidFileIds
    TIter iter(&fValidFileIds);

    AliCDBId *anIdPtr=0;
    AliCDBId* result=0;

    while((anIdPtr = dynamic_cast<AliCDBId*> (iter.Next()))){
      if(anIdPtr->GetPath() == query.GetPath()){
        result = new AliCDBId(*anIdPtr);
        break;
      }
    }
    return result;
  }

  // otherwise browse in the local filesystem CDB storage
  TString dirName = Form("%s/%s", fBaseDirectory.Data(), query.GetPath().Data());

  void* dirPtr = gSystem->OpenDirectory(dirName);
  if (!dirPtr) {
    AliDebug(2,Form("Directory <%s> not found", (query.GetPath()).Data()));
    AliDebug(2,Form("in DB folder %s", fBaseDirectory.Data()));
    return NULL;
  }

  const char* filename;
  AliCDBId *result = new AliCDBId();
  result->SetPath(query.GetPath());

  AliCDBRunRange aRunRange; // the runRange got from filename
  Int_t aVersion, aSubVersion; // the version and subVersion got from filename

  if (!query.HasVersion()) { // neither version and subversion specified -> look for highest version and subVersion

    while ((filename = gSystem->GetDirEntry(dirPtr))) { // loop on files

      TString aString(filename);
      if (aString.BeginsWith('.')) continue;

      if (!FilenameToId(filename, aRunRange, aVersion, aSubVersion)) continue;
      // aRunRange, aVersion, aSubVersion filled from filename

      if (!aRunRange.Comprises(query.GetAliCDBRunRange())) continue;
      // aRunRange contains requested run!

      AliDebug(1,Form("Filename %s matches\n",filename));

      if (result->GetVersion() < aVersion) {
        result->SetVersion(aVersion);
        result->SetSubVersion(aSubVersion);

        result->SetFirstRun(
            aRunRange.GetFirstRun());
        result->SetLastRun(
            aRunRange.GetLastRun());

      } else if (result->GetVersion() == aVersion
          && result->GetSubVersion()
          < aSubVersion) {

        result->SetSubVersion(aSubVersion);

        result->SetFirstRun(
            aRunRange.GetFirstRun());
        result->SetLastRun(
            aRunRange.GetLastRun());
      } else if (result->GetVersion() == aVersion
          && result->GetSubVersion() == aSubVersion){
        AliError(Form("More than one object valid for run %d, version %d_%d!",
              query.GetFirstRun(), aVersion, aSubVersion));
        gSystem->FreeDirectory(dirPtr);
        delete result;
        return NULL;
      }
    }

  } else if (!query.HasSubVersion()) { // version specified but not subversion -> look for highest subVersion
    result->SetVersion(query.GetVersion());

    while ((filename = gSystem->GetDirEntry(dirPtr))) { // loop on files

      TString aString(filename);
      if (aString.BeginsWith('.')) continue;

      if (!FilenameToId(filename, aRunRange, aVersion, aSubVersion)) continue;
      // aRunRange, aVersion, aSubVersion filled from filename

      if (!aRunRange.Comprises(query.GetAliCDBRunRange())) continue; 
      // aRunRange contains requested run!

      if(query.GetVersion() != aVersion) continue;
      // aVersion is requested version!

      if(result->GetSubVersion() == aSubVersion){
        AliError(Form("More than one object valid for run %d, version %d_%d!",
              query.GetFirstRun(), aVersion, aSubVersion));
        gSystem->FreeDirectory(dirPtr);
        delete result;
        return NULL;
      }
      if( result->GetSubVersion() < aSubVersion) {

        result->SetSubVersion(aSubVersion);

        result->SetFirstRun(
            aRunRange.GetFirstRun());
        result->SetLastRun(
            aRunRange.GetLastRun());
      } 
    }

  } else { // both version and subversion specified

    //AliCDBId dataId(queryId.GetAliCDBPath(), -1, -1, -1, -1);
    //Bool_t result;
    while ((filename = gSystem->GetDirEntry(dirPtr))) { // loop on files

      TString aString(filename);
      if (aString.BeginsWith('.')) continue;

      if (!FilenameToId(filename, aRunRange, aVersion, aSubVersion)){
        AliDebug(5, Form("Could not make id from file: %s", filename));
        continue;
      }
      // aRunRange, aVersion, aSubVersion filled from filename

      if (!aRunRange.Comprises(query.GetAliCDBRunRange())) continue;
      // aRunRange contains requested run!

      if(query.GetVersion() != aVersion || query.GetSubVersion() != aSubVersion){
          continue;
      }
      // aVersion and aSubVersion are requested version and subVersion!

      result->SetVersion(aVersion);
      result->SetSubVersion(aSubVersion);
      result->SetFirstRun(aRunRange.GetFirstRun());
      result->SetLastRun(aRunRange.GetLastRun());
      break;
    }
  }

  gSystem->FreeDirectory(dirPtr);

  return result;
}

//_____________________________________________________________________________
AliCDBEntry* AliCDBLocal::GetEntry(const AliCDBId& queryId) {
// get AliCDBEntry from the storage (the CDB file matching the query is
// selected by GetEntryId and the contained AliCDBid is passed here)

  AliCDBId* dataId = GetEntryId(queryId);

  TString errMessage(TString::Format("No valid CDB object found! request was: %s", queryId.ToString().Data()));
  if (!dataId || !dataId->IsSpecified()){
    AliError(Form("No file found matching this id!"));
    throw std::runtime_error(errMessage.Data());
    return NULL;
  }

  TString filename;
  if (!IdToFilename(*dataId, filename)) {
    AliError(Form("Bad data ID encountered!"));
    delete dataId;
    throw std::runtime_error(errMessage.Data());
    return NULL;
  }

  TFile file(filename, "READ"); // open file
  if (!file.IsOpen()) {
    AliError(Form("Can't open file <%s>!", filename.Data()));
    delete dataId;
    throw std::runtime_error(errMessage.Data());
    return NULL;
  }

  // get the only AliCDBEntry object from the file
  // the object in the file is an AliCDBEntry entry named "AliCDBEntry"

  AliCDBEntry* anEntry = dynamic_cast<AliCDBEntry*> (file.Get("AliCDBEntry"));
  if (!anEntry) {
    AliError(Form("Bad storage data: No AliCDBEntry in file!"));
    file.Close();
    delete dataId;
    throw std::runtime_error(errMessage.Data());
    return NULL;
  }

  AliCDBId& entryId = anEntry->GetId();

  // The object's Id are not reset during storage
  // If object's Id runRange or version do not match with filename,
  // it means that someone renamed file by hand. In this case a warning msg is issued.

  anEntry-> SetLastStorage("local");

  if(!entryId.IsEqual(dataId)){
    AliWarning(Form("Mismatch between file name and object's Id!"));
    AliWarning(Form("File name: %s", dataId->ToString().Data()));
    AliWarning(Form("Object's Id: %s", entryId.ToString().Data()));
  }

  // Check whether entry contains a TTree. In case load the tree in memory!
  LoadTreeFromFile(anEntry);

  // close file, return retrieved entry
  file.Close();
  delete dataId;

  return anEntry;
}

//_____________________________________________________________________________
AliCDBId* AliCDBLocal::GetEntryId(const AliCDBId& queryId) {
// get AliCDBId from the storage
// Via GetId, select the CDB file matching the query and return
// the contained AliCDBId

  AliCDBId* dataId = 0;

  // look for a filename matching query requests (path, runRange, version, subVersion)
  if (!queryId.HasVersion()) {
    // if version is not specified, first check the selection criteria list
    AliCDBId selectedId(queryId);
    GetSelection(&selectedId);
    dataId = GetId(selectedId);
  } else {
    dataId = GetId(queryId);
  }

  if (dataId && !dataId->IsSpecified()) {
    delete dataId;
    return NULL;
  }

  return dataId;
}

//_____________________________________________________________________________
void AliCDBLocal::GetEntriesForLevel0(const char* level0,
    const AliCDBId& queryId, TList* result) {
  // multiple request (AliCDBStorage::GetAll)

  TString level0Dir = Form("%s/%s", fBaseDirectory.Data(), level0);

  void* level0DirPtr = gSystem->OpenDirectory(level0Dir);
  if (!level0DirPtr) {
    AliDebug(2,Form("Can't open level0 directory <%s>!",
          level0Dir.Data()));
    return;
  }

  const char* level1;
  Long_t flag=0;
  while ((level1 = gSystem->GetDirEntry(level0DirPtr))) {

    TString level1Str(level1);
    // skip directories starting with a dot (".svn" and similar in old svn working copies)
    if (level1Str.BeginsWith('.')) {
      continue;
    }

    TString fullPath = Form("%s/%s",level0Dir.Data(), level1); 

    Int_t res=gSystem->GetPathInfo(fullPath.Data(), 0, (Long64_t*) 0, &flag, 0);

    if(res){
      AliDebug(2, Form("Error reading entry %s !",level1Str.Data()));
      continue;
    }
    if(!(flag&2)) continue; // bit 1 of flag = directory!

    if (queryId.GetAliCDBPath().Level1Comprises(level1)) {
      GetEntriesForLevel1(level0, level1, queryId, result);
    }
  }

  gSystem->FreeDirectory(level0DirPtr);
}

//_____________________________________________________________________________
void AliCDBLocal::GetEntriesForLevel1(const char* level0, const char* level1,
    const AliCDBId& queryId, TList* result) {
  // multiple request (AliCDBStorage::GetAll)

  TString level1Dir = Form("%s/%s/%s", fBaseDirectory.Data(), level0,level1);

  void* level1DirPtr = gSystem->OpenDirectory(level1Dir);
  if (!level1DirPtr) {
    AliDebug(2,Form("Can't open level1 directory <%s>!",
          level1Dir.Data()));
    return;
  }

  const char* level2;
  Long_t flag=0;
  while ((level2 = gSystem->GetDirEntry(level1DirPtr))) {

    TString level2Str(level2);
    // skip directories starting with a dot (".svn" and similar in old svn working copies)
    if (level2Str.BeginsWith('.')) {
      continue;
    }

    TString fullPath = Form("%s/%s",level1Dir.Data(), level2); 

    Int_t res=gSystem->GetPathInfo(fullPath.Data(), 0, (Long64_t*) 0, &flag, 0);

    if(res){
      AliDebug(2, Form("Error reading entry %s !",level2Str.Data()));
      continue;
    }
    if(!(flag&2)) continue; // skip if not a directory

    if (queryId.GetAliCDBPath().Level2Comprises(level2)) {

      AliCDBPath entryPath(level0, level1, level2);

      AliCDBId entryId(entryPath, queryId.GetAliCDBRunRange(),
          queryId.GetVersion(), queryId.GetSubVersion());

      // check filenames to see if any includes queryId.GetAliCDBRunRange()
      void* level2DirPtr = gSystem->OpenDirectory(fullPath);
      if (!level2DirPtr) {
        AliDebug(2,Form("Can't open level2 directory <%s>!", fullPath.Data()));
        return;
      }
      const char* level3;
      Long_t file_flag=0;
      while ((level3 = gSystem->GetDirEntry(level2DirPtr))) {
        TString fileName(level3);
        TString fullFileName = Form("%s/%s", fullPath.Data(), level3); 

        Int_t file_res=gSystem->GetPathInfo(fullFileName.Data(), 0, (Long64_t*) 0, &file_flag, 0);

        if(file_res){
          AliDebug(2, Form("Error reading entry %s !",level2Str.Data()));
          continue;
        }
        if(file_flag)
          continue; // it is not a regular file!

        // skip if result already contains an entry for this path
        Bool_t alreadyLoaded = kFALSE;
        Int_t nEntries = result->GetEntries();
        for(int i=0; i<nEntries; i++){
          AliCDBEntry *lEntry = (AliCDBEntry*) result->At(i);
          AliCDBId lId = lEntry->GetId();
          TString lPath = lId.GetPath();
          if(lPath.EqualTo(entryPath.GetPath())){
            alreadyLoaded = kTRUE;
            break;
          }
        }
        if (alreadyLoaded) continue;

        //skip filenames not matching the regex below
        TRegexp re("^Run[0-9]+_[0-9]+_");
        if(!fileName.Contains(re))
          continue;
        // Extract first- and last-run and version and subversion.
        // This allows to avoid quering for a calibration path if we did not find a filename with
        // run-range including the one specified in the query and
        // with version, subversion matching the query
        TString fn = fileName( 3, fileName.Length()-3 );
        TString firstRunStr = fn( 0, fn.First('_') );
        fn.Remove( 0, firstRunStr.Length()+1 );
        TString lastRunStr = fn( 0, fn.First('_') );
        fn.Remove( 0, lastRunStr.Length()+1 );
        TString versionStr = fn( 1, fn.First('_')-1 );
        fn.Remove( 0, versionStr.Length()+2 );
        TString subvStr = fn(1, fn.First('.')-1);
        Int_t firstRun = firstRunStr.Atoi();
        Int_t lastRun  = lastRunStr.Atoi();
        AliCDBRunRange rr(firstRun,lastRun);
        Int_t version = versionStr.Atoi();
        Int_t subVersion = subvStr.Atoi();

        AliCDBEntry* anEntry = 0;
        Bool_t versionOK = kTRUE, subVersionOK = kTRUE;
        if ( queryId.HasVersion() && version!=queryId.GetVersion())
          versionOK = kFALSE;
        if ( queryId.HasSubVersion() && subVersion!=queryId.GetSubVersion())
          subVersionOK = kFALSE;
        if (rr.Comprises(queryId.GetAliCDBRunRange()) && versionOK && subVersionOK )
        {
          anEntry = GetEntry(entryId);
          result->Add(anEntry);
        }
      }
    }
  }

  gSystem->FreeDirectory(level1DirPtr);
}

//_____________________________________________________________________________
TList* AliCDBLocal::GetEntries(const AliCDBId& queryId) {
// multiple request (AliCDBStorage::GetAll)

  TList* result = new TList();
  result->SetOwner();

  // if querying for fRun and not specifying a version, look in the fValidFileIds list
  if(queryId.GetFirstRun() == fRun && !queryId.HasVersion()) {
    // get id from fValidFileIds
    TIter *iter = new TIter(&fValidFileIds);
    TObjArray selectedIds;
    selectedIds.SetOwner(1);

    // loop on list of valid Ids to select the right version to get.
    // According to query and to the selection criteria list, version can be the highest or exact
    AliCDBId* anIdPtr=0;
    AliCDBId* dataId=0;
    AliCDBPath queryPath = queryId.GetAliCDBPath();
    while((anIdPtr = dynamic_cast<AliCDBId*> (iter->Next()))){
      AliCDBPath thisCDBPath = anIdPtr->GetAliCDBPath();
      if(!(queryPath.Comprises(thisCDBPath))){
        continue;
      }

      AliCDBId thisId(*anIdPtr);
      dataId = GetId(thisId);
      if(dataId)
        selectedIds.Add(dataId);
    }

    delete iter; iter=0;

    // selectedIds contains the Ids of the files matching all requests of query!
    // All the objects are now ready to be retrieved
    iter = new TIter(&selectedIds);
    while((anIdPtr = dynamic_cast<AliCDBId*> (iter->Next()))){
      AliCDBEntry* anEntry = GetEntry(*anIdPtr);
      if(anEntry) result->Add(anEntry);
    }
    delete iter; iter=0;
    return result;
  }

  void* storageDirPtr = gSystem->OpenDirectory(fBaseDirectory);
  if (!storageDirPtr) {
    AliDebug(2,Form("Can't open storage directory <%s>",
          fBaseDirectory.Data()));
    return NULL;
  }

  const char* level0;
  Long_t flag=0;
  while ((level0 = gSystem->GetDirEntry(storageDirPtr))) {

    TString level0Str(level0);
    // skip directories starting with a dot (".svn" and similar in old svn working copies)
    if (level0Str.BeginsWith('.')) {
      continue;
    }

    TString fullPath = Form("%s/%s",fBaseDirectory.Data(), level0); 

    Int_t res=gSystem->GetPathInfo(fullPath.Data(), 0, (Long64_t*) 0, &flag, 0);

    if(res){
      AliDebug(2, Form("Error reading entry %s !",level0Str.Data()));
      continue;
    }

    if(!(flag&2)) continue; // bit 1 of flag = directory!				

    if (queryId.GetAliCDBPath().Level0Comprises(level0)) {
      GetEntriesForLevel0(level0, queryId, result);
    }
  }

  gSystem->FreeDirectory(storageDirPtr);

  return result;	
}

//_____________________________________________________________________________
Bool_t AliCDBLocal::PutEntry(AliCDBEntry* entry, const char* mirrors) {
// put an AliCDBEntry object into the database

  AliCDBId& id = entry->GetId();

  // set version and subVersion for the entry to be stored
  if (!PrepareId(id)) return kFALSE;


  // build filename from entry's id
  TString filename="";
  if (!IdToFilename(id, filename)) {

    AliDebug(2,Form("Bad ID encountered! Subnormal error!"));
    return kFALSE;
  }

  TString mirrorsString(mirrors);
  if(!mirrorsString.IsNull())
    AliWarning("AliCDBLocal storage cannot take mirror SEs into account. They will be ignored.");

  // open file
  TFile file(filename, "CREATE");
  if (!file.IsOpen()) {
    AliError(Form("Can't open file <%s>!", filename.Data()));
    return kFALSE;
  }

  //SetTreeToFile(entry, &file);

  entry->SetVersion(id.GetVersion());
  entry->SetSubVersion(id.GetSubVersion());

  // write object (key name: "AliCDBEntry")
  Bool_t result = file.WriteTObject(entry, "AliCDBEntry");
  if (!result) AliDebug(2,Form("Can't write entry to file: %s", filename.Data()));

  file.Close();
  if(result) {
    if(!(id.GetPath().Contains("SHUTTLE/STATUS")))
      AliInfo(Form("CDB object stored into file %s",filename.Data()));
  }

  return result;
}

//_____________________________________________________________________________
TList* AliCDBLocal::GetIdListFromFile(const char* fileName){

  TString fullFileName(fileName);
  fullFileName.Prepend(fBaseDirectory+'/');
  TFile *file = TFile::Open(fullFileName);
  if (!file) {
    AliError(Form("Can't open selection file <%s>!", fullFileName.Data()));
    return NULL;
  }
  file->cd();

  TList *list = new TList();
  list->SetOwner();
  int i=0;
  TString keycycle;

  AliCDBId *id;
  while(1){
    i++;
    keycycle = "AliCDBId;";
    keycycle+=i;

    id = (AliCDBId*) file->Get(keycycle);
    if(!id) break;
    list->AddFirst(id);
  }
  file->Close(); delete file; file=0;	
  return list;
}

//_____________________________________________________________________________
Bool_t AliCDBLocal::Contains(const char* path) const{
// check for path in storage's fBaseDirectory

  TString dirName = Form("%s/%s", fBaseDirectory.Data(), path);
  Bool_t result=kFALSE;

  void* dirPtr = gSystem->OpenDirectory(dirName); 
  if (dirPtr) result=kTRUE;
  gSystem->FreeDirectory(dirPtr);

  return result;
}

//_____________________________________________________________________________
void AliCDBLocal::QueryValidFiles() {
// Query the CDB for files valid for AliCDBStorage::fRun.
// Fills list fValidFileIds with AliCDBId objects extracted from CDB files
// present in the local storage.
// If fVersion was not set, fValidFileIds is filled with highest versions.

  if(fVersion != -1) AliWarning ("Version parameter is not used by local storage query!");
  if(fMetaDataFilter) {
    AliWarning ("CDB meta data parameters are not used by local storage query!");
    delete fMetaDataFilter; fMetaDataFilter=0;
  }
  void* storageDirPtr = gSystem->OpenDirectory(fBaseDirectory);

  const char* level0;
  while ((level0 = gSystem->GetDirEntry(storageDirPtr))) {

    TString level0Str(level0);
    if (level0Str.BeginsWith(".")) {
      continue;
    }

    if (fPathFilter.Level0Comprises(level0)) {
      TString level0Dir = Form("%s/%s",fBaseDirectory.Data(),level0);
      void* level0DirPtr = gSystem->OpenDirectory(level0Dir);
      const char* level1;
      while ((level1 = gSystem->GetDirEntry(level0DirPtr))) {

        TString level1Str(level1);
        if (level1Str.BeginsWith(".")) {
          continue;
        }

        if (fPathFilter.Level1Comprises(level1)) {
          TString level1Dir = Form("%s/%s/%s",
              fBaseDirectory.Data(),level0,level1);

          void* level1DirPtr = gSystem->OpenDirectory(level1Dir);
          const char* level2;
          while ((level2 = gSystem->GetDirEntry(level1DirPtr))) {

            TString level2Str(level2);
            if (level2Str.BeginsWith(".")) {
              continue;
            }

            if (fPathFilter.Level2Comprises(level2)) {
              TString dirName = Form("%s/%s/%s/%s", fBaseDirectory.Data(), level0, level1, level2);

              void* dirPtr = gSystem->OpenDirectory(dirName);

              const char* filename;

              AliCDBRunRange aRunRange; // the runRange got from filename
              AliCDBRunRange hvRunRange; // the runRange of the highest version valid file
              Int_t aVersion, aSubVersion; // the version and subVersion got from filename
              Int_t highestV=-1, highestSubV=-1; // the highest version and subVersion for this calibration type

              while ((filename = gSystem->GetDirEntry(dirPtr))) { // loop on files

                TString aString(filename);
                if (aString.BeginsWith(".")) continue;

                if (!FilenameToId(filename, aRunRange, aVersion, aSubVersion)) {
                  continue;
                }

                AliCDBRunRange runrg(fRun, fRun);
                if (!aRunRange.Comprises(runrg))
                  continue;

                // check to keep the highest version/subversion (in case of more than one)
                if (aVersion > highestV) {
                  highestV = aVersion;
                  highestSubV = aSubVersion;
                  hvRunRange = aRunRange;
                } else if (aVersion == highestV) {
                  if (aSubVersion > highestSubV) {
                    highestSubV = aSubVersion;
                    hvRunRange = aRunRange;
                  }
                }
              }
              if(highestV >= 0){
                AliCDBPath validPath(level0, level1, level2);
                AliCDBId *validId = new AliCDBId(validPath, hvRunRange, highestV, highestSubV);
                fValidFileIds.AddLast(validId);
              }

              gSystem->FreeDirectory(dirPtr);
            }
          }
          gSystem->FreeDirectory(level1DirPtr);
        }
      }
      gSystem->FreeDirectory(level0DirPtr);
    }
  }
  gSystem->FreeDirectory(storageDirPtr);

}

/////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                             //
// AliCDBLocal factory  			                                               //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////

ClassImp(AliCDBLocalFactory)

//_____________________________________________________________________________
Bool_t AliCDBLocalFactory::Validate(const char* dbString) {
// check if the string is valid local URI

  TRegexp dbPatternLocal("^local://.+$");

  return (TString(dbString).Contains(dbPatternLocal) || TString(dbString).BeginsWith("snapshot://folder="));
}

//_____________________________________________________________________________
AliCDBParam* AliCDBLocalFactory::CreateParameter(const char* dbString) {
// create AliCDBLocalParam class from the URI string

  if (!Validate(dbString)) {
    return NULL;
  }

  TString checkSS(dbString);
  if(checkSS.BeginsWith("snapshot://"))
  {
    TString snapshotPath("OCDB");
    snapshotPath.Prepend(TString(gSystem->WorkingDirectory()) + '/');
    checkSS.Remove(0,checkSS.First(':')+3);
    return new AliCDBLocalParam(snapshotPath,checkSS);
  }

  // if the string argument is not a snapshot URI, than it is a plain local URI
  TString pathname(dbString + sizeof("local://") - 1);

  if(gSystem->ExpandPathName(pathname))
    return NULL;

  if (pathname[0] != '/') {
    pathname.Prepend(TString(gSystem->WorkingDirectory()) + '/');
  }
  //pathname.Prepend("local://");

  return new AliCDBLocalParam(pathname);
}

//_____________________________________________________________________________
AliCDBStorage* AliCDBLocalFactory::Create(const AliCDBParam* param) {
// create AliCDBLocal storage instance from parameters

  if (AliCDBLocalParam::Class() == param->IsA()) {

    const AliCDBLocalParam* localParam = 
      (const AliCDBLocalParam*) param;

    return new AliCDBLocal(localParam->GetPath());
  }

  return NULL;
}
//_____________________________________________________________________________
void AliCDBLocal::SetRetry(Int_t /* nretry */, Int_t /* initsec */) {

  // Function to set the exponential retry for putting entries in the OCDB

  AliInfo("This function sets the exponential retry for putting entries in the OCDB - to be used ONLY for AliCDBGrid --> returning without doing anything");
  return;
} 



/////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                             //
// AliCDBLocal Parameter class  			                                       //                                          //
//                                                                                             //
/////////////////////////////////////////////////////////////////////////////////////////////////

ClassImp(AliCDBLocalParam)

  //_____________________________________________________________________________
  AliCDBLocalParam::AliCDBLocalParam():
    AliCDBParam(),
    fDBPath()
{
  // default constructor

}

//_____________________________________________________________________________
AliCDBLocalParam::AliCDBLocalParam(const char* dbPath):
  AliCDBParam(),
  fDBPath(dbPath)
{
  // constructor

  SetType("local");
  SetURI(TString("local://") + dbPath);
}

//_____________________________________________________________________________
AliCDBLocalParam::AliCDBLocalParam(const char* dbPath, const char* uri):
  AliCDBParam(),
  fDBPath(dbPath)
{
  // constructor

  SetType("local");
  SetURI(TString("alien://") + uri);
}

//_____________________________________________________________________________
AliCDBLocalParam::~AliCDBLocalParam() {
  // destructor

}

//_____________________________________________________________________________
AliCDBParam* AliCDBLocalParam::CloneParam() const {
  // clone parameter

  return new AliCDBLocalParam(fDBPath);
}

//_____________________________________________________________________________
ULong_t AliCDBLocalParam::Hash() const {
  // return Hash function

  return fDBPath.Hash();
}

//_____________________________________________________________________________
Bool_t AliCDBLocalParam::IsEqual(const TObject* obj) const {
  // check if this object is equal to AliCDBParam obj

  if (this == obj) {
    return kTRUE;
  }

  if (AliCDBLocalParam::Class() != obj->IsA()) {
    return kFALSE;
  }

  AliCDBLocalParam* other = (AliCDBLocalParam*) obj;

  return fDBPath == other->fDBPath;
}

