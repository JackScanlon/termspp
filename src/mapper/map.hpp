#pragma once

#include "termspp/mapper/defs.hpp"

#include "fastcsv/csv.h"

#include <filesystem>
#include <memory>
#include <string_view>
#include <vector>

namespace termspp {
namespace mapper {

/************************************************************
 *                                                          *
 *                         Helpers                          *
 *                                                          *
 ************************************************************/

/// Map types
typedef std::vector<std::string_view> MapRow;
typedef bool (*MapPredicate)(MapRow &);

/// Filter columns by index
template <typename Container, typename Iter>
auto ToggleIndices(Container &container, Iter beg, Iter end) -> decltype(std::end(container)) {
  size_t index{0};
  return std::stable_partition(
    std::begin(container), std::end(container), [&](typename Container::value_type const & /*val*/) -> bool {
      return std::find(beg, end, index++) != end;
    });
};

/************************************************************
 *                                                          *
 *                         Policies                         *
 *                                                          *
 ************************************************************/

template <char Token = '|'>
struct ColumnDelimiter {
  static auto ParseLine(std::string_view input) -> std::vector<std::string_view> {
    auto data = std::vector<std::string_view>{};
    data.reserve(input.length() / 2);

    const auto *ptr = input.data();
    const auto dlm  = std::unique_ptr<const char[]>(new char[1]{Token});
    while (ptr) {
      const auto *src = ptr;
      const auto chr  = *src;
      switch (chr) {
      case '\r':
      case '\n':
        goto exit;
        break;
      default: {
        ptr = std::strpbrk(ptr, dlm.get());
        if (ptr) {
          data.emplace_back(src, static_cast<size_t>(ptr - src));
          ptr++;
          break;
        }

        goto exit;
        break;
      };
      }
    }

  exit:
    return data;
  }
};

struct NoFilter {
  static auto Filter(MapRow & /*row*/) -> bool {
    return false;
  }
};

template <MapPredicate Predicate>
struct RowFilter {
  static auto Filter(MapRow &row) -> bool {
    return Predicate(row);
  }
};

struct NoSelector {
  static auto Select(MapRow & /*row*/) -> void {}
};

template <uint16_t... Args>
struct ColumnSelector {
  static auto Select(MapRow &row) -> void {
    static const std::vector<uint16_t> kSelected{Args...};
    row.erase(ToggleIndices(row, kSelected.begin(), kSelected.end()), row.end());
  }
};

/************************************************************
 *                                                          *
 *                       MapDocument                        *
 *                                                          *
 ************************************************************/

/// TODO(J): docs for Map document container
template <class DelimiterPolicy = ColumnDelimiter<>, class FilterPolicy = NoFilter, class SelectorPolicy = NoSelector>
class MapDocument final
    : public std::enable_shared_from_this<MapDocument<DelimiterPolicy, FilterPolicy, SelectorPolicy>> {
  using MapDoc = MapDocument<DelimiterPolicy, FilterPolicy, SelectorPolicy>;

  /// Column width(s)
  static constexpr uint8_t kMapRelWidth   = 16U;
  static constexpr uint8_t kMapConsoWidth = 18U;

public:
  /// Creates a new Map document instance
  static auto Load(const char *filepath) -> std::shared_ptr<MapDoc> {
    return std::shared_ptr<MapDoc>(new MapDoc(filepath));
  }

public:
  ~MapDocument() = default;

  MapDocument(MapDocument const &)                   = delete;
  auto operator=(MapDocument const &)->MapDocument & = delete;

private:
  auto readRow() -> bool;

private:
  std::string_view target_;
  MapResult        result_;

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

    try {
      char *line{nullptr};
      while ((line = reader->next_line()) && line) {
        auto cols = DelimiterPolicy::ParseLine(line);
        if (FilterPolicy::Filter(cols)) {
          continue;
        }

        SelectorPolicy::Select(cols);

        /// TODO(J):
        ///   - Alloc
        ///   - Comp to addr?
        ///
      }
    } catch (const std::exception &err) {
      result_ = mapper::MapResult{mapper::MapStatus::kLineReaderErr, err.what()};
      return;
    }

    result_ = mapper::MapResult{mapper::MapStatus::kSuccessful};
  }
};

}  // namespace mapper
}  // namespace termspp
