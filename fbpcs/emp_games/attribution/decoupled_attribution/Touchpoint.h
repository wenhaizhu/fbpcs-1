/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <stdexcept>
#include <string>

#include <emp-sh2pc/emp-sh2pc.h>
#include "folly/logging/xlog.h"

#include "fbpcs/emp_games/attribution/decoupled_attribution/Constants.h"
#include "fbpcs/emp_games/attribution/decoupled_attribution/Timestamp.h"

namespace aggregation::private_attribution {

struct Touchpoint {
  int64_t id;
  bool isClick;
  int64_t ts;

  // privatelyShareArrayFrom support
  friend bool operator==(const Touchpoint& a, const Touchpoint& b) {
    return a.id == b.id;
  }
  friend std::ostream& operator<<(std::ostream& os, const Touchpoint& tp) {
    return os << (tp.isClick ? "Click{" : "View{") << "id=" << tp.id
              << ", ts=" << tp.ts << "}";
  }

  /**
   * If both are clicks, or both are views, the earliest one comes first.
   * If one is a click but the other is a view, the view comes first.
  */
  bool operator<(const Touchpoint& tp) const {
    return (isClick == tp.isClick) ? (ts < tp.ts) : !isClick;
  }
};

struct PrivateTouchpoint {
  emp::Bit isClick;
  aggregation::private_attribution::Timestamp ts;
  emp::Integer id;

#define EMP_BIT_SIZE (static_cast<int>(emp::Bit::bool_size()))

  explicit PrivateTouchpoint(
      const emp::Bit& _isClick,
      const Timestamp& _ts,
      const emp::Integer& _id)
      : isClick{_isClick}, ts{_ts}, id{_id} {}

  explicit PrivateTouchpoint()
      : isClick{false, emp::PUBLIC},
        ts{-1},
        id{INT_SIZE, INVALID_TP_ID, emp::PUBLIC} {}

  // emp::batcher based construction support
  explicit PrivateTouchpoint(int len, const emp::block* b)
      // constructor for emp::Bit function happens to be a const emp::block&,
      // rather than emp::block* like other emp primitives. Making an explicit
      // static+cast is required for the compiler to select the right
      // constructor (otherwise the empty constructor is used).
      : isClick{static_cast<const emp::block&>(*b)},
        // TODO there has to be a better way to do this addition, rather than
        // being forced to do it all inline?
        ts{b + EMP_BIT_SIZE},
        id{INT_SIZE, b + EMP_BIT_SIZE + ts.length()} {}

  PrivateTouchpoint select(const emp::Bit& useRhs, const PrivateTouchpoint& rhs)
      const {
    return PrivateTouchpoint{
        /* isClick */ isClick.select(useRhs, rhs.isClick),
        /* ts */ ts.select(useRhs, rhs.ts),
        /* id */ id.select(useRhs, rhs.id)};
  }

  // Checking if timestamp > 0
  emp::Bit isValid() const {
    const Timestamp one{1};
    return ts >= one;
  }

  // string conversion support
  template <typename T = std::string>
  T reveal(int party) const {
    std::stringstream out;

    out << (isClick.reveal<bool>(party) ? "Click{" : "View{");
    out << "id=";
    out << id.reveal<std::string>(party);
    out << ", ts=";
    out << ts.reveal<int64_t>(party);
    out << "}";

    return out.str();
  }

  // emp::batcher serialization support
  template <typename... Args>
  static size_t bool_size(Args...) {
    return emp::Bit::bool_size() + Timestamp::bool_size() +
        emp::Integer::bool_size(INT_SIZE, 0 /* dummy value */);
  }

  // emp::batcher serialization support
  static void bool_data(bool* data, const Touchpoint& tp) {
    auto offset = 0;

    emp::Bit::bool_data(data, tp.isClick);
    offset += emp::Bit::bool_size();

    Timestamp::bool_data(data + offset, tp.ts);
    offset += Timestamp::bool_size();

    emp::Integer::bool_data(data + offset, INT_SIZE, tp.id);
    offset += emp::Integer::bool_size(INT_SIZE, 0 /* dummy value */);
  }
};

} // namespace aggregation::private_attribution
