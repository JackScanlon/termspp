#pragma once

#include "termspp/common/arena.hpp"
#include "termspp/common/utils.hpp"
#include "termspp/mapper/defs.hpp"

#include "fastcsv/csv.h"
#include "nonstd/expected.hpp"

#include <cstring>
#include <filesystem>
#include <memory>
#include <regex>
#include <sstream>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace termspp {
namespace mapper {

/************************************************************
 *                                                          *
 *                         Helpers                          *
 *                                                          *
 ************************************************************/

/// MRCONSO const.
///   - See: https://www.ncbi.nlm.nih.gov/books/NBK9685/table/ch03.T.concept_names_and_sources_file_mr/
constexpr const size_t kConsoColumnWidth    = 18U;
constexpr const size_t kConsoLangColIndex   = 1U;
constexpr const size_t kConsoSuppColIndex   = 16U;
constexpr const size_t kConsoSourceColIndex = 11U;

/// Mem. alignment
constexpr const size_t kMapRowAlignment    = 16U;
constexpr const size_t kMapRecordAlignment = 16U;

/// Columns contained by a single row
typedef std::vector<std::string_view> MapCols;

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

/// Describes a parsed map row
///   - Used for structured binding of ColumnDelimiter policy
struct alignas(kMapRowAlignment) MapRow {
  MapCols   cols;
  uint64_t  size;
  MapStatus status;
};

/// Describes a finalised map record
struct alignas(kMapRecordAlignment) MapRecord {
  char    *buf;
  uint64_t bufLen;
  uint16_t colLen;
};

/// Predicate type for `FilterPolicy` policies
typedef bool (*MapPredicate)(MapRow &);

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
    auto status = MapStatus::kSuccessful;
    if (size < 1 || data.size() < 1) {
      status = MapStatus::kNoRowData;
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

/************************************************************
 *                                                          *
 *                         Filters                          *
 *                                                          *
 ************************************************************/

/// Filter for the `MRCONSO.RRF` definition file
static auto consoFilter(MapRow &row) -> bool {
  static const auto kCodingSystemPattern = std::regex{"^(SNOMED|MSH)"};

  // Ignore empty
  auto cols = row.cols;
  if (cols.size() < kConsoColumnWidth) {
    return true;
  }

  // Ignore non-English & any obsolete rows
  if (cols.at(kConsoLangColIndex) != "ENG" || cols.at(kConsoSuppColIndex) == "O") {
    return true;
  }

  // Ignore any row that doesn't reference SCT / MeSH terms
  auto sab = cols.at(kConsoSourceColIndex);
  return !std::regex_search(sab.begin(), sab.end(), kCodingSystemPattern);
};

/************************************************************
 *                                                          *
 *                       MapDocument                        *
 *                                                          *
 ************************************************************/

/// Uid reference map type
///
/// Note: we should've probably just split up the line buffer and/or used the hash directly
///       since we've wasted mem by assigning string pairs here...
typedef std::pair<std::string, std::string>                                              MapKey;
typedef std::unordered_map<MapKey, MapRecord, common::PairHash>                          MapTargets;
typedef std::unordered_map<const char *, MapTargets, common::CharHash, common::CharComp> RecordMap;

/// SCT<->MeSH Document
///   - Maps SCT & MeSH codes described in the following ref:
///     https://www.ncbi.nlm.nih.gov/books/NBK9685/table/ch03.T.concept_names_and_sources_file_mr/
///
/// [!] Issues:
///   - Policies here were used here when assessing how best to map the source documents; we should probably
///     move to a more definitive class at some point to reduce comp. times
///
///   - Similarly, we're still incl. fastcsv as a dependency but we're only using it as a file reader now; we
///     should just remove it and buffer the file ourselves
///
template <class DelimiterPolicy = ColumnDelimiter<>,
          class FilterPolicy    = NoRowFilter,
          class SelectorPolicy  = AllSelected>
class MapDocument final
    : public std::enable_shared_from_this<MapDocument<DelimiterPolicy, FilterPolicy, SelectorPolicy>> {

  /// Arena allocator region size
  static constexpr const size_t kArenaRegionSize{8192LL};

  /// Policy typedef
  using MapDoc = MapDocument<DelimiterPolicy, FilterPolicy, SelectorPolicy>;

public:
  /// Creates a new Map document instance
  static auto Load(const char *filepath) -> std::shared_ptr<MapDoc> {
    return std::shared_ptr<MapDoc>(new MapDoc(filepath));
  }

public:
  ~MapDocument() = default;

  MapDocument(MapDocument const &)                   = delete;
  auto operator=(MapDocument const &)->MapDocument & = delete;

  /// Retrieve a shared_ptr that references & shares the ownership of this cls
  [[nodiscard]] auto GetRef() -> std::shared_ptr<MapDoc> {
    return std::enable_shared_from_this<MapDoc>::shared_from_this();
  }

  /// Getter: test whether this document loaded successfully
  [[nodiscard]] auto Ok() const -> bool {
    return result_.Ok();
  }

  /// Getter: retrieve the status of this document
  [[nodiscard]] auto Status() const -> MapStatus {
    return result_.Status();
  }

  /// Getter: retrieve the `MapResult` of this document describing success or any assoc. errs
  [[nodiscard]] auto GetResult() const -> MapResult {
    return result_;
  }

  /// Getter: get the document target
  [[nodiscard]] auto GetTarget() const -> std::string_view {
    return target_;
  }

private:
  /// Allocates a record to this instance's arena and packs it into a struct
  [[nodiscard]] auto allocRow(const MapCols &row, const uint64_t &size) -> nonstd::expected<MapRecord, MapResult> {
    uint8_t *ptr{nullptr};
    if (!allocator_->Allocate(static_cast<int64_t>(size), &ptr)) {
      auto out = std::ostringstream{};
      out << "Unable to allocate row of Size<"  //
          << size                               //
          << "> with data:\n\t| ";              //

      auto str = std::string{};
      for (auto [iter, end, index] = std::tuple{row.begin(), row.end(), 0}; iter != end; ++iter, ++index) {
        str.assign(iter->data(), iter->length());

        out << str                                //
            << (iter == end - 1 ? " |" : " | ");  //
      }

      return nonstd::make_unexpected(MapResult{MapStatus::kAllocationErr, out.str()});
    }

    size_t length{0};
    size_t offset{0};
    for (auto [iter, end, index] = std::tuple{row.begin(), row.end(), 0}; iter != end; ++iter, ++index) {
      length = iter->length();

      std::memcpy(ptr + offset, iter->data(), length);
      ptr[offset + length++] = '\0';

      offset += length;
    }

    return MapRecord{
      .buf    = reinterpret_cast<char *>(ptr),
      .bufLen = offset,
      .colLen = static_cast<uint16_t>(std::distance(row.begin(), row.end())),
    };
  }

  /// Responsible for parsing the document from file & building a unique map across MeSH & SCT records
  auto buildMapping(const char *filepath) -> void {
    if (!std::filesystem::exists(filepath)) {
      result_ = mapper::MapResult{MapStatus::kFileNotFoundErr};
      return;
    }

    auto reader = std::unique_ptr<io::LineReader>();
    try {
      reader = std::make_unique<io::LineReader>(filepath);
    } catch (const std::exception &err) {
      result_ = mapper::MapResult{mapper::MapStatus::kFileInitErr, err.what()};
      return;
    }

    target_    = filepath;
    allocator_ = common::Arena::Create(kArenaRegionSize);

    try {
      char *line{nullptr};
      while ((line = reader->next_line()) && line) {
        auto row = DelimiterPolicy::ParseLine(line);
        if (row.status != MapStatus::kSuccessful) {
          continue;
        }

        if (FilterPolicy::Filter(row)) {
          continue;
        }
        SelectorPolicy::Select(row);

        auto [cols, size, status] = row;
        if (cols.size() < 3 || size < 1) {
          continue;
        }

        // Clip extents since sv isn't null term'd
        auto cuid_value = std::string{cols.at(0).data(), cols.at(0).length()};  // CUID
        auto src_value  = std::string{cols.at(1).data(), cols.at(1).length()};  // SAB
        auto trg_value  = std::string{cols.at(2).data(), cols.at(2).length()};  // CODE/TERM

        // Ensure unique
        auto mapped      = records_.find(cuid_value.c_str());
        auto has_mapping = mapped != records_.end();
        auto map_key     = MapKey(src_value, trg_value);
        if (has_mapping && mapped->second.find(map_key) != mapped->second.end()) {
          continue;
        }

        // Alloc & record
        auto result = allocRow(cols, size);
        if (!result.has_value()) {
          result_ = result.error();
          return;
        }

        auto record = result.value();
        if (!has_mapping) {
          auto [iter, ins] = records_.emplace(record.buf, MapTargets{});
          mapped           = iter;
        }
        mapped->second.emplace(map_key, record);
      }
    } catch (const std::exception &err) {
      result_ = mapper::MapResult{mapper::MapStatus::kLineReaderErr, err.what()};
      return;
    }

    result_ = mapper::MapResult{mapper::MapStatus::kSuccessful};
  }

private:
  std::string_view               target_;     /// Document target resource
  MapResult                      result_;     /// Parsing result & document validity
  RecordMap                      records_;    /// Map records
  std::unique_ptr<common::Arena> allocator_;  /// Arena allocator

protected:
  /// Map document constructor
  explicit MapDocument(const char *filepath) {
    buildMapping(filepath);
  }
};

/************************************************************
 *                                                          *
 *                        Map decl.                         *
 *                                                          *
 ************************************************************/

/// MRCONSO columns of interest
///   - Col [ 0] -> CUID
///   - Col [11] -> SAB
///   - Col [13] -> CODE/TERM
typedef ColumnSelect<0, 11, 13> ConsoCols;  // NOLINT

// clang-format off
// NOLINTBEGIN
typedef MapDocument<ColumnDelimiter<'|'>,    // Columns delimited by pipe
                    RowFilter<consoFilter>,  // Filter rows by lang
                    ConsoCols                // Select CUID, SAB & CODE
                   > ConsoReader;            // <MapDocument<...>>
// NOLINTEND
// clang-format on

}  // namespace mapper
}  // namespace termspp
