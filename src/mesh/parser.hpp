#pragma once

#include "termspp/common/utils.hpp"
#include "termspp/mesh/records.hpp"

#include "arrow/memory_pool.h"

#include <memory>
#include <unordered_map>

namespace termspp {
namespace mesh {

namespace common = termspp::common;

/// MeSH document container
///   - Responsible for parsing MeSH XML
class MeshDocument final : public std::enable_shared_from_this<MeshDocument> {
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
  RecordMap                          records_;
  std::unique_ptr<arrow::MemoryPool> pool_;  // Pool backend one of [ mimalloc | jemalloc ] depending on env

protected:
  explicit MeshDocument(const char *filepath);
};

}  // namespace mesh
}  // namespace termspp
