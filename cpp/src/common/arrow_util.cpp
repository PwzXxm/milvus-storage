// Copyright 2023 Zilliz
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "common/arrow_util.h"
#include "common/macro.h"
#include "common/utils.h"
#include <arrow/record_batch.h>
#include <arrow/array.h>

namespace milvus_storage {
Result<std::unique_ptr<parquet::arrow::FileReader>> MakeArrowFileReader(arrow::fs::FileSystem& fs,
                                                                        const std::string& file_path) {
  ASSIGN_OR_RETURN_ARROW_NOT_OK(auto file, fs.OpenInputFile(file_path));

  std::unique_ptr<parquet::arrow::FileReader> file_reader;
  RETURN_ARROW_NOT_OK(parquet::arrow::OpenFile(file, arrow::default_memory_pool(), &file_reader));
  return std::move(file_reader);
}

Result<std::unique_ptr<parquet::arrow::FileReader>> MakeArrowFileReader(
    arrow::fs::FileSystem& fs, const std::string& file_path, const parquet::ArrowReaderProperties& read_properties) {
  ASSIGN_OR_RETURN_ARROW_NOT_OK(auto file, fs.OpenInputFile(file_path));
  parquet::arrow::FileReaderBuilder builder;
  std::unique_ptr<parquet::arrow::FileReader> reader;
  RETURN_ARROW_NOT_OK(builder.Open(std::move(file)));
  RETURN_ARROW_NOT_OK(builder.memory_pool(arrow::default_memory_pool())->properties(read_properties)->Build(&reader));
  return std::move(reader);
}

Result<std::unique_ptr<arrow::RecordBatchReader>> MakeArrowRecordBatchReader(parquet::arrow::FileReader& reader,
                                                                             std::shared_ptr<arrow::Schema> schema,
                                                                             const SchemaOptions& schema_options,
                                                                             const ReadOptions& options) {
  auto internal_options = CreateInternalReadOptions(schema, schema_options, options);
  auto metadata = reader.parquet_reader()->metadata();
  std::vector<int> row_group_indices;
  std::set<int> column_indices;

  for (const auto& column_name : internal_options.columns) {
    auto column_idx = metadata->schema()->ColumnIndex(column_name);
    column_indices.insert(column_idx);
  }
  for (const auto& filter : internal_options.filters) {
    auto column_idx = metadata->schema()->ColumnIndex(filter->get_column_name());
    column_indices.insert(column_idx);
  }

  for (int i = 0; i < metadata->num_row_groups(); ++i) {
    auto row_group_metadata = metadata->RowGroup(i);
    bool can_ignored = false;

    for (const auto& filter : internal_options.filters) {
      auto column_idx = metadata->schema()->ColumnIndex(filter->get_column_name());
      auto column_meta = row_group_metadata->ColumnChunk(column_idx);
      auto stats = column_meta->statistics();

      if (stats == nullptr || !stats->HasMinMax()) {
        continue;
      }
      if (filter->CheckStatistics(stats.get())) {
        can_ignored = true;
        break;
      }
    }
    if (!can_ignored) {
      row_group_indices.emplace_back(i);
    }
  }
  std::unique_ptr<arrow::RecordBatchReader> record_reader;

  RETURN_ARROW_NOT_OK(
      reader.GetRecordBatchReader(row_group_indices, {column_indices.begin(), column_indices.end()}, &record_reader));
  return record_reader;
}

size_t GetRecordBatchMemorySize(const std::shared_ptr<arrow::RecordBatch>& record_batch) {
  size_t total_size = 0;
  for (const auto& column : record_batch->columns()) {
    total_size += GetArrowArrayMemorySize(column);
  }
  return total_size;
}

size_t GetArrowArrayMemorySize(const std::shared_ptr<arrow::Array>& array) {
  if (!array || !array->data()) {
    return 0;
  }
  size_t total_size = 0;
  for (const auto& buffer : array->data()->buffers) {
    if (buffer) {
      total_size += buffer->size();
    }
  }
  return total_size;
}

}  // namespace milvus_storage
