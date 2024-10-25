#pragma once

#include "constants.hpp"

#include <cstdint>
#include <string>
#include <utility>

namespace termspp {
namespace mesh {

/// Note to self:
///   - Unsure if we want to make an unordered map to test against known
///     snomed / other coding mappings; or if we want/need to throw it in
///     Apache arrow to process against the mappings?
///
struct DescriptorRecord {
  std::string name;
  std::string uid;
  uint8_t dcid;

  explicit DescriptorRecord(std::string &&uid_, std::string &&name_, uint8_t dcid_)
      : name(std::move(name_)), uid(std::move(uid_)), dcid(dcid_) {}

  DescriptorRecord(DescriptorRecord &&other) noexcept
      : name(std::move(other.name)), uid(std::move(other.uid)), dcid(std::exchange(other.dcid, 0)){};

  DescriptorRecord(DescriptorRecord &other) = default;
  DescriptorRecord(const DescriptorRecord &other) = default;
  DescriptorRecord &operator=(const DescriptorRecord &other) = default;
};

/// Parse MeSH terminology dataset from file
///  - docs: todo
bool ParseMeshDocument(const char *filepath);

} // namespace mesh
} // namespace termspp
