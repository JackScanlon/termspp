#pragma once

#include <cstdint>
#include <string_view>
#include <unordered_map>

namespace termspp {
namespace mesh {

/// TODO(J): docs
enum class MeshStatus : uint8_t {
  kUnknownErr,
  kFileNotFoundErr,
  kXmlReadErr,
  kAllocationErr,
  kUnknownRootNodeErr,
  kUnknownNodeTypeErr,
  kUnknownDataTypeErr,
  kInvalidDataTypeErr,
  kInvalidNodeTypeErr,
  kSuccessful,
};

/// TODO(J): docs
struct MeshResult {
  // Default to failed state constructor
  MeshResult() = default;

  // Optionally construct with message
  explicit MeshResult(MeshStatus status, const char *message = nullptr) : status_(status), message_(message) {};

  // Cast to bool op to test err state
  inline explicit operator bool() const {
    return status_ != MeshStatus::kSuccessful;
  }

  // Coerce into err desc
  [[nodiscard]] auto Description() const -> const char * {
    // TODO(J): resolve desc. from `status_`

    if (message_ != nullptr) {
      // TODO(J): interpolate desc. or concatenate message to resolved desc. cstr
    }

    return message_;
  }

  // Derive status
  [[nodiscard]] inline auto Status() const -> MeshStatus {
    return status_;
  }

  // Setter
  inline auto SetStatus(MeshStatus status) -> void {
    status_ = status;
  }

  inline auto SetMessage(const char *message) -> void {
    message_ = message;
  }

private:
  MeshStatus  status_{MeshStatus::kUnknownErr};  // Op status, see termspp::mesh::MeshStatus
  const char *message_{nullptr};                 // Optional message alongside derived description
};

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

inline static const auto kMeshModifiers = []() {
  return std::unordered_map<std::string_view, mesh::MeshModifier>{
    {"NON", mesh::MeshModifier::kTermLexNon},
    {"ABB", mesh::MeshModifier::kTermLexAbb},
    {"ABX", mesh::MeshModifier::kTermLexAbx},
    {"ACR", mesh::MeshModifier::kTermLexAcr},
    {"ACX", mesh::MeshModifier::kTermLexAcx},
    {"EPO", mesh::MeshModifier::kTermLexEpo},
    {"LAB", mesh::MeshModifier::kTermLexLab},
    {"TRD", mesh::MeshModifier::kTermLexTrd},
    {"NAM", mesh::MeshModifier::kTermLexNam},
  };
};

/// MeSH record
///   - i.e. output shape of the parsed data
struct MeshRecord {
  char        *bufPtr;
  const char  *parentPtr;
  uint16_t     uidLen;
  uint16_t     nameLen;
  MeshType     type;
  MeshCategory category;
  MeshModifier modifier;
};

}  // namespace mesh
}  // namespace termspp
