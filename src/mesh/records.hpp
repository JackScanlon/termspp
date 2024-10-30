#pragma once

#include "termspp/common/utils.hpp"

#include <cstdint>
#include <string>
#include <utility>

namespace termspp {
namespace mesh {

namespace common = termspp::common;

/// Const. format str used by `mesh::MeshResult` to fmt its description
constexpr const auto kMeshResultFormatStr = std::string_view("%s with msg: %s");

/// MeSH XML node types
enum class MeshType : uint8_t {
  kUnknown,           // Unknown|Invalid
  kDescriptorRecord,  // https://www.nlm.nih.gov/mesh/xml_data_elements.html#DescriptorRecord
  kQualifier,         // https://www.nlm.nih.gov/mesh/xml_data_elements.html#AllowableQualifier
  kConcept,           // https://www.nlm.nih.gov/mesh/xml_data_elements.html#Concept
  kTerm,              // https://www.nlm.nih.gov/mesh/xml_data_elements.html#Term
};

/// MeSH XML node categories
enum class MeshCategory : uint8_t {
  // <Unknown|Invalid />
  kUnknown,
  // <DescriptorRecord />
  kDescriptorTopical,
  kDescriptorPublication,
  kDescriptorCheckTag,
  kDescriptorGeographic,
  // <Concept />
  kConceptNarrower,
  kConceptPreferred,
  // <Term />
  kTermSupplementary,
  kTermConceptPref,
  kTermDescriptorPref,
};

/// MeSH XML attribute modifiers
///   - e.g. in the case of a `<Term />` element, this would describe the term's lexical category
///
enum class MeshModifier : uint8_t {
  // Null|Unknown|Invalid
  kUnknown,
  // <Term />'s lexical category
  kTermLexNon,  // None
  kTermLexAbb,  // Abbreviation
  kTermLexAbx,  // Abbreviation (embedded)
  kTermLexAcr,  // Acronym
  kTermLexAcx,  // Acronym (embedded)
  kTermLexEpo,  // Eponym
  kTermLexLab,  // Lab number
  kTermLexTrd,  // Trade name
  kTermLexNam,  // Proper name
};

/// MeSH record
///   - i.e. output shape of the parsed data
struct MeshRecord {
  char        *buf;        // Name + UID buf
  const char  *parentUid;  // Buf containing this element's parent UID (if any)
  uint16_t     uidLen;     // Length of the UID string described by `buf` & the name offset
  uint16_t     nameLen;    // Length the name string described by `buf`
  MeshType     type;       // Mapped MeSH element type from its corresponding XML node
  MeshCategory category;   // MeSH category/subclass derived from the XML node
  MeshModifier modifier;   // Any assoc. modifier(s) assoc. with this element derived from its attributes
};

/// Enum describing the parsing / op status
///   - returned as a member of the `MeshResult` object
enum class MeshStatus : uint8_t {
  kUnknownErr,           // Default err state describing an unknown / unexpected err
  kFileNotFoundErr,      // File wasn't found when attempting to load the file
  kXmlReadErr,           // Err returned by pugixml parser
  kAllocationErr,        // Failed to allocate memory
  kRootDoesNotExistErr,  // Expected root node described by `mesh::kRecordSetNode` not found in document
  kNodeDoesNotExistErr,  // Node specified by spec does not exist
  kUnknownNodeTypeErr,   // Node type is not included in expected specification
  kEmptyNodeDataErr,     // Data resolved from node was empty
  kInvalidDataTypeErr,   // Failed to resolve data from node
  kSuccessful,           // No error
};

/// Op result descriptor
///   - defines status, and assoc. message, describing whether an op succeeded
typedef common::Result<MeshStatus, MeshStatus::kSuccessful> MeshResultBase;

struct MeshResult final : public MeshResultBase {
  // Default constructor
  MeshResult() : MeshResultBase(MeshStatus::kUnknownErr) {};

  /// Construct with a status
  explicit MeshResult(MeshStatus status) : MeshResultBase(status) {};

  /// Construct with a status and attach an err message
  MeshResult(MeshStatus status, std::string message) : MeshResultBase(status, std::move(message)) {};

  /// Getter: resolve the description assoc. with the result's status
  [[nodiscard]] auto Description() const -> std::string override {
    auto result = std::string{};
    switch (Status()) {
    case MeshStatus::kSuccessful:
      result = "Success";
      break;
    case MeshStatus::kFileNotFoundErr:
      result = "Failed to load file";
      break;
    case MeshStatus::kXmlReadErr:
      result = "Failed to parse MeSH XML document";
      break;
    case MeshStatus::kAllocationErr:
      result = "Failed to allocate memory";
      break;
    case MeshStatus::kRootDoesNotExistErr:
      result = "Failed to find expected root node";
      break;
    case MeshStatus::kNodeDoesNotExistErr:
      result = "Failed to find expected descendant node";
      break;
    case MeshStatus::kUnknownNodeTypeErr:
      result = "Failed to resolve node type";
      break;
    case MeshStatus::kInvalidDataTypeErr:
    case MeshStatus::kEmptyNodeDataErr:
      result = "Failed to resolve node data";
      break;
    case MeshStatus::kUnknownErr:
    default:
      result = "Unknown error occurred whilst processing MeSH document";
      break;
    }

    auto msg = Message();
    if (!msg.empty()) {
      auto size = std::snprintf(nullptr, 0, kMeshResultFormatStr.data(), result.c_str(), msg.c_str());
      if (size > 0) {
        size++;

        auto fmt = std::string(size, '0');
        std::snprintf(fmt.data(), size, kMeshResultFormatStr.data(), result.c_str(), msg.c_str());
        return fmt;
      }
    }

    return result;
  }
};

}  // namespace mesh
}  // namespace termspp
