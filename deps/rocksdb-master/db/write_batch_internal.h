//  Copyright (c) 2013, Facebook, Inc.  All rights reserved.
//  This source code is licensed under the BSD-style license found in the
//  LICENSE file in the root directory of this source tree. An additional grant
//  of patent rights can be found in the PATENTS file in the same directory.
//
// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#pragma once
#include "rocksdb/types.h"
#include "rocksdb/write_batch.h"
#include "rocksdb/db.h"
#include "rocksdb/options.h"

namespace rocksdb {

class MemTable;

class ColumnFamilyMemTables {
 public:
  virtual ~ColumnFamilyMemTables() {}
  virtual bool Seek(uint32_t column_family_id) = 0;
  // returns true if the update to memtable should be ignored
  // (useful when recovering from log whose updates have already
  // been processed)
  virtual uint64_t GetLogNumber() const = 0;
  virtual MemTable* GetMemTable() const = 0;
  virtual const Options* GetOptions() const = 0;
  virtual ColumnFamilyHandle* GetColumnFamilyHandle() = 0;
};

class ColumnFamilyMemTablesDefault : public ColumnFamilyMemTables {
 public:
  ColumnFamilyMemTablesDefault(MemTable* mem, const Options* options)
      : ok_(false), mem_(mem), options_(options) {}

  bool Seek(uint32_t column_family_id) override {
    ok_ = (column_family_id == 0);
    return ok_;
  }

  uint64_t GetLogNumber() const override { return 0; }

  MemTable* GetMemTable() const override {
    assert(ok_);
    return mem_;
  }

  const Options* GetOptions() const override {
    assert(ok_);
    return options_;
  }

  ColumnFamilyHandle* GetColumnFamilyHandle() override { return nullptr; }

 private:
  bool ok_;
  MemTable* mem_;
  const Options* const options_;
};

// WriteBatchInternal provides static methods for manipulating a
// WriteBatch that we don't want in the public WriteBatch interface.
class WriteBatchInternal {
 public:
  // WriteBatch methods with column_family_id instead of ColumnFamilyHandle*
  static void Put(WriteBatch* batch, uint32_t column_family_id,
                  const Slice& key, const Slice& value);

  static void Put(WriteBatch* batch, uint32_t column_family_id,
                  const SliceParts& key, const SliceParts& value);

  static void Delete(WriteBatch* batch, uint32_t column_family_id,
                     const SliceParts& key);

  static void Delete(WriteBatch* batch, uint32_t column_family_id,
                     const Slice& key);

  static void Merge(WriteBatch* batch, uint32_t column_family_id,
                    const Slice& key, const Slice& value);

  // Return the number of entries in the batch.
  static int Count(const WriteBatch* batch);

  // Set the count for the number of entries in the batch.
  static void SetCount(WriteBatch* batch, int n);

  // Return the seqeunce number for the start of this batch.
  static SequenceNumber Sequence(const WriteBatch* batch);

  // Store the specified number as the seqeunce number for the start of
  // this batch.
  static void SetSequence(WriteBatch* batch, SequenceNumber seq);

  static Slice Contents(const WriteBatch* batch) {
    return Slice(batch->rep_);
  }

  static size_t ByteSize(const WriteBatch* batch) {
    return batch->rep_.size();
  }

  static void SetContents(WriteBatch* batch, const Slice& contents);

  // Inserts batch entries into memtable
  // If dont_filter_deletes is false AND options.filter_deletes is true,
  // then --> Drops deletes in batch if db->KeyMayExist returns false
  // If recovery == true, this means InsertInto is executed on a recovery
  // code-path. WriteBatch referencing a dropped column family can be
  // found on a recovery code-path and should be ignored (recovery should not
  // fail). Additionally, the memtable will be updated only if
  // memtables->GetLogNumber() >= log_number
  // However, if recovery == false, any WriteBatch referencing
  // non-existing column family will return a failure. Also, log_number is
  // ignored in that case
  static Status InsertInto(const WriteBatch* batch,
                           ColumnFamilyMemTables* memtables,
                           bool recovery = false, uint64_t log_number = 0,
                           DB* db = nullptr,
                           const bool dont_filter_deletes = true);

  static void Append(WriteBatch* dst, const WriteBatch* src);
};

}  // namespace rocksdb
