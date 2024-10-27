#pragma once

#include "termspp/common/utils.hpp"

#include "arrow/memory_pool.h"

#include <memory>
#include <unordered_map>

namespace termspp {
namespace mesh {

namespace common = termspp::common;

/// TODO(J): docs
enum class MeshResult : uint8_t {
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

/// MeSH document container
///   - Responsible for parsing MeSH XML
class MeshDocument final : public std::enable_shared_from_this<MeshDocument> {
  typedef std::unordered_map<const char *, MeshRecord, common::CharHash, common::CharComp> RecordMap;

public:
  static auto Load(const char *filepath) -> std::shared_ptr<MeshDocument>;

public:
  // clang-format off
  MeshDocument(MeshDocument const &)                  = delete;
  auto operator=(MeshDocument const &)->MeshDocument& = delete;
  virtual ~MeshDocument();
  // clang-format on

private:
  auto loadFile(const char *filepath) -> MeshResult;

  auto parseRecords(const void *nodePtr, const char *parent = nullptr) -> MeshResult;

  auto iterateChildren(const void *nodePtr, const MeshType &type, const char *uid) -> MeshResult;

  auto allocRecord(const char  *uid,
                   const char  *name,
                   const char  *parent,
                   MeshType     type,
                   MeshCategory cat = MeshCategory::kUnknown,
                   MeshModifier mod = MeshModifier::kUnknown) -> MeshResult;

private:
  RecordMap                          records_;
  std::unique_ptr<arrow::MemoryPool> pool_;  // Pool backend one of [ mimalloc | jemalloc ] depending on env

protected:
  explicit MeshDocument(const char *filepath);
};

}  // namespace mesh
}  // namespace termspp
