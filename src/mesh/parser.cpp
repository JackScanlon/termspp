#include "termspp/mesh/parser.hpp"

#include "termspp/common/strings.hpp"
#include "termspp/mesh/constants.hpp"
#include "termspp/mesh/fields.hpp"

#include "nonstd/expected.hpp"
#include "pugixml.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

namespace mesh   = termspp::mesh;
namespace common = termspp::common;

/************************************************************
 *                                                          *
 *                         Helpers                          *
 *                                                          *
 ************************************************************/

/// Attempt to derive the record type from the node's children
auto tryGetRecordType(const pugi::xml_node *node) -> nonstd::expected<mesh::MeshType, mesh::MeshResult> {
  const auto types = mesh::kNodeTypes();
  const auto type  = types.find(node->name());
  if (type == types.end()) {
    return nonstd::make_unexpected(mesh::MeshResult{mesh::MeshStatus::kUnknownNodeTypeErr});
  }

  return type->second;
}

/// Attempt to retrieve some MeSH node's top-level field(s)
auto tryGetRecordFields(const mesh::MeshType &type,
                        const pugi::xml_node *node) -> nonstd::expected<mesh::MeshProps, mesh::MeshResult> {
  const auto fields = mesh::kNodeFields();
  const auto schema = std::find_if(fields.begin(), fields.end(), [&](const mesh::MeshFields &elem) {
    return elem.nodeType == type;
  });

  if (schema == fields.end()) {
    return nonstd::make_unexpected(mesh::MeshResult{mesh::MeshStatus::kUnknownNodeTypeErr});
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
    return nonstd::make_unexpected(mesh::MeshResult{mesh::MeshStatus::kInvalidDataTypeErr});
  }

  return mesh::MeshProps{
    .uid  = uid,
    .name = name,
  };
}

/// Attempt to derive the `DescriptorRecordSet` node's class
auto tryGetDescriptorClass(const char *attr) -> nonstd::expected<mesh::MeshCategory, mesh::MeshResult> {
  auto status = mesh::MeshResult{mesh::MeshStatus::kEmptyNodeDataErr};
  if (attr == nullptr || attr[0] == '\0') {
    return nonstd::make_unexpected(status);
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
    return nonstd::make_unexpected(status);
  }

  return result;
}

/// Attempt to retrieve the `<Concept />` node's preference attribute
auto tryGetConceptPreference(const char *attr) -> nonstd::expected<mesh::MeshCategory, mesh::MeshResult> {
  auto status = mesh::MeshResult{mesh::MeshStatus::kEmptyNodeDataErr};
  if (attr == nullptr) {
    return nonstd::make_unexpected(status);
  }

  try {
    auto preference = common::coerceIntoBoolean(std::string_view(attr));
    return preference ? mesh::MeshCategory::kConceptPreferred : mesh::MeshCategory::kConceptNarrower;
  } catch (const std::exception &e) {
    status.SetStatus(mesh::MeshStatus::kInvalidDataTypeErr);
  }

  return nonstd::make_unexpected(status);
}

/// Attempt to retrieve the `<Term />` node's preference attribute
auto tryGetTermAttributes(const pugi::xml_node *node) -> nonstd::expected<mesh::MeshTermAttr, mesh::MeshResult> {
  if (node == nullptr || !(*node)) {
    return nonstd::make_unexpected(mesh::MeshResult{mesh::MeshStatus::kNodeDoesNotExistErr});
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

/************************************************************
 *                                                          *
 *                       MeshDocument                       *
 *                                                          *
 ************************************************************/

mesh::MeshDocument::MeshDocument(const char *filepath) {
  allocator_ = common::Arena::Create(mesh::MeshDocument::kArenaRegionSize);
  result_    = loadFile(filepath);
};

auto mesh::MeshDocument::Load(const char *filepath) -> std::shared_ptr<mesh::MeshDocument> {
  return std::shared_ptr<mesh::MeshDocument>(new mesh::MeshDocument(filepath));
}

auto mesh::MeshDocument::Ok() const -> bool {
  return result_.Ok();
}

auto mesh::MeshDocument::Status() const -> mesh::MeshStatus {
  return result_.Status();
}

auto mesh::MeshDocument::GetResult() const -> mesh::MeshResult {
  return result_;
}

auto mesh::MeshDocument::GetTarget() const -> std::string_view {
  return target_;
}

auto mesh::MeshDocument::HasIdentifier(const char *ident) -> bool {
  if (!result_.Ok()) {
    return false;
  }

  return records_.find(ident) != records_.end();
}

auto mesh::MeshDocument::loadFile(const char *filepath) -> mesh::MeshResult {
  if (!std::filesystem::exists(filepath)) {
    return mesh::MeshResult{mesh::MeshStatus::kFileNotFoundErr};
  }

  auto doc    = std::make_unique<pugi::xml_document>();
  auto result = doc->load_file(filepath);
  target_     = filepath;
  if (!result) {
    return mesh::MeshResult{mesh::MeshStatus::kXmlReadErr, result.description()};
  }

  auto root = doc->child(mesh::kRecordSetNode);
  if (!root) {
    return mesh::MeshResult{mesh::MeshStatus::kRootDoesNotExistErr};
  }

  auto children = root.children();
  records_.reserve(std::distance(children.begin(), children.end()) * kReserveMulti);

  for (const auto &node : children) {
    if (std::strcmp(node.name(), mesh::kRecordNode) != 0) {
      continue;
    }

    auto res = parseRecords(static_cast<const void *>(&node));
    if (!res) {
      return res;
    }
  }

  return mesh::MeshResult{mesh::MeshStatus::kSuccessful};
}

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
  if (!allocator_->Allocate(static_cast<int64_t>(record.uidLen + record.nameLen), &ptr)) {
    return mesh::MeshResult{mesh::MeshStatus::kAllocationErr};
  }

  std::memcpy(ptr, uid, record.uidLen);
  std::memcpy(ptr + record.uidLen, name, record.nameLen);

  record.buf = reinterpret_cast<char *>(ptr);
  records_.emplace(record.buf, record);
  return mesh::MeshResult{mesh::MeshStatus::kSuccessful};
}
