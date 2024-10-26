#include "parser.hpp"

#include "arrow/api.h"
#include "pugixml.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <expected>
#include <filesystem>

namespace mesh = termspp::mesh;

/// TODO(J): docs
enum class ParseResult : char { kAllocationFail, kInvalidDataType, kUnknownNodeType, kSuccess };

/// TODO(J): impl. + docs
auto tryGetDescriptorClass(const char *dcid) -> std::expected<mesh::DescriptorClass, ParseResult> {
  if (dcid == nullptr || dcid[0] == '\0') {
    return std::unexpected(ParseResult::kInvalidDataType);
  }

  try {
    auto value = static_cast<mesh::DescriptorClass>(std::strtoul(dcid, nullptr, 0));
    if (value > mesh::kGeographicDescriptorClass) {
      return std::unexpected(ParseResult::kInvalidDataType);
    }

    return static_cast<mesh::DescriptorClass>(value);
  } catch (...) {
    return std::unexpected(ParseResult::kInvalidDataType);
  }
}

/// TODO(J): impl. + docs
auto parseDescriptorRecord(const pugi::xml_node &node, arrow::MemoryPool *pool) -> ParseResult {
  const auto dcid = tryGetDescriptorClass(node.attribute(mesh::kDescClassAttr).value());
  if (!dcid.has_value()) {
    return ParseResult::kInvalidDataType;
  }

  const auto *uid  = node.child(mesh::kDescUIProp).child_value();
  const auto *name = node.child(mesh::kDescNameProp).first_child().child_value();

  auto record = mesh::DescriptorRecord{
    .namePtr = nullptr,
    .uidPtr  = nullptr,
    .nameLen = static_cast<uint16_t>(std::strlen(name) + 1),
    .uidLen  = static_cast<uint16_t>(std::strlen(uid) + 1),
    .dcid    = dcid.value(),
  };

  uint8_t *buf{};
  if (!pool->Allocate(static_cast<int64_t>(record.uidLen + record.nameLen), &buf).ok()) {
    return ParseResult::kAllocationFail;
  }

  std::memcpy(buf, uid, record.uidLen);
  std::memcpy(buf + record.uidLen, name, record.nameLen);

  record.uidPtr  = reinterpret_cast<char *>(buf);
  record.namePtr = reinterpret_cast<char *>(buf + record.uidLen);

  /// TODO(J)
  /// -> Impl. Concept map

  return ParseResult::kSuccess;
}

/// TODO(J): impl. + docs
mesh::MeshDocument::MeshDocument(const char *filepath) {
  pool_ = arrow::MemoryPool::CreateDefault();
  loadFile(filepath);
};

mesh::MeshDocument::~MeshDocument() = default;

auto mesh::MeshDocument::Load(const char *filepath) -> std::shared_ptr<mesh::MeshDocument> {
  return std::shared_ptr<mesh::MeshDocument>(new mesh::MeshDocument(filepath));
}

/// TODO(J): impl. + docs
auto mesh::MeshDocument::loadFile(const char *filepath) -> bool {
  if (!std::filesystem::exists(filepath)) {
    return false;
  }

  auto doc    = pugi::xml_document{};
  auto result = doc.load_file(filepath);
  if (!result) {
    return false;
  }

  auto root = doc.child(mesh::kRecordSetNode);
  if (!root) {
    return false;
  }

  auto *pool = pool_.get();
  for (const auto &node : root.children()) {
    if (std::strcmp(node.name(), mesh::kRecordNode) != 0) {
      continue;
    }

    auto res = parseDescriptorRecord(node, pool);
    if (res != ParseResult::kSuccess) {
      /// TODO(J): err hnd
      return false;
    }
  }

  std::printf("[Pool: %s] Alloc: %ld | MaxMem alloc: %ld | Num allocs: %ld\n",
              pool->backend_name().c_str(),
              pool->bytes_allocated(),
              pool->max_memory(),
              pool->num_allocations());

  return true;
}
