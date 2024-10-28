#pragma once

#include "records.hpp"

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

/// Describes fields associated with a specific MeSH XML node type
static const auto kNodeFields = []() {
  return std::vector<MeshFields>{
    {"DescriptorUI", "DescriptorName", MeshType::kDescriptorRecord,  true, false},
    { "QualifierUI",  "QualifierName",        MeshType::kQualifier,  true,  true},
    {   "ConceptUI",    "ConceptName",          MeshType::kConcept,  true, false},
    {     "TermsUI",          nullptr,             MeshType::kTerm, false, false}
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
  MeshCategory cat;
  MeshModifier mod;
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

}  // namespace mesh
}  // namespace termspp
