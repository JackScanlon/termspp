#pragma once

#include "termspp/common/arena.hpp"
#include "termspp/mapper/defs.hpp"

#include "fastcsv/csv.h"
#include "nonstd/expected.hpp"

#include <cstring>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string_view>
#include <tuple>
#include <vector>

namespace termspp {
namespace mapper {

/************************************************************
 *                                                          *
 *                         Helpers                          *
 *                                                          *
 ************************************************************/

constexpr const size_t kMapRowAlignment    = 64U;
constexpr const size_t kMapRecordAlignment = 32U;

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

/// Describes a finalised map item
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
 *                       MapDocument                        *
 *                                                          *
 ************************************************************/

/// TODO(J): docs for Map document container
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

  /// TODO(J): docs
  [[nodiscard]] auto Ok() const -> bool {
    return result_.Ok();
  }

  /// TODO(J): docs
  [[nodiscard]] auto Status() const -> MapStatus {
    return result_.Status();
  }

  /// TODO(J): docs
  [[nodiscard]] auto GetResult() const -> MapResult {
    return result_;
  }

  [[nodiscard]] auto GetTarget() const -> std::string_view {
    return target_;
  }

private:
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

private:
  std::string_view               target_;     // Document target resource
  MapResult                      result_;     // Parsing result & document validity
  std::vector<MapRecord>         records_;    // Document records
  std::unique_ptr<common::Arena> allocator_;  // Arena allocator

protected:
  /// Map document constructor
  explicit MapDocument(const char *filepath) {
    if (!std::filesystem::exists(filepath)) {
      result_ = mapper::MapResult{MapStatus::kFileNotFoundErr};
      return;
    }

    auto reader = std::unique_ptr<io::LineReader>();
    target_     = filepath;

    try {
      reader = std::make_unique<io::LineReader>(filepath);
    } catch (const std::exception &err) {
      result_ = mapper::MapResult{mapper::MapStatus::kFileInitErr, err.what()};
      return;
    }

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
        if (cols.size() < 1 || size < 1) {
          continue;
        }

        auto result = allocRow(cols, size);
        if (!result.has_value()) {
          result_ = result.error();
          return;
        }

        auto [buf, bufLen, colLen] = result.value();
        records_.emplace_back(buf, bufLen, colLen);
      }
    } catch (const std::exception &err) {
      result_ = mapper::MapResult{mapper::MapStatus::kLineReaderErr, err.what()};
      return;
    }

    result_ = mapper::MapResult{mapper::MapStatus::kSuccessful};

    // for (const auto &record : records_) {
    //   std::printf("[SAMPLE] Record<Id: %s, Lang: %s>\n", record.buf, record.buf + std::strlen(record.buf) + 1);
    // }
  }
};

}  // namespace mapper
}  // namespace termspp
