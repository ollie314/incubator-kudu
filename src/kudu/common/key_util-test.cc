// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include "kudu/common/key_util.h"

#include <gtest/gtest.h>

#include "kudu/common/partial_row.h"
#include "kudu/common/row.h"
#include "kudu/common/schema.h"
#include "kudu/gutil/mathlimits.h"
#include "kudu/util/test_util.h"

namespace kudu {

class KeyUtilTest : public KuduTest {
 public:
  KeyUtilTest()
    : arena_(1024, 4096) {}

 protected:
  uint8_t* row_data(KuduPartialRow* row) {
    return row->row_data_;
  }

  Arena arena_;
};

TEST_F(KeyUtilTest, TestIncrementNonCompositePrimaryKey) {
  Schema schema({ ColumnSchema("key", INT32),
                  ColumnSchema("other_col", INT32),
                  ColumnSchema("other_col2", STRING, true) },
                1);
  KuduPartialRow p_row(&schema);
  ContiguousRow row(&schema, row_data(&p_row));

  // Normal increment.
  EXPECT_OK(p_row.SetInt32(0, 1000));
  EXPECT_TRUE(key_util::IncrementPrimaryKey(&row, &arena_));
  EXPECT_EQ("int32 key=1001", p_row.ToString());

  // Overflow increment.
  EXPECT_OK(p_row.SetInt32(0, MathLimits<int32_t>::kMax));
  EXPECT_FALSE(key_util::IncrementPrimaryKey(&row, &arena_));
  EXPECT_EQ("int32 key=-2147483648", p_row.ToString());
}

TEST_F(KeyUtilTest, TestIncrementCompositePrimaryKey) {
  Schema schema({ ColumnSchema("k1", INT32),
                  ColumnSchema("k2", INT32),
                  ColumnSchema("other_col", STRING, true) },
                2);

  KuduPartialRow p_row(&schema);
  ContiguousRow row(&schema, row_data(&p_row));

  // Normal increment.
  EXPECT_OK(p_row.SetInt32(0, 1000));
  EXPECT_OK(p_row.SetInt32(1, 1000));
  EXPECT_TRUE(key_util::IncrementPrimaryKey(&row, &arena_));
  EXPECT_EQ("int32 k1=1000, int32 k2=1001", p_row.ToString());

  // Overflow a later part of the key, carrying into the earlier
  // part..
  EXPECT_OK(p_row.SetInt32(1, MathLimits<int32_t>::kMax));
  EXPECT_TRUE(key_util::IncrementPrimaryKey(&row, &arena_));
  EXPECT_EQ("int32 k1=1001, int32 k2=-2147483648", p_row.ToString());

  // Overflow the whole key.
  EXPECT_OK(p_row.SetInt32(0, MathLimits<int32_t>::kMax));
  EXPECT_OK(p_row.SetInt32(1, MathLimits<int32_t>::kMax));
  EXPECT_FALSE(key_util::IncrementPrimaryKey(&row, &arena_));
  EXPECT_EQ("int32 k1=-2147483648, int32 k2=-2147483648", p_row.ToString());
}

TEST_F(KeyUtilTest, TestIncrementCompositeIntStringPrimaryKey) {
  Schema schema({ ColumnSchema("k1", INT32),
                  ColumnSchema("k2", STRING),
                  ColumnSchema("other_col", STRING, true) },
                2);

  KuduPartialRow p_row(&schema);
  ContiguousRow row(&schema, row_data(&p_row));

  // Normal increment.
  EXPECT_OK(p_row.SetInt32(0, 1000));
  EXPECT_OK(p_row.SetStringNoCopy(1, "hello"));
  EXPECT_TRUE(key_util::IncrementPrimaryKey(&row, &arena_));
  EXPECT_EQ("int32 k1=1000, string k2=hello\\000", p_row.ToString());

  // There's no way to overflow a string key - you can always make it higher
  // by tacking on more \x00.
  EXPECT_TRUE(key_util::IncrementPrimaryKey(&row, &arena_));
  EXPECT_EQ("int32 k1=1000, string k2=hello\\000\\000", p_row.ToString());
}

TEST_F(KeyUtilTest, TestIncrementCompositeStringIntPrimaryKey) {
  Schema schema({ ColumnSchema("k1", STRING),
                  ColumnSchema("k2", INT32),
                  ColumnSchema("other_col", STRING, true) },
                2);

  KuduPartialRow p_row(&schema);
  ContiguousRow row(&schema, row_data(&p_row));

  // Normal increment.
  EXPECT_OK(p_row.SetStringNoCopy(0, "hello"));
  EXPECT_OK(p_row.SetInt32(1, 1000));
  EXPECT_TRUE(key_util::IncrementPrimaryKey(&row, &arena_));
  EXPECT_EQ("string k1=hello, int32 k2=1001", p_row.ToString());

  // Overflowing the int32 portion should tack \x00 onto the
  // string portion.
  EXPECT_OK(p_row.SetInt32(1, MathLimits<int32_t>::kMax));
  EXPECT_TRUE(key_util::IncrementPrimaryKey(&row, &arena_));
  EXPECT_EQ("string k1=hello\\000, int32 k2=-2147483648", p_row.ToString());
}

} // namespace kudu
