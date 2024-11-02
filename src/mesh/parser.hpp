#pragma once

#include "termspp/common/arena.hpp"
#include "termspp/common/result.hpp"
#include "termspp/mesh/defs.hpp"

#include <map>
#include <memory>
#include <string_view>

namespace termspp {
namespace mesh {

/// Uid reference map type
typedef std::multimap<std::string_view, MeshRecord> MeshRecords;

// /// Uid reference map type
// typedef std::unordered_map<const char *, MeshRecord, common::CharHash, common::CharComp> MeshRecords;

/// MeSH document container
///   - Responsible for parsing MeSH XML docs
class MeshDocument final : public std::enable_shared_from_this<MeshDocument> {
  /// Arena allocator region size
  static constexpr const size_t kArenaRegionSize{4096LL};

public:
  /// Creates a new MeSH document instance by attemting to load
  /// the referenced MeSH XML file into memory and constructing a map
  /// of the MeSH unique identifiers
  static auto Load(const char *filepath) -> std::shared_ptr<MeshDocument>;

public:
  ~MeshDocument() = default;

  MeshDocument(MeshDocument const &)                   = delete;
  auto operator=(MeshDocument const &)->MeshDocument & = delete;

  /// Retrieve a shared_ptr that references & shares the ownership of this cls
  [[nodiscard]] auto GetRef() -> std::shared_ptr<MeshDocument> {
    return shared_from_this();
  }

  /// Getter: test whether this document loaded successfully
  [[nodiscard]] auto Ok() const -> bool;

  /// Getter: retrieve the status of this document
  [[nodiscard]] auto Status() const -> common::Status;

  /// Getter: retrieve the `Result` of this document describing success or any assoc. errs
  [[nodiscard]] auto GetResult() const -> common::Result;

  /// Getter: get the document target
  [[nodiscard]] auto GetTarget() const -> std::string_view;

  /// Getter: Get records contained by this instance
  [[nodiscard]] auto GetRecords() -> MeshRecords &;

  /// Test whether a MeSH identifier exists within this document
  [[nodiscard]] auto HasIdentifier(std::string_view ident) -> bool;

private:
  /// Loads the document from file
  auto loadFile(const char *filepath) -> common::Result;

  /// Tandem recursive function alongside `iterateChildren()` to parse records
  auto parseRecords(const void *nodePtr, const char *parentUid = nullptr) -> common::Result;

  /// Tandem recursive function alongside `parseRecords()` to parse records
  auto iterateChildren(const void *nodePtr, const MeshType &type, const char *parentUid) -> common::Result;

  /// Allocates a record to this instance's arena and packs it into a struct
  auto allocRecord(MeshRecord  &out,
                   const char  *uid,
                   const char  *name,
                   const char  *parentUid,
                   MeshType     type,
                   MeshCategory cat = MeshCategory::kUnknown,
                   MeshModifier mod = MeshModifier::kUnknown) -> common::Result;

private:
  common::Result                          result_;     /// Parsing result & document validity
  MeshRecords                             records_;    /// MeSH UID reference map
  std::unique_ptr<termspp::common::Arena> allocator_;  /// Arena allocator

protected:
  /// MeSH document constructor
  ///   - expects filepath to reference a valid XML document defining
  ///     MeSH ontological terms
  explicit MeshDocument(const char *filepath);
};

}  // namespace mesh
}  // namespace termspp
