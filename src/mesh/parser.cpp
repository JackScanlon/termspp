#include "termspp/mesh/parser.hpp"

#include "termspp/common/strings.hpp"
#include "termspp/mesh/constants.hpp"
#include "termspp/mesh/fields.hpp"

#include "arrow/api.h"
#include "pugixml.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <expected>
#include <filesystem>

namespace mesh   = termspp::mesh;
namespace common = termspp::common;

/// TODO(J): docs
auto tryGetRecordType(const pugi::xml_node *node) -> std::expected<mesh::MeshType, mesh::MeshResult> {
  const auto types = mesh::kNodeTypes();
  const auto type  = types.find(node->name());
  if (type == types.end()) {
    return std::unexpected(mesh::MeshResult{mesh::MeshStatus::kUnknownNodeTypeErr});
  }

  return type->second;
}

/// TODO(J): docs
auto tryGetRecordFields(const mesh::MeshType &type,
                        const pugi::xml_node *node) -> std::expected<mesh::MeshProps, mesh::MeshResult> {
  const auto fields = mesh::kNodeFields();
  const auto schema = std::find_if(fields.begin(), fields.end(), [&](const mesh::MeshFields &elem) {
    return elem.nodeType == type;
  });

  if (schema == fields.end()) {
    return std::unexpected(mesh::MeshResult{mesh::MeshStatus::kUnknownNodeTypeErr});
  }

  const auto *uid  = node->child(schema->uidField).child_value();
  const char *name = nullptr;
  if (schema->isEncapsulated && schema->isNamedField) {
    name = node->first_child().child(schema->nameField).first_child().child_value();
  } else if (schema->isEncapsulated) {
    name = node->first_child().child(mesh::kStringField).child_value();
  } else if (schema->isNamedField) {
    name = node->child(schema->nameField).first_child().child_value();
  } else {
    name = node->child(mesh::kStringField).child_value();
  }

  if (uid == nullptr || name == nullptr) {
    return std::unexpected(mesh::MeshResult{mesh::MeshStatus::kInvalidDataTypeErr});
  }

  return mesh::MeshProps{
    .uid  = uid,
    .name = name,
  };
}

/// TODO(J): docs
auto tryGetDescriptorClass(const char *attr) -> std::expected<mesh::MeshCategory, mesh::MeshResult> {
  auto status = mesh::MeshResult{mesh::MeshStatus::kEmptyNodeDataErr};
  if (attr == nullptr || attr[0] == '\0') {
    return std::unexpected(status);
  }

  auto result = mesh::MeshCategory::kUnknown;
  try {
    auto value = static_cast<mesh::MeshCategory>(std::strtoul(attr, nullptr, 0));
    if (value > mesh::MeshCategory::kUnknown && value <= mesh::MeshCategory::kDescriptorGeographic) {
      result = value;
      status.SetStatus(mesh::MeshStatus::kSuccessful);
    }
  } catch (const std::exception &e) {
    status.SetStatus(mesh::MeshStatus::kInvalidDataTypeErr);
  }

  if (!status) {
    return std::unexpected(status);
  }

  return result;
}

/// TODO(J): docs
auto tryGetConceptPreference(const char *attr) -> std::expected<mesh::MeshCategory, mesh::MeshResult> {
  auto status = mesh::MeshResult{mesh::MeshStatus::kEmptyNodeDataErr};
  if (attr == nullptr) {
    return std::unexpected(status);
  }

  try {
    auto preference = common::coerceIntoBoolean(std::string_view(attr));
    return preference ? mesh::MeshCategory::kConceptPreferred : mesh::MeshCategory::kConceptNarrower;
  } catch (const std::exception &e) {
    status.SetStatus(mesh::MeshStatus::kInvalidDataTypeErr);
  }

  return std::unexpected(status);
}

/// TODO(J): docs
auto tryGetTermAttributes(const pugi::xml_node *node) -> std::expected<mesh::MeshTermAttr, mesh::MeshResult> {
  if (node == nullptr || !(*node)) {
    return std::unexpected(mesh::MeshResult{mesh::MeshStatus::kNodeDoesNotExistErr});
  }

  auto result = mesh::MeshTermAttr{
    .cat = mesh::MeshCategory::kUnknown,
    .mod = mesh::MeshModifier::kUnknown,
  };

  // clang-format off
  try {
    const auto *pref = node->attribute(mesh::kTermDescAttr).value();
    if (pref != nullptr) {
      result.cat = common::coerceIntoBoolean(std::string_view(pref))
        ? mesh::MeshCategory::kTermDescriptorPref
        : result.cat;
    }
  } catch (const std::exception& e) {}

  if (result.cat == mesh::MeshCategory::kUnknown) {
    try {
      const auto *pref = node->attribute(mesh::kTermConcAttr).value();
      if (pref != nullptr) {
        result.cat = common::coerceIntoBoolean(std::string_view(pref))
          ? mesh::MeshCategory::kTermConceptPref
          : result.cat;
      }
    } catch (const std::exception& e) {}
  }
  // clang-format on

  const auto *mod_value = node->attribute(mesh::kTermLexAttr).value();
  if (mod_value != nullptr) {
    const auto mods = mesh::kMeshModifiers();
    const auto mod  = mods.find(mod_value);
    if (mod != mods.end()) {
      result.mod = mod->second;
    }
  }

  return result;
}

/// TODO(J): docs
mesh::MeshDocument::MeshDocument(const char *filepath) {
  pool_ = arrow::MemoryPool::CreateDefault();

  auto result = loadFile(filepath);
  std::printf("Result: %d | Msg: %s\n", static_cast<uint8_t>(result.Status()), result.Description().c_str());

  // TODO(J): err hnd
  // ...
};

mesh::MeshDocument::~MeshDocument() = default;

auto mesh::MeshDocument::Load(const char *filepath) -> std::shared_ptr<mesh::MeshDocument> {
  return std::shared_ptr<mesh::MeshDocument>(new mesh::MeshDocument(filepath));
}

/// TODO(J): docs
auto mesh::MeshDocument::loadFile(const char *filepath) -> mesh::MeshResult {
  if (!std::filesystem::exists(filepath)) {
    return mesh::MeshResult{mesh::MeshStatus::kFileNotFoundErr};
  }

  auto doc    = pugi::xml_document{};
  auto result = doc.load_file(filepath);
  if (!result) {
    return mesh::MeshResult{mesh::MeshStatus::kXmlReadErr, result.description()};
  }

  auto root = doc.child(mesh::kRecordSetNode);
  if (!root) {
    return mesh::MeshResult{mesh::MeshStatus::kRootDoesNotExistErr};
  }

  for (const auto &node : root.children()) {
    if (std::strcmp(node.name(), mesh::kRecordNode) != 0) {
      continue;
    }

    auto res = parseRecords(static_cast<const void *>(&node));
    if (!res) {
      return res;
    }
  }

  // for (const auto &[uid, rec] : records_) {
  //   std::printf("[SAMPLE] UID: %s | Name: %s\n", uid, rec.buf + rec.uidLen);
  // }

  std::printf("[Pool: %s] Alloc: %ld | MaxMem alloc: %ld | Num allocs: %ld\n",
              pool_->backend_name().c_str(),
              pool_->bytes_allocated(),
              pool_->max_memory(),
              pool_->num_allocations());

  return mesh::MeshResult{mesh::MeshStatus::kSuccessful};
}

/// TODO(J): docs
auto mesh::MeshDocument::parseRecords(const void *nodePtr, const char *parentUid /*= nullptr*/) -> mesh::MeshResult {
  if (nodePtr == nullptr) {
    return mesh::MeshResult{mesh::MeshStatus::kNodeDoesNotExistErr};
  }

  const auto *node = static_cast<const pugi::xml_node *>(nodePtr);
  if (node == nullptr || !(*node)) {
    return mesh::MeshResult{mesh::MeshStatus::kNodeDoesNotExistErr};
  }

  auto type = mesh::MeshType::kUnknown;
  auto cat  = mesh::MeshCategory::kUnknown;
  auto mod  = mesh::MeshModifier::kUnknown;

  const auto type_res = tryGetRecordType(node);
  if (!type_res.has_value()) {
    return type_res.error();
  }
  type = type_res.value();

  const auto fields = tryGetRecordFields(type, node);
  if (!fields.has_value()) {
    return fields.error();
  }

  const auto [uid, name] = fields.value();
  if (uid == nullptr || name == nullptr) {
    return mesh::MeshResult{mesh::MeshStatus::kInvalidDataTypeErr};
  }

  switch (type) {
  case mesh::MeshType::kDescriptorRecord: {
    const auto cat_result = tryGetDescriptorClass(node->attribute(mesh::kDescClassAttr).value());
    if (!cat_result.has_value()) {
      return cat_result.error();
    }
    cat = cat_result.value();
  } break;

  case mesh::MeshType::kConcept: {
    const auto cat_result = tryGetConceptPreference(node->attribute(mesh::kConcPrefAttr).value());
    if (!cat_result.has_value()) {
      return cat_result.error();
    }

    cat = cat_result.value();
  } break;

  case mesh::MeshType::kTerm: {
    const auto attr = tryGetTermAttributes(node);
    if (!attr.has_value()) {
      return attr.error();
    }

    cat = attr->cat;
    mod = attr->mod;
  } break;

  case mesh::MeshType::kQualifier:
    break;

  default:
    return mesh::MeshResult{mesh::MeshStatus::kUnknownNodeTypeErr};
  };

  auto res = allocRecord(uid, name, parentUid, type, cat, mod);
  if (!res) {
    return res;
  }

  if (type == mesh::MeshType::kDescriptorRecord || type == mesh::MeshType::kConcept) {
    res = iterateChildren(nodePtr, type, uid);
    if (!res) {
      return res;
    }
  }

  return mesh::MeshResult{mesh::MeshStatus::kSuccessful};
}

/// TODO(J): docs
auto mesh::MeshDocument::iterateChildren(const void           *nodePtr,
                                         const mesh::MeshType &type,
                                         const char           *parentUid) -> mesh::MeshResult {
  if (nodePtr == nullptr) {
    return mesh::MeshResult{mesh::MeshStatus::kSuccessful};
  }

  const auto *node = static_cast<const pugi::xml_node *>(nodePtr);
  if (node == nullptr || !(*node)) {
    return mesh::MeshResult{mesh::MeshStatus::kSuccessful};
  }

  auto result = mesh::MeshResult{mesh::MeshStatus::kUnknownNodeTypeErr};
  switch (type) {
  case mesh::MeshType::kConcept: {
    auto terms = node->child(mesh::kTermListNode);
    if (terms) {
      for (const auto &child : terms.children()) {
        if (std::strcmp(child.name(), mesh::kTermNode) != 0) {
          continue;
        }

        auto res = parseRecords(static_cast<const void *>(&child), parentUid);
        if (!res) {
          return res;
        }
      }
    }
    result.SetStatus(mesh::MeshStatus::kSuccessful);
  } break;

  case mesh::MeshType::kDescriptorRecord: {
    auto concepts = node->child(mesh::kConcListNode);
    if (concepts) {
      for (const auto &child : concepts.children()) {
        if (std::strcmp(child.name(), mesh::kConcNode) != 0) {
          continue;
        }

        auto res = parseRecords(static_cast<const void *>(&child), parentUid);
        if (!res) {
          return res;
        }
      }
    }

    auto qualifiers = node->child(mesh::kQualListNode);
    if (qualifiers) {
      for (const auto &child : qualifiers.children()) {
        if (std::strcmp(child.name(), mesh::kQualNode) != 0) {
          continue;
        }

        auto res = parseRecords(static_cast<const void *>(&child), parentUid);
        if (!res) {
          return res;
        }
      }
    }
    result.SetStatus(mesh::MeshStatus::kSuccessful);
  } break;

  default:
    break;
  }

  return result;
}

/// TODO(J): docs
auto mesh::MeshDocument::allocRecord(const char        *uid,
                                     const char        *name,
                                     const char        *parentUid,
                                     mesh::MeshType     type,
                                     mesh::MeshCategory cat, /*= mesh::MeshCategory::kUnknown*/
                                     mesh::MeshModifier mod /*= mesh::MeshModifier::kUnknown*/) -> mesh::MeshResult {
  auto record = mesh::MeshRecord{
    .buf       = nullptr,
    .parentUid = parentUid,
    .uidLen    = static_cast<uint16_t>(std::strlen(uid) + 1),
    .nameLen   = static_cast<uint16_t>(std::strlen(name) + 1),
    .type      = type,
    .category  = cat,
    .modifier  = mod,
  };

  uint8_t *ptr{nullptr};
  if (!pool_->Allocate(static_cast<int64_t>(record.uidLen + record.nameLen), &ptr).ok()) {
    return mesh::MeshResult{mesh::MeshStatus::kAllocationErr};
  }

  std::memcpy(ptr, uid, record.uidLen);
  std::memcpy(ptr + record.uidLen, name, record.nameLen);

  record.buf = reinterpret_cast<char *>(ptr);
  records_.emplace(record.buf, record);
  return mesh::MeshResult{mesh::MeshStatus::kSuccessful};
}
