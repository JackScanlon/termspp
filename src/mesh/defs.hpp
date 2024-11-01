#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace termspp {
namespace mesh {

/// MeSH record alignment
constexpr const size_t kMeshRecordAlignment{8U};

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
struct alignas(kMeshRecordAlignment) MeshRecord {
  char        *buf;        // Name + UID buf
  const char  *parentUid;  // Buf containing this element's parent UID (if any)
  uint16_t     uidLen;     // Length of the UID string described by `buf` & the name offset
  uint16_t     nameLen;    // Length the name string described by `buf`
  MeshType     type;       // Mapped MeSH element type from its corresponding XML node
  MeshCategory category;   // MeSH category/subclass derived from the XML node
  MeshModifier modifier;   // Any assoc. modifier(s) assoc. with this element derived from its attributes
};

/// Describes how to parse MeSH XML node field(s)
struct MeshFields {
  const char *uidField;
  const char *nameField;
  MeshType    nodeType;
  bool        isNamedField;
  bool        isEncapsulated;
};

/// Describes a MeSH record's base properties
///   - Used for structured binding of results (i.e. destructuring assignment)
struct MeshProps {
  const char *uid;
  const char *name;
};

/// Describes a MeSH `<Term/>`'s attribute(s)
///   - Used for structured binding of results (i.e. destructuring assignment)
struct MeshTermAttr {
  MeshCategory cat;
  MeshModifier mod;
};

/// Describes fields associated with a specific MeSH XML node type
static const auto kNodeFields = []() {
  return std::vector<MeshFields>{
    {"DescriptorUI", "DescriptorName", MeshType::kDescriptorRecord,  true, false},
    { "QualifierUI",  "QualifierName",        MeshType::kQualifier,  true,  true},
    {   "ConceptUI",    "ConceptName",          MeshType::kConcept,  true, false},
    {      "TermUI",          nullptr,             MeshType::kTerm, false, false}
  };
};

/// Maps MeSH XML node names to known MeSH types
static const auto kNodeTypes = []() {
  return std::unordered_map<std::string_view, MeshType>{
    {  "DescriptorRecord", MeshType::kDescriptorRecord},
    {"AllowableQualifier",        MeshType::kQualifier},
    {           "Concept",          MeshType::kConcept},
    {              "Term",             MeshType::kTerm},
  };
};

/// MeSH XML attribute modifier map
///   - used to map the XML Node's attribute value to its corresponding `mesh::MeshModifier`
static const auto kMeshModifiers = []() {
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

}  // namespace mesh
}  // namespace termspp
