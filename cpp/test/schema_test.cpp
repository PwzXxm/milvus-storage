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

#include "milvus-storage/storage/schema.h"
#include "milvus-storage/common/constants.h"
#include "gtest/gtest.h"
#include "milvus-storage/storage/options.h"
#include "test_util.h"
#include <arrow/util/key_value_metadata.h>

namespace milvus_storage {

TEST(SchemaValidateTest, SchemaValidateNoVersionColTest) {
  // Create Fields
  auto metadata = arrow::KeyValueMetadata::Make(std::vector<std::string>{"key1", "key2"},
                                                std::vector<std::string>{"value1", "value2"});
  auto pk_field = arrow::field("pk_field", arrow::int64(), /*nullable=*/false, metadata);
  auto vec_field = arrow::field("vec_field", arrow::fixed_size_binary(10), /*nullable=*/false, metadata);

  // Create Arrow Schema
  arrow::SchemaBuilder schema_builder;
  auto status = schema_builder.AddField(pk_field);
  ASSERT_TRUE(status.ok());

  status = schema_builder.AddField(vec_field);
  ASSERT_TRUE(status.ok());
  auto schema_metadata =
      arrow::KeyValueMetadata(std::vector<std::string>{"key1", "key2"}, std::vector<std::string>{"value1", "value2"});
  auto metadata_status = schema_builder.AddMetadata(schema_metadata);
  ASSERT_TRUE(metadata_status.ok());
  auto arrow_schema = schema_builder.Finish().ValueOrDie();

  // Create Options
  SchemaOptions schema_options;
  schema_options.primary_column = "pk_field";
  schema_options.vector_column = "vec_field";

  // Create Schema
  auto space_schema1 = std::make_shared<Schema>(arrow_schema, schema_options);

  auto sp_status1 = space_schema1->Validate();
  ASSERT_TRUE(sp_status1.ok());

  auto scalar_schema = space_schema1->scalar_schema();
  /// scalar schema has no version column but has offset column
  ASSERT_EQ(scalar_schema->num_fields(), 2);
  ASSERT_EQ(scalar_schema->field(0)->name(), schema_options.primary_column);
  ASSERT_EQ(scalar_schema->field(1)->name(), kOffsetFieldName);

  auto vector_schema = space_schema1->vector_schema();
  ASSERT_EQ(vector_schema->num_fields(), 2);
  ASSERT_EQ(vector_schema->field(0)->name(), schema_options.primary_column);

  ASSERT_EQ(vector_schema->field(1)->name(), schema_options.vector_column);

  auto delete_schema = space_schema1->delete_schema();
  ASSERT_EQ(delete_schema->num_fields(), 1);
  ASSERT_EQ(delete_schema->field(0)->name(), schema_options.primary_column);
}

TEST(SchemaValidateTest, SchemaValidateVersionColTest) {
  // Create Fields
  auto metadata = arrow::KeyValueMetadata::Make(std::vector<std::string>{"key1", "key2"},
                                                std::vector<std::string>{"value1", "value2"});
  auto pk_field = arrow::field("pk_field", arrow::int64(), /*nullable=*/false, metadata);
  auto ts_field = arrow::field("ts_field", arrow::int64(), /*nullable=*/false, metadata);
  auto vec_field = arrow::field("vec_field", arrow::fixed_size_binary(10), /*nullable=*/false, metadata);

  // Create Arrow Schema
  arrow::SchemaBuilder schema_builder;
  auto status = schema_builder.AddField(pk_field);
  ASSERT_TRUE(status.ok());
  status = schema_builder.AddField(ts_field);
  ASSERT_TRUE(status.ok());
  status = schema_builder.AddField(vec_field);
  ASSERT_TRUE(status.ok());
  auto schema_metadata =
      arrow::KeyValueMetadata(std::vector<std::string>{"key1", "key2"}, std::vector<std::string>{"value1", "value2"});
  auto metadata_status = schema_builder.AddMetadata(schema_metadata);
  ASSERT_TRUE(metadata_status.ok());
  auto arrow_schema = schema_builder.Finish().ValueOrDie();

  // Create Options
  SchemaOptions schema_options;
  schema_options.primary_column = "pk_field";
  schema_options.version_column = "ts_field";
  schema_options.vector_column = "vec_field";

  // Create Schema
  auto space_schema1 = std::make_shared<Schema>(arrow_schema, schema_options);

  auto sp_status1 = space_schema1->Validate();
  ASSERT_TRUE(sp_status1.ok());

  auto scalar_schema = space_schema1->scalar_schema();
  ASSERT_EQ(scalar_schema->num_fields(), 3);
  ASSERT_EQ(scalar_schema->field(0)->name(), schema_options.primary_column);
  ASSERT_EQ(scalar_schema->field(1)->name(), schema_options.version_column);
  ASSERT_EQ(scalar_schema->field(2)->name(), kOffsetFieldName);

  auto vector_schema = space_schema1->vector_schema();
  ASSERT_EQ(vector_schema->num_fields(), 3);
  ASSERT_EQ(vector_schema->field(0)->name(), schema_options.primary_column);
  ASSERT_EQ(vector_schema->field(1)->name(), schema_options.version_column);
  ASSERT_EQ(vector_schema->field(2)->name(), schema_options.vector_column);

  auto delete_schema = space_schema1->delete_schema();
  ASSERT_EQ(delete_schema->num_fields(), 2);
  ASSERT_EQ(delete_schema->field(0)->name(), schema_options.primary_column);
  ASSERT_EQ(delete_schema->field(1)->name(), schema_options.version_column);
}

}  // namespace milvus_storage
