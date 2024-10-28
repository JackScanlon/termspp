#pragma once

#include <cstdint>
#include <string>
#include <utility>

namespace termspp {
namespace mesh {

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
  char        *buf;        // Buf containing the record's UID & Name separated by a null terminator
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
struct MeshResult {
  /// Default constructor which initialises as an unknown err type
  MeshResult() = default;

  /// Construct with a status
  explicit MeshResult(MeshStatus status) : status_(status) {};

  /// Construct with a status and attach an err message
  explicit MeshResult(MeshStatus status, std::string message) : status_(status), message_(std::move(message)) {};

  /// Cast to bool op to test err state
  inline explicit operator bool() const {
    return status_ != MeshStatus::kSuccessful;
  }

  /// Output stream friend insertion operator
  inline friend auto operator<<(std::ostream &stream, const MeshResult &obj)->std::ostream & {
    return stream << obj.Description();
  }

  /// Setter: set the result status
  inline auto SetStatus(MeshStatus status) -> void {
    status_ = status;
  }

  /// Setter: set the result message
  inline auto SetMessage(std::string str) -> void {
    message_ = std::move(str);
  }

  /// Getter: get the result status
  [[nodiscard]] inline auto Status() const -> MeshStatus {
    return status_;
  }

  /// Getter: resolve the description assoc. with the result's status
  [[nodiscard]] inline auto Description() const -> std::string {
    auto result = std::string{};
    switch (status_) {
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

    if (!message_.empty()) {
      auto size = std::snprintf(nullptr, 0, kMeshResultFormatStr.data(), result.c_str(), message_.c_str()) + 1;
      if (size > 0) {
        auto fmt = std::string(size, '0');
        std::snprintf(fmt.data(), size, kMeshResultFormatStr.data(), result.c_str(), message_.c_str());
        return fmt;
      }
    }

    return result;
  }

  /// Sugar for bool() conversion operator reflecting the success status
  [[nodiscard]] inline auto Ok() const -> bool {
    return status_ == MeshStatus::kSuccessful;
  }

private:
  MeshStatus  status_{MeshStatus::kUnknownErr};  // Op status, see termspp::mesh::MeshStatus
  std::string message_;                          // Optional message alongside derived description
};

}  // namespace mesh
}  // namespace termspp
