#pragma once

#include "termspp/common/arena.hpp"
#include "termspp/mapper/defs.hpp"

#include "fastcsv/csv.h"
#include "nonstd/expected.hpp"

#include <filesystem>
#include <string_view>
#include <tuple>
#include <utility>

namespace termspp {
namespace mapper {

/************************************************************
 *                                                          *
 *                       SctDocument                        *
 *                                                          *
 ************************************************************/

/// SCT<->MeSH Document
///   - Scts SCT & MeSH codes described in the following ref:
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
          class SctPolicy       = SctAll,
          class BuilderPolicy   = NoBuilder>
class SctDocument final                                                  //
    : public std::enable_shared_from_this<SctDocument<DelimiterPolicy,   //
                                                      FilterPolicy,      //
                                                      SelectorPolicy,    //
                                                      SctPolicy,         //
                                                      BuilderPolicy>> {  //

  /// Arena allocator region size
  static constexpr const size_t kArenaRegionSize{4096LL};

  /// Policy typedef
  using SctDoc = SctDocument<DelimiterPolicy, FilterPolicy, SelectorPolicy, SctPolicy, BuilderPolicy>;

public:
  /// Creates a new Sct document instance
  static auto Load(const char *filepath) -> std::shared_ptr<SctDoc> {
    return std::shared_ptr<SctDoc>(new SctDoc(filepath));
  }

public:
  ~SctDocument() = default;

  SctDocument(SctDocument const &)                   = delete;
  auto operator=(SctDocument const &)->SctDocument & = delete;

  /// Retrieve a shared_ptr that references & shares the ownership of this cls
  [[nodiscard]] auto GetRef() -> std::shared_ptr<SctDoc> {
    return std::enable_shared_from_this<SctDoc>::shared_from_this();
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

  /// Getter: Get records contained by this instance
  [[nodiscard]] auto GetRecords() -> RecordSct & {
    return records_;
  }

private:
  /// Builds a unique map across MeSH & SCT xrefs from file
  auto buildSctping(const char *filepath) -> void {
    auto result = parseFile(filepath);
    if (!result.Ok()) {
      result_ = result;
      return;
    }

    auto rec_iter = records_.begin();
    while (rec_iter != records_.end()) {
      auto record  = rec_iter->second;
      auto *source = record.srcBuf;

      const auto *sibling = std::strncmp(source, kMeshSab, std::strlen(kMeshSab)) == 0  //
                              ? kSnomedSab                                              //
                              : kMeshSab;                                               //

      auto clen  = std::strlen(sibling);
      auto range = records_.equal_range(std::string_view{record.uidBuf});

      // Find records with valid xrefs
      auto has_sibling = std::any_of(range.first, range.second, [sibling, clen](const auto &rec) {
        return std::strncmp(rec.second.srcBuf, sibling, clen) == 0;
      });

      // Erase key-value pairs in which no mapping was made between a SNOMED + MeSH code
      if (!has_sibling) {
        rec_iter = records_.erase(range.first, range.second);
        continue;
      }

      // Advance to next key
      rec_iter = range.second;
    }

    result_ = result;
  }

  /// Responsible for parsing the document from file according to the given policies
  [[nodiscard]] auto parseFile(const char *filepath) -> common::Result {
    if (!std::filesystem::exists(filepath)) {
      return common::Result{common::Status::kFileNotFoundErr};
    }

    auto reader = std::unique_ptr<io::LineReader>();
    allocator_  = common::Arena::Create(kArenaRegionSize);
    try {
      reader = std::make_unique<io::LineReader>(filepath);
    } catch (const std::exception &err) {
      return common::Result{common::Status::kFileInitErr, err.what()};
    }

    try {
      char *line{nullptr};
      while ((line = reader->next_line()) && line) {
        // Parse col(s) per the given policy
        auto row = DelimiterPolicy::ParseLine(line);
        if (row.status != common::Status::kSuccessful) {
          continue;
        }

        // Filter row by predicate
        if (FilterPolicy::Filter(row)) {
          continue;
        }

        // Select column(s) by func
        SelectorPolicy::Select(row);

        // Ensure mappable e.g. uniqueness of column(s) by predicate
        if (row.status != common::Status::kSuccessful || !SctPolicy::ShouldSct(row, records_)) {
          continue;
        }

        // Alloc & record
        auto [cols, size, status] = row;
        auto result               = allocRow(row.cols, row.size);
        if (!result.has_value()) {
          return result.error();
        }

        auto record = result.value();
        records_.emplace(SctKey{record.uidBuf, record.srcBuf, record.trgBuf}, record);
      }
    } catch (const std::exception &err) {
      return common::Result{common::Status::kLineReaderErr, err.what()};
    }

    return common::Result{common::Status::kSuccessful};
  }

  /// Allocates a record to this instance's arena and packs it into a struct
  [[nodiscard]] auto allocRow(const SctCols &row, const uint64_t &size) -> nonstd::expected<SctRecord, common::Result> {
    // Alloc record size
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

    // Build record by some func
    auto result = SctRecord{nullptr, nullptr, nullptr};
    if (!BuilderPolicy::Build(row, ptr, result)) {
      return nonstd::make_unexpected(common::Result{common::Status::kPolicyErr, "failed to build record"});
    }

    return result;
  }

private:
  common::Result                 result_;     /// Parsing result & document validity
  RecordSct                      records_;    /// Sct records
  std::unique_ptr<common::Arena> allocator_;  /// Arena allocator

protected:
  /// Sct document constructor
  explicit SctDocument(const char *filepath) {
    buildSctping(filepath);
  }
};

}  // namespace mapper
}  // namespace termspp
