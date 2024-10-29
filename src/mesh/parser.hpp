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
  static constexpr const size_t kArenaRegionSize = 8192;

  typedef std::unordered_map<const char *, MeshRecord, common::CharHash, common::CharComp> RecordMap;

public:
  static auto Load(const char *filepath) -> std::shared_ptr<MeshDocument>;

public:
  // clang-format off
  MeshDocument(MeshDocument const &)                  = delete;
  auto operator=(MeshDocument const &)->MeshDocument& = delete;
  virtual ~MeshDocument();
  // clang-format on

private:
  auto loadFile(const char *filepath) -> MeshResult;

  auto parseRecords(const void *nodePtr, const char *parentUid = nullptr) -> MeshResult;

  auto iterateChildren(const void *nodePtr, const MeshType &type, const char *parentUid) -> MeshResult;

  auto allocRecord(const char  *uid,
                   const char  *name,
                   const char  *parentUid,
                   MeshType     type,
                   MeshCategory cat = MeshCategory::kUnknown,
                   MeshModifier mod = MeshModifier::kUnknown) -> MeshResult;

private:
  RecordMap                               records_;    // Uid
  std::unique_ptr<termspp::common::Arena> allocator_;  // Arena allocator

protected:
  explicit MeshDocument(const char *filepath);
};

}  // namespace mesh
}  // namespace termspp
