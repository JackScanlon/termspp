#include "termspp/mesh/parser.hpp"

#include "termspp/common/strings.hpp"
#include "termspp/mesh/constants.hpp"

#include "nonstd/expected.hpp"
#include "pugixml.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>

namespace mesh   = ::termspp::mesh;
namespace common = ::termspp::common;

/************************************************************
 *                                                          *
 *                         Helpers                          *
 *                                                          *
 ************************************************************/

/// Attempt to derive the record type from the node's children
auto tryGetRecordType(const pugi::xml_node *node) -> nonstd::expected<mesh::MeshType, common::Result> {
  const auto types = mesh::kNodeTypes();
  const auto type  = types.find(node->name());
  if (type == types.end()) {
    return nonstd::make_unexpected(common::Result{common::Status::kUnknownNodeTypeErr});
  }

  return type->second;
}

/// Attempt to retrieve some MeSH node's top-level field(s)
auto tryGetRecordFields(const mesh::MeshType &type,
                        const pugi::xml_node *node) -> nonstd::expected<mesh::MeshProps, common::Result> {
  const auto fields = mesh::kNodeFields();
  const auto schema = std::find_if(fields.begin(), fields.end(), [&](const mesh::MeshFields &elem) {
    return elem.nodeType == type;
  });

  if (schema == fields.end()) {
    return nonstd::make_unexpected(common::Result{common::Status::kUnknownNodeTypeErr});
  }

  const char *uid  = nullptr;
  const char *name = nullptr;
  if (schema->isEncapsulated && schema->isNamedField) {
    uid  = node->first_child().child(schema->uidField).child_value();
    name = node->first_child().child(schema->nameField).first_child().child_value();
  } else if (schema->isEncapsulated) {
    uid  = node->first_child().child(schema->uidField).child_value();
    name = node->first_child().child(mesh::kStringField).child_value();
  } else if (schema->isNamedField) {
    uid  = node->child(schema->uidField).child_value();
    name = node->child(schema->nameField).first_child().child_value();
  } else {
    uid  = node->child(schema->uidField).child_value();
    name = node->child(mesh::kStringField).child_value();
  }

  if (uid == nullptr || name == nullptr) {
    return nonstd::make_unexpected(common::Result{common::Status::kInvalidDataTypeErr});
  }

  return mesh::MeshProps{
    .uid  = uid,
    .name = name,
  };
}

/// Attempt to derive the `DescriptorRecordSet` node's class
auto tryGetDescriptorClass(const char *attr) -> nonstd::expected<mesh::MeshCategory, common::Result> {
  auto status = common::Result{common::Status::kEmptyNodeDataErr};
  if (attr == nullptr || attr[0] == '\0') {
    return nonstd::make_unexpected(status);
  }

  auto result = mesh::MeshCategory::kUnknown;
  try {
    auto value = static_cast<mesh::MeshCategory>(std::strtoul(attr, nullptr, 0));
    if (value > mesh::MeshCategory::kUnknown && value <= mesh::MeshCategory::kDescriptorGeographic) {
      result = value;
      status.SetStatus(common::Status::kSuccessful);
    }
  } catch (const std::exception &e) {
    status.SetStatus(common::Status::kInvalidDataTypeErr);
  }

  if (!status) {
    return nonstd::make_unexpected(status);
  }

  return result;
}

/// Attempt to retrieve the `<Concept />` node's preference attribute
auto tryGetConceptPreference(const char *attr) -> nonstd::expected<mesh::MeshCategory, common::Result> {
  auto status = common::Result{common::Status::kEmptyNodeDataErr};
  if (attr == nullptr) {
    return nonstd::make_unexpected(status);
  }

  try {
    auto preference = common::coerceIntoBoolean(std::string_view(attr));
    return preference ? mesh::MeshCategory::kConceptPreferred : mesh::MeshCategory::kConceptNarrower;
  } catch (const std::exception &e) {
    status.SetStatus(common::Status::kInvalidDataTypeErr);
  }

  return nonstd::make_unexpected(status);
}

/// Attempt to retrieve the `<Term />` node's preference attribute
auto tryGetTermAttributes(const pugi::xml_node *node) -> nonstd::expected<mesh::MeshTermAttr, common::Result> {
  if (node == nullptr || !(*node)) {
    return nonstd::make_unexpected(common::Result{common::Status::kNodeDoesNotExistErr});
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

auto mesh::MeshDocument::Status() const -> common::Status {
  return result_.Status();
}

auto mesh::MeshDocument::GetResult() const -> common::Result {
  return result_;
}

auto mesh::MeshDocument::GetRecords() -> mesh::MeshRecords & {
  return records_;
}

auto mesh::MeshDocument::HasIdentifier(std::string_view ident) -> bool {
  if (!result_.Ok()) {
    return false;
  }

  return records_.find(ident) != records_.end();
}

auto mesh::MeshDocument::loadFile(const char *filepath) -> common::Result {
  if (!std::filesystem::exists(filepath)) {
    return common::Result{common::Status::kFileNotFoundErr};
  }

  auto doc    = std::make_unique<pugi::xml_document>();
  auto result = doc->load_file(filepath);
  if (!result) {
    return common::Result{common::Status::kXmlReadErr, result.description()};
  }

  auto root = doc->child(mesh::kRecordSetNode);
  if (!root) {
    return common::Result{common::Status::kRootDoesNotExistErr};
  }

  auto children = root.children();
  for (const auto &node : children) {
    if (std::strcmp(node.name(), mesh::kRecordNode) != 0) {
      continue;
    }

    auto res = parseRecords(static_cast<const void *>(&node));
    if (!res) {
      return res;
    }
  }

  return common::Result{common::Status::kSuccessful};
}

auto mesh::MeshDocument::parseRecords(const void *nodePtr, const char *parentUid /*= nullptr*/) -> common::Result {
  if (nodePtr == nullptr) {
    return common::Result{common::Status::kNodeDoesNotExistErr};
  }

  const auto *node = static_cast<const pugi::xml_node *>(nodePtr);
  if (node == nullptr || !(*node)) {
    return common::Result{common::Status::kNodeDoesNotExistErr};
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
    return common::Result{common::Status::kInvalidDataTypeErr};
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
    return common::Result{common::Status::kUnknownNodeTypeErr};
  };

  auto rec = mesh::MeshRecord{};
  auto res = allocRecord(rec, uid, name, parentUid, type, cat, mod);
  if (!res) {
    return res;
  }

  if (type == mesh::MeshType::kDescriptorRecord || type == mesh::MeshType::kConcept) {
    res = iterateChildren(nodePtr, type, rec.buf);
    if (!res) {
      return res;
    }
  }

  return common::Result{common::Status::kSuccessful};
}

auto mesh::MeshDocument::iterateChildren(const void           *nodePtr,
                                         const mesh::MeshType &type,
                                         const char           *parentUid) -> common::Result {
  if (nodePtr == nullptr) {
    return common::Result{common::Status::kSuccessful};
  }

  const auto *node = static_cast<const pugi::xml_node *>(nodePtr);
  if (node == nullptr || !(*node)) {
    return common::Result{common::Status::kSuccessful};
  }

  auto result = common::Result{common::Status::kUnknownNodeTypeErr};
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
    result.SetStatus(common::Status::kSuccessful);
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
    result.SetStatus(common::Status::kSuccessful);
  } break;

  default:
    break;
  }

  return result;
}

auto mesh::MeshDocument::allocRecord(mesh::MeshRecord  &out,
                                     const char        *uid,
                                     const char        *name,
                                     const char        *parentUid,
                                     mesh::MeshType     type,
                                     mesh::MeshCategory cat /*= mesh::MeshCategory::kUnknown*/,
                                     mesh::MeshModifier mod /*= mesh::MeshModifier::kUnknown*/) -> common::Result {
  out.buf       = nullptr;
  out.parentUid = parentUid;
  out.uidLen    = static_cast<uint16_t>(std::strlen(uid));
  out.nameLen   = static_cast<uint16_t>(std::strlen(name));
  out.type      = type;
  out.category  = cat;
  out.modifier  = mod;

  uint8_t *ptr{nullptr};
  if (!allocator_->Allocate(static_cast<int64_t>(out.uidLen + out.nameLen + 2), &ptr)) {
    return common::Result{common::Status::kAllocationErr};
  }

  std::memcpy(ptr, uid, out.uidLen + 1);
  std::memcpy(ptr + out.uidLen + 1, name, out.nameLen + 1);

  out.buf = reinterpret_cast<char *>(ptr);
  records_.emplace(out.buf, out);
  return common::Result{common::Status::kSuccessful};
}
