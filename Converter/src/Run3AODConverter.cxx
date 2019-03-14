// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.
#include "Run3AODConverter.h"
#include "Framework/TableBuilder.h"

#include "AliESDEvent.h"
#include "AliESDtrack.h"
#include "AliESDVZERO.h"
#include "AliESDCaloCells.h"
#include "AliESDMuonTrack.h"
#include "AliExternalTrackParam.h"

#include <TTree.h>

#include <arrow/util/io-util.h>
#include <arrow/ipc/writer.h>
#include <arrow/io/file.h>
#include <arrow/io/buffered.h>
#include <arrow/util/key_value_metadata.h>

#include <memory>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>

template class std::shared_ptr<arrow::Table>;

namespace o2
{
namespace framework
{
namespace run2
{

void Run3AODConverter::convert(TTree* tEsd, std::shared_ptr<arrow::io::OutputStream> stream)
{
  TableBuilder trackParBuilder;
  TableBuilder trackParCovBuilder;
  TableBuilder trackExtraBuilder;
  TableBuilder caloBuilder;
  TableBuilder muonBuilder;
  TableBuilder v0Builder;

  auto trackFiller = trackParBuilder.persist<
    int,
    float,
    float,
    float,
    float,
    float,
    float,
    float>({ "fID4Tracks",
             "fX",
             "fAlpha",
             "fY",
             "fZ",
             "fSnp",
             "fTgl",
             "fSigned1Pt" });

  auto sigmaFiller = trackParCovBuilder.persist<
    float,
    float,
    float,
    float,
    float,
    float,
    float,
    float,
    float,
    float,
    float,
    float,
    float,
    float,
    float>({ "fCYY",
             "fCZY",
             "fCZZ",
             "fCSnpY",
             "fCSnpZ",
             "fCSnpSnp",
             "fCTglY",
             "fCTglZ",
             "fCTglSnp",
             "fCTglTgl",
             "fC1PtY",
             "fC1PtZ",
             "fC1PtSnp",
             "fC1PtTgl",
             "fC1Pt21Pt2" });

  auto extraFiller = trackExtraBuilder.persist<
    float,
    uint64_t,
    uint8_t,
    uint16_t,
    uint8_t,
    float,
    float,
    float,
    float,
    float,
    float,
    float,
    float>({ "fTPCinnerP",
             "fFlags",
             "fITSClusterMap",
             "fTPCncls",
             "fTRDntracklets",
             "fITSchi2Ncl",
             "fTPCchi2Ncl",
             "fTRDchi2",
             "fTOFchi2",
             "fTPCsignal",
             "fTRDsignal",
             "fTOFsignal",
             "fLength" });

  auto caloFiller = caloBuilder.persist<
    int32_t,
    int64_t,
    float,
    float,
    int8_t>(
    {
      "fID4Calo",
      "fCellNumber",
      "fAmplitude",
      "fTime",
      "fType",
    });

  auto muonFiller = muonBuilder.persist<
    int,
    float,
    float,
    float,
    float,
    float,
    float,
    //float, // fixme... we need to support arrays...
    float,
    float>({
    "fID4mu",
    "fInverseBendingMomentum",
    "fThetaX",
    "fThetaY",
    "fZmu",
    "fBendingCoor",
    "fNonBendingCoor",
    // "fCovariances",
    "fChi2",
    "fChi2MatchTrigger",
  });

  auto vzeroFiller = v0Builder.persist<
    int>({ "fID4vz" });

  AliESDEvent* esd = new AliESDEvent();
  esd->ReadFromTree(tEsd);
  size_t nev = tEsd->GetEntries();
  size_t ntrk = 0;
  size_t nmu = 0;
  size_t ncalo = 0;
  size_t nvzero = 0;

  for (size_t iev = 0; iev < nev; ++iev) {
    esd->Reset();
    tEsd->GetEntry(iev);
    esd->ConnectTracks();

    // Tracks information
    ntrk = esd->GetNumberOfTracks();
    for (size_t itrk = 0; itrk < ntrk; ++itrk) {
      AliESDtrack* track = esd->GetTrack(itrk);
      track->SetESDEvent(esd);
      trackFiller(
        0,
        iev,
        track->GetX(),
        track->GetAlpha(),
        track->GetY(),
        track->GetZ(),
        track->GetSnp(),
        track->GetTgl(),
        track->GetSigned1Pt());

      sigmaFiller(
        0,
        track->GetSigmaY2(),
        track->GetSigmaZY(),
        track->GetSigmaZ2(),
        track->GetSigmaSnpY(),
        track->GetSigmaSnpZ(),
        track->GetSigmaSnp2(),
        track->GetSigmaTglY(),
        track->GetSigmaTglZ(),
        track->GetSigmaTglSnp(),
        track->GetSigmaTgl2(),
        track->GetSigma1PtY(),
        track->GetSigma1PtZ(),
        track->GetSigma1PtSnp(),
        track->GetSigma1PtTgl(),
        track->GetSigma1Pt2());
      const AliExternalTrackParam* intp = track->GetTPCInnerParam();

      extraFiller(
        0,
        (intp ? intp->GetP() : 0), // Set the momentum to 0 if the track did not reach TPC      //
        track->GetStatus(),
        //
        track->GetITSClusterMap(),
        track->GetTPCNcls(),
        track->GetTRDntracklets(),
        //
        (track->GetITSNcls() ? track->GetITSchi2() / track->GetITSNcls() : 0),
        (track->GetTPCNcls() ? track->GetTPCchi2() / track->GetTPCNcls() : 0),
        track->GetTRDchi2(),
        track->GetTOFchi2(),
        //
        track->GetTPCsignal(),
        track->GetTRDsignal(),
        track->GetTOFsignal(),
        track->GetIntegratedLength());
    } // End loop on tracks

    // Calorimeters:
    // EMCAL
    AliESDCaloCells* cells = esd->GetEMCALCells();
    size_t nCells = cells->GetNumberOfCells();
    ncalo += nCells;
    auto cellType = cells->GetType();
    for (size_t ice = 0; ice < nCells; ++ice) {
      Short_t cellNumber;
      Double_t amplitude;
      Double_t time;
      Int_t mclabel;
      Double_t efrac;

      cells->GetCell(ice, cellNumber, amplitude, time, mclabel, efrac);
      caloFiller(0, iev, cellNumber, amplitude, time, cellType);
    }

    // PHOS
    cells = esd->GetPHOSCells();
    nCells = cells->GetNumberOfCells();
    ncalo += nCells;
    cellType = cells->GetType();
    for (size_t icp = 0; icp < nCells; ++icp) {
      Short_t cellNumber;
      Double_t amplitude;
      Double_t time;
      Int_t mclabel;
      Double_t efrac;

      cells->GetCell(icp, cellNumber, amplitude, time, mclabel, efrac);
      caloFiller(0, iev, cellNumber, amplitude, time, cellType);
    }

    // Muon Tracks
    nmu = esd->GetNumberOfMuonTracks();
    for (size_t imu = 0; imu < nmu; ++imu) {
      AliESDMuonTrack* mutrk = esd->GetMuonTrack(imu);
      //
      //      TMatrixD cov;
      //      mutrk->GetCovariances(cov);
      //      for (Int_t i = 0; i < 5; i++)
      //        for (Int_t j = 0; j <= i; j++)
      //          fCovariances[i*(i+1)/2 + j] = cov(i,j);
      //      //
      //
      //
      muonFiller(
        0,
        iev,
        mutrk->GetInverseBendingMomentum(),
        mutrk->GetThetaX(),
        mutrk->GetThetaY(),
        mutrk->GetZ(),
        mutrk->GetBendingCoor(),
        mutrk->GetNonBendingCoor(),
        // covariance matrix goes here...
        mutrk->GetChi2(),
        mutrk->GetChi2MatchTrigger());
    }

    // VZERO
    AliESDVZERO* vz = esd->GetVZEROData();
    for (Int_t ich = 0; ich < 64; ++ich) {
    //  fAdcVZ[ich] = vz->GetAdc(ich);
    //  fTimeVZ[ich] = vz->GetTime(ich);
    //  fWidthVZ[ich] = vz->GetWidth(ich);
    }
    vzeroFiller(0, iev);
  } // Loop on events
  //
  std::vector<std::shared_ptr<arrow::Table>> tables;
  if (ntrk) {
    auto trackMetadata = std::make_shared<arrow::KeyValueMetadata>(std::vector<std::string>{"description"}, std::vector<std::string>{"TRACKPAR"});
    tables.emplace_back(trackParBuilder.finalize()->ReplaceSchemaMetadata(trackMetadata));
    auto trackCovMetadata = std::make_shared<arrow::KeyValueMetadata>(std::vector<std::string>{"description"}, std::vector<std::string>{"TRACKPARCOV"});
    tables.emplace_back(trackParCovBuilder.finalize()->ReplaceSchemaMetadata(trackCovMetadata));
    auto trackExtraMetadata = std::make_shared<arrow::KeyValueMetadata>(std::vector<std::string>{"description"}, std::vector<std::string>{"TRACKEXTRA"});
    tables.emplace_back(trackExtraBuilder.finalize()->ReplaceSchemaMetadata(trackExtraMetadata));
  }
  if (ncalo) {
    auto caloMetadata = std::make_shared<arrow::KeyValueMetadata>(std::vector<std::string>{"description"}, std::vector<std::string>{"CALO"});
    tables.emplace_back(caloBuilder.finalize()->ReplaceSchemaMetadata(caloMetadata));
  }
  if (nmu) {
    auto muonMetadata = std::make_shared<arrow::KeyValueMetadata>(std::vector<std::string>{"description"}, std::vector<std::string>{"MUON"});
    tables.emplace_back(muonBuilder.finalize()->ReplaceSchemaMetadata(muonMetadata));
  }
  if (nvzero) {
    auto vzeroMetadata = std::make_shared<arrow::KeyValueMetadata>(std::vector<std::string>{"description"}, std::vector<std::string>{"VZERO"});
    tables.emplace_back(v0Builder.finalize()->ReplaceSchemaMetadata(vzeroMetadata));
  }

  /// Writing to a stream
  for (auto &table : tables) {
    arrow::TableBatchReader reader(*table);
    std::shared_ptr<arrow::ipc::RecordBatchWriter> writer;
    auto outBatch = arrow::ipc::RecordBatchStreamWriter::Open(stream.get(), reader.schema(), &writer);
    if (outBatch.ok() == false) {
      std::runtime_error("Unable to open writer");
    }
    std::shared_ptr<arrow::RecordBatch> batch;
    while (true) {
      auto status = reader.ReadNext(&batch);
      if (status.ok() != true) {
        std::runtime_error("Error while processing table");
      }
      if (batch == nullptr) {
        break;
      }
      auto outStatus = writer->WriteRecordBatch(*batch);
    }
    if (writer->Close().ok() != true) {
      std::runtime_error("Unable to close file");
    }
  }
}

} // namespace run2
} // namespace framework
} // namespace o2
