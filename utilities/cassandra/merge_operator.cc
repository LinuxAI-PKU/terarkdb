//  Copyright (c) 2017-present, Facebook, Inc.  All rights reserved.
//  This source code is licensed under both the GPLv2 (found in the
//  COPYING file in the root directory) and Apache 2.0 License
//  (found in the LICENSE.Apache file in the root directory).

#include "merge_operator.h"

#include <memory>
#include <assert.h>

#include "rocksdb/slice.h"
#include "rocksdb/merge_operator.h"
#include "utilities/merge_operators.h"
#include "utilities/cassandra/format.h"

namespace rocksdb {
namespace cassandra {

// Implementation for the merge operation (merges two Cassandra values)
bool CassandraValueMergeOperator::FullMergeV2(
    const MergeOperationInput& merge_in,
    MergeOperationOutput* merge_out) const {
  // Clear the *new_value for writing.
  merge_out->new_value.clear();
  std::vector<RowValue> row_values;
  if (merge_in.existing_value) {
    if (!merge_in.existing_value->decode().ok()) {
      return false;
    }
    row_values.push_back(
        RowValue::Deserialize(merge_in.existing_value->data(),
                              merge_in.existing_value->size()));
  }

  for (auto& operand : merge_in.operand_list) {
    if (!operand.decode().ok()) {
      return false;
    }
    row_values.push_back(RowValue::Deserialize(operand.data(),
                                               operand.size()));
  }

  RowValue merged = RowValue::Merge(std::move(row_values));
  merged = merged.RemoveTombstones(gc_grace_period_in_seconds_);

  std::string* buffer = merge_out->new_value.trans_to_buffer();
  buffer->reserve(merged.Size());
  merged.Serialize(buffer);

  return true;
}

bool CassandraValueMergeOperator::PartialMergeMulti(
    const Slice& /*key*/, const std::vector<LazySlice>& operand_list,
    LazySlice* new_value, Logger* /*logger*/) const {
  // Clear the *new_value for writing.
  assert(new_value);
  new_value->clear();

  std::vector<RowValue> row_values;
  for (auto& operand : operand_list) {
    if (!operand.decode().ok()) {
      return false;
    }
    row_values.push_back(RowValue::Deserialize(operand.data(),
                                               operand.size()));
  }
  RowValue merged = RowValue::Merge(std::move(row_values));

  std::string* buffer = new_value->trans_to_buffer();
  buffer->reserve(merged.Size());
  merged.Serialize(buffer);
  return true;
}

const char* CassandraValueMergeOperator::Name() const  {
  return "CassandraValueMergeOperator";
}

} // namespace cassandra

} // namespace rocksdb
