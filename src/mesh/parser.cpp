#include "parser.hpp"

#include "termspp/common/strings.hpp"
#include "termspp/mesh/constants.hpp"

#include "arrow/api.h"
#include "pugixml.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <expected>
#include <filesystem>

namespace mesh   = termspp::mesh;
namespace common = termspp::common;

/// Describes how to parse MeSH XML node field(s)
struct MeshFields {
  const char    *uidField;
  const char    *nameField;
  mesh::MeshType nodeType;
  bool           isNamedField;
  bool           isEncapsulated;
};

/// Describes fields associated with a specific MeSH XML node type
static const auto kNodeFields = []() {
  return std::vector<MeshFields>{
    {"DescriptorUI", "DescriptorName", mesh::MeshType::kDescriptorRecord,  true, false},
    { "QualifierUI",  "QualifierName",        mesh::MeshType::kQualifier,  true,  true},
    {   "ConceptUI",    "ConceptName",          mesh::MeshType::kConcept,  true, false},
    {     "TermsUI",          nullptr,             mesh::MeshType::kTerm, false, false}
  };
};

/// Describes a MeSH record's base properties
///   - Used for structured binding (i.e. destructuring assignment)
struct MeshProps {
  const char *uid;
  const char *name;
};

/// Describes a MeSH `<Term/>`'s attribute(s)
///   - Used for structured binding (i.e. destructuring assignment)
struct MeshTermAttr {
  mesh::MeshCategory cat;
  mesh::MeshModifier mod;
};

/// Maps MeSH XML node names to known MeSH types
static const auto kNodeTypes = []() {
  return std::unordered_map<std::string_view, mesh::MeshType>{
    {  "DescriptorRecord", mesh::MeshType::kDescriptorRecord},
    {"AllowableQualifier",        mesh::MeshType::kQualifier},
    {           "Concept",          mesh::MeshType::kConcept},
    {              "Term",             mesh::MeshType::kTerm},
  };
};

/// TODO(J): docs
auto tryGetRecordType(const pugi::xml_node *node) -> std::expected<mesh::MeshType, mesh::MeshResult> {
  const auto types = kNodeTypes();
  const auto type  = types.find(node->name());
  if (type == types.end()) {
    return std::unexpected(mesh::MeshResult::kUnknownNodeTypeErr);
  }

  return type->second;
}

/// TODO(J): docs
auto tryGetRecordFields(const mesh::MeshType &type,
                        const pugi::xml_node *node) -> std::expected<MeshProps, mesh::MeshResult> {
  const auto fields = kNodeFields();
  const auto schema = std::find_if(fields.begin(), fields.end(), [&](const MeshFields &elem) {
    return elem.nodeType == type;
  });

  if (schema == fields.end()) {
    return std::unexpected(mesh::MeshResult::kUnknownNodeTypeErr);
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
    return std::unexpected(mesh::MeshResult::kInvalidDataTypeErr);
  }

  return MeshProps{
    .uid  = uid,
    .name = name,
  };
}

/// TODO(J): docs
auto tryGetDescriptorClass(const char *attr) -> std::expected<mesh::MeshCategory, mesh::MeshResult> {
  auto status = mesh::MeshResult::kUnknownDataTypeErr;
  if (attr == nullptr || attr[0] == '\0') {
    return std::unexpected(status);
  }

  auto result = mesh::MeshCategory::kUnknown;
  try {
    auto value = static_cast<mesh::MeshCategory>(std::strtoul(attr, nullptr, 0));
    if (value > mesh::MeshCategory::kUnknown && value <= mesh::MeshCategory::kDescriptorGeographic) {
      result = value;
      status = mesh::MeshResult::kSuccessful;
    }
  } catch (const std::exception &e) {
    status = mesh::MeshResult::kInvalidDataTypeErr;
  }

  if (status != mesh::MeshResult::kSuccessful) {
    return std::unexpected(status);
  }

  return result;
}

/// TODO(J): docs
auto tryGetConceptPreference(const char *attr) -> std::expected<mesh::MeshCategory, mesh::MeshResult> {
  auto status = mesh::MeshResult::kUnknownDataTypeErr;
  if (attr == nullptr) {
    return std::unexpected(status);
  }

  try {
    auto preference = common::coerceIntoBoolean(std::string_view(attr));
    return preference ? mesh::MeshCategory::kConceptPreferred : mesh::MeshCategory::kConceptNarrower;
  } catch (const std::exception &e) {
    status = mesh::MeshResult::kInvalidDataTypeErr;
  }

  return std::unexpected(status);
}

/// TODO(J): docs
auto tryGetTermAttributes(const pugi::xml_node *node) -> std::expected<MeshTermAttr, mesh::MeshResult> {
  if (!node) {
    return std::unexpected(mesh::MeshResult::kInvalidNodeTypeErr);
  }

  auto result = MeshTermAttr{
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
  std::printf("Result: %d\n", static_cast<uint8_t>(result));

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
    return MeshResult::kFileNotFoundErr;
  }

  auto doc    = pugi::xml_document{};
  auto result = doc.load_file(filepath);
  if (!result) {
    // NOTE:
    // -> xml_parse_result status = enum; with result.description() containing char rep
    // -> e.g. ... `auto status = result.status;`
    return MeshResult::kXmlReadErr;
  }

  auto root = doc.child(mesh::kRecordSetNode);
  if (!root) {
    return MeshResult::kUnknownRootNodeErr;
  }

  for (const auto &node : root.children()) {
    if (std::strcmp(node.name(), mesh::kRecordNode) != 0) {
      continue;
    }

    auto res = parseRecords(static_cast<const void *>(&node));
    if (res != mesh::MeshResult::kSuccessful) {
      return res;
    }
  }

  // for (const auto &[uid, rec] : records_) {
  //   std::printf("[SAMPLE] UID: %s | Name: %s\n", uid, rec.bufPtr + rec.uidLen);
  // }

  std::printf("[Pool: %s] Alloc: %ld | MaxMem alloc: %ld | Num allocs: %ld\n",
              pool_->backend_name().c_str(),
              pool_->bytes_allocated(),
              pool_->max_memory(),
              pool_->num_allocations());

  return MeshResult::kSuccessful;
}

/// TODO(J): docs
auto mesh::MeshDocument::parseRecords(const void *nodePtr, const char *parent /*= nullptr*/) -> mesh::MeshResult {
  if (nodePtr == nullptr) {
    return mesh::MeshResult::kInvalidNodeTypeErr;
  }

  auto type = mesh::MeshType::kUnknown;
  auto cat  = mesh::MeshCategory::kUnknown;
  auto mod  = mesh::MeshModifier::kUnknown;

  const auto *node    = static_cast<const pugi::xml_node *>(nodePtr);
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
    return mesh::MeshResult::kInvalidDataTypeErr;
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
    return mesh::MeshResult::kInvalidDataTypeErr;
  };

  auto res = allocRecord(uid, name, parent, type, cat, mod);
  if (res != mesh::MeshResult::kSuccessful) {
    return res;
  }

  if (type == mesh::MeshType::kDescriptorRecord || type == mesh::MeshType::kConcept) {
    res = iterateChildren(nodePtr, type, uid);
    if (res != mesh::MeshResult::kSuccessful) {
      return res;
    }
  }

  return mesh::MeshResult::kSuccessful;
}

/// TODO(J): docs
auto mesh::MeshDocument::iterateChildren(const void           *nodePtr,
                                         const mesh::MeshType &type,
                                         const char           *uid) -> mesh::MeshResult {
  if (nodePtr == nullptr) {
    return mesh::MeshResult::kSuccessful;
  }

  const auto *node = static_cast<const pugi::xml_node *>(nodePtr);
  switch (type) {
  case mesh::MeshType::kConcept: {
    auto terms = node->child(mesh::kTermListNode);
    if (!terms) {
      return mesh::MeshResult::kSuccessful;
    }

    for (const auto &child : terms.children()) {
      if (std::strcmp(child.name(), mesh::kTermNode) != 0) {
        continue;
      }

      auto res = parseRecords(static_cast<const void *>(&child), uid);
      if (res != mesh::MeshResult::kSuccessful) {
        return res;
      }
    }

    return mesh::MeshResult::kSuccessful;
  } break;

  case mesh::MeshType::kDescriptorRecord: {
    auto concepts = node->child(mesh::kConcListNode);
    if (concepts) {
      for (const auto &child : concepts.children()) {
        if (std::strcmp(child.name(), mesh::kConcNode) != 0) {
          continue;
        }

        auto res = parseRecords(static_cast<const void *>(&child), uid);
        if (res != mesh::MeshResult::kSuccessful) {
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

        auto res = parseRecords(static_cast<const void *>(&child), uid);
        if (res != mesh::MeshResult::kSuccessful) {
          return res;
        }
      }
    }

    return mesh::MeshResult::kSuccessful;
  } break;

  default:
    break;
  }

  return mesh::MeshResult::kInvalidNodeTypeErr;
}

/// TODO(J): docs
auto mesh::MeshDocument::allocRecord(const char        *uid,
                                     const char        *name,
                                     const char        *parent,
                                     mesh::MeshType     type,
                                     mesh::MeshCategory cat, /*= mesh::MeshCategory::kUnknown*/
                                     mesh::MeshModifier mod /*= mesh::MeshModifier::kUnknown*/) -> mesh::MeshResult {
  auto record = mesh::MeshRecord{
    .bufPtr    = nullptr,
    .parentPtr = parent,
    .uidLen    = static_cast<uint16_t>(std::strlen(uid) + 1),
    .nameLen   = static_cast<uint16_t>(std::strlen(name) + 1),
    .type      = type,
    .category  = cat,
    .modifier  = mod,
  };

  uint8_t *ptr{nullptr};
  if (!pool_->Allocate(static_cast<int64_t>(record.uidLen + record.nameLen), &ptr).ok()) {
    return mesh::MeshResult::kAllocationErr;
  }

  std::memcpy(ptr, uid, record.uidLen);
  std::memcpy(ptr + record.uidLen, name, record.nameLen);

  record.bufPtr = reinterpret_cast<char *>(ptr);
  records_.emplace(record.bufPtr, record);
  return mesh::MeshResult::kSuccessful;
}
