#pragma once

#include "termspp/common/result.hpp"
#include "termspp/mapper/constants.hpp"

#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace termspp {
namespace mapper {

/************************************************************
 *                                                          *
 *                           Data                           *
 *                                                          *
 ************************************************************/

/// Columns contained by a single row
typedef std::vector<std::string_view> MapCols;

/// Describes a parsed map row
///   - Used for structured binding of ColumnDelimiter policy
struct alignas(kMapRowAlignment) MapRow {
  MapCols        cols;
  uint64_t       size;
  common::Status status;
};

/// Describes a finalised map record
struct alignas(kMapRecordAlignment) MapRecord {
  char *uidBuf;
  char *srcBuf;
  char *trgBuf;
};

/// Predicate type for `FilterPolicy` policies
typedef bool (*MapPredicate)(MapRow &);

/// Record handler for `BuilderPolicy` policies
typedef bool (*MapBuilder)(const MapCols &, uint8_t *ptr, MapRecord &);

/************************************************************
 *                                                          *
 *                         Helpers                          *
 *                                                          *
 ************************************************************/
/// Filter columns by index
template <typename Container, typename Iter>
auto FilterColumnIndices(Container &container, uint64_t &size, Iter beg, Iter end) -> decltype(std::end(container)) {
  size_t index{0};
  size = 0;

  return std::stable_partition(
    std::begin(container), std::end(container), [&](typename Container::value_type const &val) -> bool {
      if (std::find(beg, end, index++) == end) {
        return false;
      };

      size += val.length() + 1;
      return true;
    });
};

/************************************************************
 *                                                          *
 *                         Policies                         *
 *                                                          *
 ************************************************************/

/// DelimiterPolicy: Parse columns from a row by some delimiter described by `Token`
template <char Token = '|'>
struct ColumnDelimiter {
  static auto ParseLine(std::string_view input) -> MapRow {
    auto data = MapCols{};
    data.reserve(input.length() / 2);

    uint64_t size{0};
    size_t   length{0};

    const auto *ptr = input.data();
    const auto dlm  = std::unique_ptr<const char[]>(new char[1]{Token});
    while (ptr) {
      const auto *src = ptr;
      const auto chr  = *src;
      switch (chr) {
      case '\n':
        goto exit;
        break;
      default: {
        ptr = std::strpbrk(ptr, dlm.get());
        if (ptr != nullptr) {
          length  = static_cast<size_t>(ptr - src);
          size   += length + 1;

          data.emplace_back(src, length);
          ptr++;
          break;
        }

        goto exit;
      }
      };
    }

  exit:
    auto status = common::Status::kSuccessful;
    if (size < 1 || data.size() < 1) {
      status = common::Status::kNoRowData;
    }

    return {
      .cols   = std::move(data),
      .size   = size,
      .status = status,
    };
  }
};

/// FilterPolicy: Accept all rows and don't filter
struct NoRowFilter {
  static auto Filter(MapRow & /*row*/) -> bool {
    return false;
  }
};

/// FilterPolicy: Filter rows by some predicate
template <MapPredicate Predicate>
struct RowFilter {
  static auto Filter(MapRow &row) -> bool {
    return Predicate(row);
  }
};

/// FilterPolicy: Filter rows by some predicate with capture
template <class L>
auto LambdaFilter(L &&lambda) {
  static L func = std::forward<L>(lambda);
  return [](MapRow &row) -> bool {
    return func(row);
  };
}

/// SelectorPolicy: Return
struct AllSelected {
  static auto Select(MapRow & /*row*/) -> void {}
};

/// SelectorPolicy: Select columns by indices
template <uint16_t... Args>
struct ColumnSelect {
  static auto Select(MapRow &row) -> void {
    static const std::vector<uint16_t> kSelected{Args...};
    row.cols.erase(FilterColumnIndices(row.cols, row.size, kSelected.begin(), kSelected.end()), row.cols.end());
  }
};

/// BuilderPolicy: Default, throw err
struct NoBuilder {
  static auto Build(const MapCols & /*row*/, uint8_t * /*ptr*/, MapRecord & /*record*/) -> bool {
    return false;
  }
};

/// BuilderPolicy: Build Conso record
template <MapBuilder Builder>
struct RecordBuilder {
  static auto Build(const MapCols &row, uint8_t *ptr, MapRecord &record) -> bool {
    return Builder(row, ptr, record);
  }
};

}  // namespace mapper
}  // namespace termspp
