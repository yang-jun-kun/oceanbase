/**
 * Copyright (c) 2021 OceanBase
 * OceanBase CE is licensed under Mulan PubL v2.
 * You can use this software according to the terms and conditions of the Mulan PubL v2.
 * You may obtain a copy of Mulan PubL v2 at:
 *          http://license.coscl.org.cn/MulanPubL-2.0
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PubL v2 for more details.
 */

#ifndef OCEABASE_SHARE_SCN_
#define OCEABASE_SHARE_SCN_
#include "lib/ob_define.h"                      // Serialization
#include "lib/utility/ob_print_utils.h"         // Print*
namespace oceanbase {
namespace share {

const uint64_t OB_INVALID_SCN_VAL = UINT64_MAX;
const uint64_t OB_MIN_SCN_TS_NS = 0;
const uint64_t OB_BASE_SCN_TS_NS = 1;
const uint64_t OB_MAX_SCN_TS_NS = (1UL << 62) - 1;     //4611686018427387903

class SCN
{
public:
  SCN() : val_(OB_INVALID_SCN_VAL) {}
  void reset();
  bool is_valid() const;
  bool is_valid_and_not_min() const;
  bool is_max() const;
  bool is_min() const;
  bool is_base_scn() const { return *this == base_scn(); }
  void set_invalid();
  void set_max();
  void set_min();
  void set_base();
  static SCN invalid_scn();
  static SCN max_scn();
  static SCN min_scn();
  static SCN base_scn();
  static SCN max(const SCN &left, const SCN &right);
  static SCN min(const SCN &left, const SCN &right);
  static SCN plus(const SCN &ref, uint64_t delta);
  static SCN minus(const SCN &ref, uint64_t delta);
  static SCN scn_inc(const SCN &ref);
  static SCN scn_dec(const SCN &ref);

  void atomic_store(const SCN &ref);
  void atomic_set(const SCN &ref);
  SCN atomic_get() const;
  bool atomic_bcas(const SCN &old_v, const SCN &new_val);
  SCN atomic_vcas(const SCN &old_v, const SCN &new_val);
  SCN inc_update(const SCN &ref_scn);
  SCN dec_update(const SCN &ref_scn);
  /*******convert functions******/
  //[convert scn to timestamp(us)
  // @result[out]: timestamp in us
  int64_t convert_to_ts(bool ignore_invalid = false) const;

  // @param[in] :timestamp with us
  int convert_from_ts(uint64_t ts_us);

  // convert time_ns generated by gts to scn. only used by gts
  // @param[in] time_ns: time_ns generated by gts
  int convert_for_gts(int64_t time_ns);

  // convert id generated by lsn allocator. only used by logservice
  // @param[in] id: id generated by lsn allocator
  int convert_for_logservice(uint64_t scn_val);

  // convert scn_related column value to scn. only used when extracting inner table query result
  // @param[in] column_value: query result of scn_related column of inner table
  int convert_for_inner_table_field(uint64_t column_value);

  //convert function to sql
  int convert_for_sql(uint64_t scn_val);

  // @param[in] convert for tx: INT64_MAX may convert to MAX_SCN
  int convert_for_tx(int64_t commit_trans_version);

  // change val_ from INT64_MAX to OB_MAX_SCN_TS_NS for compitibility in
  // deserialization

  //only for filling inner_table fields
  uint64_t get_val_for_inner_table_field() const;

  //only for gts use
  uint64_t get_val_for_gts() const;

  //only for log service use
  uint64_t get_val_for_logservice() const;
  //only for sql use
  uint64_t get_val_for_sql() const;

  //only for tx, include transform from SCN_MAX to INT64_MAX
  int64_t get_val_for_tx() const;

  // compare function
  bool operator==(const SCN &scn) const;
  bool operator!=(const SCN &scn) const;
  bool operator<(const SCN &scn) const;
  bool operator<=(const SCN &scn) const;
  bool operator>(const SCN &scn) const;
  bool operator>=(const SCN &scn) const;
  SCN &operator=(const SCN &scn);

  //fixed length serialization
  int fixed_serialize(char* buf, const int64_t buf_len, int64_t& pos) const;
  int fixed_deserialize(const char* buf, const int64_t data_len, int64_t& pos);
  int64_t get_fixed_serialize_size(void) const;

  //variable serialization
  int serialize(char* buf, const int64_t buf_len, int64_t& pos) const;
  int deserialize(const char* buf, const int64_t data_len, int64_t& pos);
  int64_t get_serialize_size(void) const;
  int to_yson(char *buf, const int64_t buf_len, int64_t &pos) const;

  TO_STRING_KV(K_(val));
private:
  void transform_max_();
private:
  static const uint64_t SCN_VERSION = 0;
  union {
    uint64_t val_;
    struct {
      uint64_t ts_ns_ : 62;
      uint64_t v_ : 2;
    };
  };
};

} // end namespace share
} // end namespace oceanbase
#endif
