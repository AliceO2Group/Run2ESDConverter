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
#include "Framework/AnalysisDataModel.h"
#include "Framework/TableBuilder.h"

#include "AliESDCaloCells.h"
#include "AliESDEvent.h"
#include "AliESDMuonTrack.h"
#include "AliESDVZERO.h"
#include "AliESDVertex.h"
#include "AliESDtrack.h"
#include "AliExternalTrackParam.h"

#include <TTree.h>

#include <arrow/io/buffered.h>
#include <arrow/io/file.h>
#include <arrow/ipc/writer.h>
#include <arrow/util/io-util.h>
#include <arrow/util/key_value_metadata.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

template class std::shared_ptr<arrow::Table>;

namespace o2::framework::run2 {

template <typename T>
void appendTable(std::vector<std::shared_ptr<arrow::Table>> &tables,
                 TableBuilder &builder) {
  using metadata = typename aod::MetadataTrait<std::decay_t<T>>::metadata;
  auto metadataKeys = std::vector<std::string>{"description"};
  auto metadataValues = std::vector<std::string>{metadata::description()};
  auto schemaMetadata =
      std::make_shared<arrow::KeyValueMetadata>(metadataKeys, metadataValues);
  tables.emplace_back(
      builder.finalize()->ReplaceSchemaMetadata(schemaMetadata));
}

void Run3AODConverter::convert(TTree *tEsd,
                               std::shared_ptr<arrow::io::OutputStream> stream,
                               size_t nEvents) {
  TableBuilder trackParBuilder;
  TableBuilder trackParCovBuilder;
  TableBuilder trackExtraBuilder;
  TableBuilder caloBuilder;
  TableBuilder muonBuilder;
  TableBuilder v0Builder;
  TableBuilder collisionsBuilder;
  TableBuilder timeframeBuilder;

  auto trackFiller = trackParBuilder.cursor<aod::Tracks>();
  auto sigmaFiller = trackParCovBuilder.cursor<aod::TracksCov>();
  auto extraFiller = trackExtraBuilder.cursor<aod::TracksExtra>();
  auto caloFiller = caloBuilder.cursor<aod::Calos>();
  auto muonFiller = muonBuilder.cursor<aod::Muons>();
  auto vzeroFiller = v0Builder.cursor<aod::VZeros>();
  auto collisionFiller = collisionsBuilder.cursor<aod::Collisions>();
  auto timeframeFiller = timeframeBuilder.cursor<aod::Timeframes>();

  AliESDEvent *esd = new AliESDEvent();
  esd->ReadFromTree(tEsd);
  size_t nev = tEsd->GetEntries();
  if ((nEvents > 0) && (nEvents < nev)) {
    nev = nEvents;
  }
  size_t ntrk = 0;
  size_t nmu = 0;
  size_t ncalo = 0;
  size_t nvzero = 0;
  // FIXME: what should we put as a timestamp for the timeframe??
  timeframeFiller(0, 0);

  for (size_t iev = 0; iev < nev; ++iev) {
    esd->Reset();
    tEsd->GetEntry(iev);
    esd->ConnectTracks();

    // Tracks information
    ntrk = esd->GetNumberOfTracks();
    for (size_t itrk = 0; itrk < ntrk; ++itrk) {
      AliESDtrack *track = esd->GetTrack(itrk);
      track->SetESDEvent(esd);
      trackFiller(0, iev, track->GetX(), track->GetAlpha(), track->GetY(),
                  track->GetZ(), track->GetSnp(), track->GetTgl(),
                  track->GetSigned1Pt());

      sigmaFiller(
          0, track->GetSigmaY2(), track->GetSigmaZY(), track->GetSigmaZ2(),
          track->GetSigmaSnpY(), track->GetSigmaSnpZ(), track->GetSigmaSnp2(),
          track->GetSigmaTglY(), track->GetSigmaTglZ(), track->GetSigmaTglSnp(),
          track->GetSigmaTgl2(), track->GetSigma1PtY(), track->GetSigma1PtZ(),
          track->GetSigma1PtSnp(), track->GetSigma1PtTgl(),
          track->GetSigma1Pt2());
      const AliExternalTrackParam *intp = track->GetTPCInnerParam();

      extraFiller(
          0,
          (intp ? intp->GetP()
                : 0), // Set the momentum to 0 if the track did not reach TPC //
          track->GetStatus(),
          //
          track->GetITSClusterMap(), track->GetTPCNcls(),
          track->GetTRDntracklets(),
          //
          (track->GetITSNcls() ? track->GetITSchi2() / track->GetITSNcls() : 0),
          (track->GetTPCNcls() ? track->GetTPCchi2() / track->GetTPCNcls() : 0),
          track->GetTRDchi2(), track->GetTOFchi2(),
          //
          track->GetTPCsignal(), track->GetTRDsignal(), track->GetTOFsignal(),
          track->GetIntegratedLength());
    } // End loop on tracks

    // Calorimeters:
    // EMCAL
    AliESDCaloCells *cells = esd->GetEMCALCells();
    size_t nCells = cells->GetNumberOfCells();
    ncalo += nCells;
    auto cellType = cells->GetType();
    // FIXME: this should retrieve the caloType
    auto caloType = 0;
    for (size_t ice = 0; ice < nCells; ++ice) {
      Short_t cellNumber;
      Double_t amplitude;
      Double_t time;
      Int_t mclabel;
      Double_t efrac;

      cells->GetCell(ice, cellNumber, amplitude, time, mclabel, efrac);
      caloFiller(0, iev, cellNumber, amplitude, time, cellType, caloType);
    }

    // PHOS
    cells = esd->GetPHOSCells();
    nCells = cells->GetNumberOfCells();
    ncalo += nCells;
    cellType = cells->GetType();
    caloType = 0;
    for (size_t icp = 0; icp < nCells; ++icp) {
      Short_t cellNumber;
      Double_t amplitude;
      Double_t time;
      Int_t mclabel;
      Double_t efrac;

      cells->GetCell(icp, cellNumber, amplitude, time, mclabel, efrac);
      caloFiller(0, iev, cellNumber, amplitude, time, caloType, cellType);
    }

    // Muon Tracks
    nmu = esd->GetNumberOfMuonTracks();
    for (size_t imu = 0; imu < nmu; ++imu) {
      AliESDMuonTrack *mutrk = esd->GetMuonTrack(imu);
      //
      //      TMatrixD cov;
      //      mutrk->GetCovariances(cov);
      //      for (Int_t i = 0; i < 5; i++)
      //        for (Int_t j = 0; j <= i; j++)
      //          fCovariances[i*(i+1)/2 + j] = cov(i,j);
      //      //
      //
      //
      muonFiller(0, iev, mutrk->GetInverseBendingMomentum(), mutrk->GetThetaX(),
                 mutrk->GetThetaY(), mutrk->GetZ(), mutrk->GetBendingCoor(),
                 mutrk->GetNonBendingCoor(),
                 // covariance matrix goes here...
                 mutrk->GetChi2(), mutrk->GetChi2MatchTrigger());
    }

    // VZERO
    AliESDVZERO *vz = esd->GetVZEROData();
    for (Int_t ich = 0; ich < 64; ++ich) {
      //  fAdcVZ[ich] = vz->GetAdc(ich);
      //  fTimeVZ[ich] = vz->GetTime(ich);
      //  fWidthVZ[ich] = vz->GetWidth(ich);
    }
    vzeroFiller(0, iev, 0, 0);
    AliESDVertex const *vertex = esd->GetVertex();
    // FIXME: timeframeid is dummy
    // FIXME: last few entries are obviously dummy
    collisionFiller(0, 0, ntrk, iev, vertex->GetX(), vertex->GetY(),
                    vertex->GetZ(), vertex->GetChi2(), vertex->GetBC(), 0, 0, 0, 
                    0, 0, 0, 0);
  } // Loop on events
  //
  std::vector<std::shared_ptr<arrow::Table>> tables;
  if (ntrk) {
    appendTable<aod::Tracks>(tables, trackParBuilder);
    appendTable<aod::TracksCov>(tables, trackParCovBuilder);
    appendTable<aod::TracksExtra>(tables, trackExtraBuilder);
  }
  if (ncalo) {
    appendTable<aod::Calos>(tables, caloBuilder);
  }
  if (nmu) {
    appendTable<aod::Muons>(tables, muonBuilder);
  }
  if (nvzero) {
    appendTable<aod::VZeros>(tables, v0Builder);
  }

  if (nev) {
    appendTable<aod::Collisions>(tables, collisionsBuilder);
  }

  appendTable<aod::Timeframes>(tables, timeframeBuilder);

  /// Writing to a stream
  for (auto &table : tables) {
    std::unordered_map<std::string, std::string> meta;
    table->schema()->metadata()->ToUnorderedMap(&meta);
    std::cerr << "Writing table: " << meta["description"] << " ... ";
    arrow::TableBatchReader reader(*table);
    std::shared_ptr<arrow::ipc::RecordBatchWriter> writer;
    auto outBatch = arrow::ipc::RecordBatchStreamWriter::Open(
        stream.get(), reader.schema(), &writer);
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
      // Align the stream to 8 bytes, as requested by Arrow
      auto outStatus = writer->WriteRecordBatch(*batch);
    }
    if (writer->Close().ok() != true) {
      std::runtime_error("Unable to close file");
    }
    std::cerr << "[DONE]" << std::endl;
    int64_t pos;
    stream->Tell(&pos);
    if (pos % 8 != 0) {
      int64_t extra = 0;
      stream->Write(&extra, 8 - (pos % 8));
      std::cerr << "moving stream " << 8 - (pos % 8)
                << " positions to align ... " << std::endl;
    }
  }
}

} // namespace o2::framework::run2
