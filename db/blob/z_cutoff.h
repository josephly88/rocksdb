// Copyright (c) 2011-present, Facebook, Inc. All rights reserved.
// This source code is licensed under both the GPLv2 (found in the
// COPYING file in the root directory) and Apache 2.0 License
// (found in the LICENSE.Apache file in the root directory).

#pragma once

#include <stddef.h>
#include <stdint.h>

#include "rocksdb/file_system.h"

namespace ROCKSDB_NAMESPACE {

// Paper Equation (1), MASCOTS 2024 Same-Victim GC:
//   cutoff ratio = (min_Z(YBIZ_Z) - min(blob file index)) / N
// The victim count is that numerator. Both YBIZ and the oldest live blob
// index are dense blob creation indexes (1, 2, 3, ...), not RocksDB file
// numbers. Returns false when the caller should keep the age cutoff.
inline bool ZCutoffVictimCount(uint64_t min_ybiz, uint64_t min_index, size_t n,
                               size_t* cutoff_index) {
  if (cutoff_index == nullptr || n == 0 || min_ybiz <= min_index) {
    return false;
  }
  uint64_t numer = min_ybiz - min_index;
  if (numer > n) {
    numer = n;
  }
  if (numer == 0) {
    return false;
  }
  *cutoff_index = static_cast<size_t>(numer);
  return true;
}

// Looks up min YBIZ and the oldest live blob's creation index, then applies
// Equation (1). blob_files must be sorted by file number and non-empty.
template <typename BlobFileList>
inline bool TryZCutoffVictimCount(FileSystem* fs, const BlobFileList& blob_files,
                                  size_t* cutoff_index) {
  if (fs == nullptr || blob_files.empty() || blob_files.front() == nullptr) {
    return false;
  }
  uint64_t min_ybiz = 0;
  if (!fs->GetMinFullBlobYBIZ(&min_ybiz).ok()) {
    return false;
  }
  uint64_t min_index = 0;
  if (!fs->GetBlobCreationIndex(blob_files.front()->GetBlobFileNumber(),
                                &min_index)
           .ok()) {
    return false;
  }
  return ZCutoffVictimCount(min_ybiz, min_index, blob_files.size(),
                            cutoff_index);
}

}  // namespace ROCKSDB_NAMESPACE
