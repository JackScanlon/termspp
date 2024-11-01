#pragma once

#include "termspp/common/arena.hpp"
#include "termspp/mapper/defs.hpp"

#include "fastcsv/csv.h"
#include "nonstd/expected.hpp"

#include <filesystem>
#include <map>
#include <string_view>
#include <tuple>
#include <utility>

namespace termspp {
namespace mapper {

namespace common = termspp::common;

/************************************************************
 *                                                          *
 *                       MapDocument                        *
 *                                                          *
 ************************************************************/

/// Uid reference map type
typedef std::tuple<const char *, const char *, const char *> MapKey;

/// Lookup by individual key components
struct RecordLookup {
  const char *uid;
  const char *src;
  const char *trg;
};

/// Comparator for record keys
struct RecordComp {
  using is_transparent = bool;

  auto operator()(MapKey const &elem0, MapKey const &elem1) const->bool {
    // clang-format off
    // NOLINTBEGIN
    return (std::strcmp(std::get<0>(elem0), std::get<0>(elem1))
          + std::strcmp(std::get<1>(elem0), std::get<1>(elem1))
          + std::strcmp(std::get<2>(elem0), std::get<2>(elem1))) < 0;
    // NOLINTEND
    // clang-format on
  }

  auto operator()(MapKey const &elem, const RecordLookup &lkup) const->bool {
    // clang-format off
    // NOLINTBEGIN
    return ((lkup.uid != nullptr ? std::strcmp(std::get<0>(elem), lkup.uid) : 0)
          + (lkup.src != nullptr ? std::strcmp(std::get<1>(elem), lkup.src) : 0)
          + (lkup.trg != nullptr ? std::strcmp(std::get<2>(elem), lkup.trg) : 0)) < 0;
    // NOLINTEND
    // clang-format on
  }

  auto operator()(const RecordLookup &lkup, MapKey const &elem) const->bool {
    // clang-format off
    // NOLINTBEGIN
    return ((lkup.uid != nullptr ? std::strcmp(lkup.uid, std::get<0>(elem)) : 0)
          + (lkup.src != nullptr ? std::strcmp(lkup.src, std::get<1>(elem)) : 0)
          + (lkup.trg != nullptr ? std::strcmp(lkup.trg, std::get<2>(elem)) : 0)) < 0;
    // NOLINTEND
    // clang-format on
  }
};

/// Multimap of records, keyed to components
typedef std::multimap<MapKey, MapRecord, RecordComp> RecordMap;

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
          class SelectorPolicy  = AllSelected,
          class BuilderPolicy   = NoBuilder>
class MapDocument final
    : public std::enable_shared_from_this<MapDocument<DelimiterPolicy, FilterPolicy, SelectorPolicy, BuilderPolicy>> {

  /// Arena allocator region size
  static constexpr const size_t kArenaRegionSize{4096LL};

  /// Policy typedef
  using MapDoc = MapDocument<DelimiterPolicy, FilterPolicy, SelectorPolicy, BuilderPolicy>;

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
  [[nodiscard]] auto Status() const -> common::Status {
    return result_.Status();
  }

  /// Getter: retrieve the `Result` of this document describing success or any assoc. errs
  [[nodiscard]] auto GetResult() const -> common::Result {
    return result_;
  }

private:
  /// Allocates a record to this instance's arena and packs it into a struct
  [[nodiscard]] auto allocRow(const MapCols &row, const uint64_t &size) -> nonstd::expected<MapRecord, common::Result> {
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

      return nonstd::make_unexpected(common::Result{common::Status::kAllocationErr, out.str()});
    }

    auto result = MapRecord{nullptr, nullptr, nullptr};
    if (!BuilderPolicy::Build(row, ptr, result)) {
      return nonstd::make_unexpected(common::Result{common::Status::kPolicyErr, "failed to build record"});
    }

    return result;
  }

  /// Responsible for parsing the document from file & building a unique map across MeSH & SCT records
  auto buildMapping(const char *filepath) -> void {
    if (!std::filesystem::exists(filepath)) {
      result_ = common::Result{common::Status::kFileNotFoundErr};
      return;
    }

    auto reader = std::unique_ptr<io::LineReader>();
    allocator_  = common::Arena::Create(kArenaRegionSize);
    try {
      reader = std::make_unique<io::LineReader>(filepath);
    } catch (const std::exception &err) {
      result_ = common::Result{common::Status::kFileInitErr, err.what()};
      return;
    }

    try {
      char *line{nullptr};
      while ((line = reader->next_line()) && line) {
        auto row = DelimiterPolicy::ParseLine(line);
        if (row.status != common::Status::kSuccessful) {
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
        auto uid = std::string{cols.at(0).data(), cols.at(0).length()};  // CUID
        auto src = std::string{cols.at(1).data(), cols.at(1).length()};  // SAB
        auto trg = std::string{cols.at(2).data(), cols.at(2).length()};  // CODE/TERM

        // Ensure unique
        if (records_.find(RecordLookup{uid.c_str(), src.c_str(), trg.c_str()}) != records_.end()) {
          continue;
        }

        // Alloc & record
        auto result = allocRow(cols, size);
        if (!result.has_value()) {
          result_ = result.error();
          return;
        }

        auto record = result.value();
        records_.emplace(MapKey{record.uidBuf, record.srcBuf, record.trgBuf}, record);
      }
    } catch (const std::exception &err) {
      result_ = common::Result{common::Status::kLineReaderErr, err.what()};
      return;
    }

    result_ = common::Result{common::Status::kSuccessful};
  }

private:
  common::Result                 result_;     /// Parsing result & document validity
  RecordMap                      records_;    /// Map records
  std::unique_ptr<common::Arena> allocator_;  /// Arena allocator

protected:
  /// Map document constructor
  explicit MapDocument(const char *filepath) {
    buildMapping(filepath);
  }
};

}  // namespace mapper
}  // namespace termspp
