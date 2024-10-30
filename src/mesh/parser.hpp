#pragma once

#include "termspp/common/arena.hpp"
#include "termspp/common/utils.hpp"
#include "termspp/mesh/records.hpp"

#include <memory>
#include <unordered_map>

namespace termspp {
namespace mesh {

namespace common = termspp::common;

/// MeSH document container
///   - Responsible for parsing MeSH XML docs
class MeshDocument final : public std::enable_shared_from_this<MeshDocument> {
  /// Arena allocator region size
  static constexpr const size_t kArenaRegionSize = 8192LL;

  /// Uid reference map type
  typedef std::unordered_map<const char *, MeshRecord, common::CharHash, common::CharComp> RecordMap;

public:
  /// Creates a new MeSH document instance by attemting to load
  /// the referenced MeSH XML file into memory and constructing a map
  /// of the MeSH unique identifiers
  static auto Load(const char *filepath) -> std::shared_ptr<MeshDocument>;

public:
  ~MeshDocument() = default;

  MeshDocument(MeshDocument const &)                   = delete;
  auto operator=(MeshDocument const &)->MeshDocument & = delete;

  /// TODO(J): docs
  auto Ok() const -> bool;
  auto Status() const -> MeshStatus;
  auto GetResult() const -> MeshResult;

private:
  /// TODO(J): docs
  auto loadFile(const char *filepath) -> MeshResult;

  /// TODO(J): docs
  auto parseRecords(const void *nodePtr, const char *parentUid = nullptr) -> MeshResult;

  /// TODO(J): docs
  auto iterateChildren(const void *nodePtr, const MeshType &type, const char *parentUid) -> MeshResult;

  /// TODO(J): docs
  auto allocRecord(const char  *uid,
                   const char  *name,
                   const char  *parentUid,
                   MeshType     type,
                   MeshCategory cat = MeshCategory::kUnknown,
                   MeshModifier mod = MeshModifier::kUnknown) -> MeshResult;

private:
  MeshResult                              result_;     // Parsing result & document validity
  RecordMap                               records_;    // MeSH UID reference map
  std::unique_ptr<termspp::common::Arena> allocator_;  // Arena allocator

protected:
  /// MeSH document constructor
  ///   - expects filepath to reference a valid XML document defining
  ///     MeSH ontological terms
  explicit MeshDocument(const char *filepath);
};

}  // namespace mesh
}  // namespace termspp
