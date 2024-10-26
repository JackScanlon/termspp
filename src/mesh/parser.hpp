#pragma once

#include "arrow/memory_pool.h"
#include "constants.hpp"
#include "termspp/common/utils.hpp"

#include <cstdint>
#include <memory>
#include <unordered_map>

namespace termspp {
namespace mesh {

namespace common = termspp::common;

/// Note to self:
///   - Unsure if we want to make an unordered map to test against known
///     snomed / other coding mappings; or if we want/need to throw it in
///     Apache arrow to process against the mappings?
///
struct DescriptorRecord {
  char    *namePtr;
  char    *uidPtr;
  uint16_t nameLen;
  uint16_t uidLen;
  uint8_t  dcid;
};

/// Parse MeSH terminology dataset from file
/// TODO(J): docs
class MeshDocument final : public std::enable_shared_from_this<MeshDocument> {
  typedef std::unordered_map<const char *, DescriptorRecord, common::CharHash, common::CharComp> RecordMap;

public:
  static auto Load(const char *filepath) -> std::shared_ptr<MeshDocument>;

public:
  // clang-format off
  MeshDocument(MeshDocument const &)                  = delete;
  auto operator=(MeshDocument const &)->MeshDocument& = delete;
  virtual ~MeshDocument();

private:
  auto loadFile(const char *filepath) -> bool;

private:
  RecordMap                          records_;
  std::unique_ptr<arrow::MemoryPool> pool_;

protected:
  explicit MeshDocument(const char *filepath);
};

}  // namespace mesh
}  // namespace termspp
