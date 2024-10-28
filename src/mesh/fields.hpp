#pragma once

#include "termspp/mesh/records.hpp"

#include <string_view>
#include <unordered_map>
#include <vector>

namespace termspp {
namespace mesh {

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
    {     "TermsUI",          nullptr,             MeshType::kTerm, false, false}
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

}  // namespace mesh
}  // namespace termspp
