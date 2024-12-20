#pragma once

#include "termspp/common/result.hpp"
#include "termspp/mapper/constants.hpp"

#include <cstring>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

namespace termspp {
namespace mapper {

/************************************************************
 *                                                          *
 *                         Records                          *
 *                                                          *
 ************************************************************/

/// Columns contained by a single row
typedef std::vector<std::string_view> SctCols;

/// Describes a parsed map row
///   - Used for structured binding of ColumnDelimiter policy
struct alignas(kSctRowAlignment) SctRow {
  SctCols        cols;
  uint64_t       size;
  common::Status status;
};

/// Describes a finalised map record
struct alignas(kSctRecordAlignment) SctRecord {
  char *uidBuf;
  char *srcBuf;
  char *trgBuf;

  friend auto operator<<(std::ostream &stream, const SctRecord &obj)->std::ostream & {
    return stream << obj.uidBuf << "|"    //
                  << obj.srcBuf << "|"    //
                  << obj.trgBuf << "\n";  //
  }
};

/// Uid reference map type
typedef std::tuple<std::string_view, std::string_view, std::string_view> SctKey;

/// Lookup by individual key components
struct RecordLookup {
  std::string_view uid;
  std::string_view src;
  std::string_view trg;
};

/// Comparator for record keys
struct RecordComp {
  using is_transparent = bool;

  auto operator()(SctKey const &elem0, SctKey const &elem1) const->bool {
    auto comp0 = std::string(std::get<0>(elem0));
    auto comp1 = std::string(std::get<0>(elem1));

    comp0 += std::get<1>(elem0);
    comp1 += std::get<1>(elem1);

    return comp0.compare(comp1) < 0;
  }

  auto operator()(SctKey const &elem, const RecordLookup &lkup) const->bool {
    auto comp0 = std::string{};
    auto comp1 = std::string{};

    if (!lkup.uid.empty()) {
      comp0 += std::get<0>(elem);
      comp1 += lkup.uid;
    }

    if (!lkup.src.empty()) {
      comp0 += std::get<1>(elem);
      comp1 += lkup.src;
    }

    if (!lkup.trg.empty()) {
      comp0 += std::get<2>(elem);
      comp1 += lkup.trg;
    }

    return comp0.compare(comp1) < 0;
  }

  auto operator()(const RecordLookup &lkup, SctKey const &elem) const->bool {
    auto comp0 = std::string{};
    auto comp1 = std::string{};

    if (!lkup.uid.empty()) {
      comp0 += std::get<0>(elem);
      comp1 += lkup.uid;
    }

    if (!lkup.src.empty()) {
      comp0 += std::get<1>(elem);
      comp1 += lkup.src;
    }

    if (!lkup.trg.empty()) {
      comp0 += std::get<2>(elem);
      comp1 += lkup.trg;
    }

    return comp1.compare(comp0) < 0;
  }

  auto operator()(SctKey const &elem, const std::string_view &lkup) const->bool {
    return std::get<0>(elem).compare(lkup) < 0;
  }

  auto operator()(const std::string_view &lkup, SctKey const &elem) const->bool {
    return lkup.compare(std::get<0>(elem)) < 0;
  }
};

/// Multimap of records, keyed to components
typedef std::multimap<SctKey, SctRecord, RecordComp> RecordSct;

/************************************************************
 *                                                          *
 *                        Predicate                         *
 *                                                          *
 ************************************************************/

/// Predicate type for `FilterPolicy` policies
typedef bool (*SctPredicate)(SctRow &);

/// Record handler for `BuilderPolicy` policies
typedef bool (*RowBuilder)(const SctCols &, uint8_t *, SctRecord &);

/// Record handler for `SctPolicy` policies
typedef bool (*SctTester)(const SctRow &, const RecordSct &);

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
  static auto ParseLine(std::string_view input) -> SctRow {
    auto data = SctCols{};
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
  static auto Filter(SctRow & /*row*/) -> bool {
    return false;
  }
};

/// FilterPolicy: Filter rows by some predicate
template <SctPredicate Predicate>
struct RowFilter {
  static auto Filter(SctRow &row) -> bool {
    return Predicate(row);
  }
};

/// FilterPolicy: Filter rows by some predicate with capture
template <class L>
auto LambdaFilter(L &&lambda) {
  static L func = std::forward<L>(lambda);
  return [](SctRow &row) -> bool {
    return func(row);
  };
}

/// SelectorPolicy: Return
struct AllSelected {
  static auto Select(SctRow & /*row*/) -> void {}
};

/// SelectorPolicy: Select columns by indices
template <uint16_t... Args>
struct ColumnSelect {
  static auto Select(SctRow &row) -> void {
    static const std::vector<uint16_t> kSelected{Args...};
    row.cols.erase(FilterColumnIndices(row.cols, row.size, kSelected.begin(), kSelected.end()), row.cols.end());
  }
};

/// SctPolicy: map all rows regardless
struct SctAll {
  static auto ShouldSct(const SctRow & /*row*/, const RecordSct & /*map*/) -> bool {
    return true;
  }
};

/// SctPolicy: lambda to test uniqueness/some other prop after selection against existing records
template <SctTester Test>
struct SctSelector {
  static auto ShouldSct(const SctRow &row, const RecordSct &map) -> bool {
    return Test(row, map);
  }
};

/// BuilderPolicy: Default, throw err
struct NoBuilder {
  static auto Build(const SctCols & /*cols*/, uint8_t * /*ptr*/, SctRecord & /*record*/) -> bool {
    return false;
  }
};

/// BuilderPolicy: Build Conso record
template <RowBuilder Builder>
struct RecordBuilder {
  static auto Build(const SctCols &cols, uint8_t *ptr, SctRecord &record) -> bool {
    return Builder(cols, ptr, record);
  }
};

}  // namespace mapper
}  // namespace termspp
