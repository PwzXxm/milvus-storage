#pragma once

#include "filter/filter.h"
#include "proto/manifest.pb.h"

namespace milvus_storage {

struct WriteOption {
  int64_t max_record_per_file = 1024;
};

struct ReadOptions {
  std::vector<Filter*> filters;
  std::vector<std::string> columns;  // must have pk and version
  int limit = -1;
  int version = -1;
};

struct SpaceOptions {
  std::string uri;

  std::unique_ptr<manifest_proto::SpaceOptions> ToProtobuf();

  void FromProtobuf(const manifest_proto::SpaceOptions& options);
};

struct SchemaOptions {
  Status Validate(const arrow::Schema* schema);

  bool has_version_column() const { return !version_column.empty(); }

  std::unique_ptr<schema_proto::SchemaOptions> ToProtobuf();

  void FromProtobuf(const schema_proto::SchemaOptions& options);

  // primary_column must not be empty and the type must be int64 or string
  std::string primary_column;
  // version_column is to support mvcc and it can be set in ReadOption.
  // it can be empty and if not, the column type must be int64
  std::string version_column;
  // vector_column must not be emtpy, and the type must be fixed size binary
  std::string vector_column;
};

}  // namespace milvus_storage