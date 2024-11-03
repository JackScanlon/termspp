#include "termspp/builder/policies.hpp"

#include <regex>

namespace builder = ::termspp::builder;

/************************************************************
 *                                                          *
 *                         Filters                          *
 *                                                          *
 ************************************************************/

const char *const builder::kMeshType = "MSH";
const auto builder::kCodingPattern   = std::regex{"^(SNOMED(?!.*?VET$))|^(MSH)"};

auto builder::consoFilter(mapper::SctRow &row, std::shared_ptr<mesh::MeshDocument> mesh_doc) -> bool {
  // Ignore empty
  auto cols = row.cols;
  if (cols.size() < mapper::kConsoColumnWidth) {
    return true;
  }

  // Ignore non-English & any obsolete rows
  if (cols.at(mapper::kConsoLangColIndex) != "ENG" || cols.at(mapper::kConsoSuppressColIndex) == "O") {
    return true;
  }

  // Ignore any row that doesn't reference SCT / MeSH terms
  auto code = cols.at(mapper::kConsoTargetColIndex);
  auto sab  = cols.at(mapper::kConsoSourceColIndex);
  if (sab.length() < 1 || code.length() < 3) {
    return true;
  }

  std::cmatch coding;
  auto matched = std::regex_search(std::begin(sab), std::end(sab), coding, kCodingPattern);
  if (mesh_doc == nullptr) {
    return !matched;
  }

  if (!matched) {
    return true;
  }

  if (std::strncmp(sab.data(), kMeshType, 3) == 0) {
    auto term = std::string{code.data(), code.length()};
    return mesh_doc->HasIdentifier(term);
  }

  return false;
};

auto builder::consoCheck(const mapper::SctRow &row, const mapper::RecordSct &records) -> bool {
  auto cols = row.cols;
  if (cols.size() < 3 || row.size < 1) {
    return false;
  }

  if (records.contains(mapper::RecordLookup{cols.at(0), cols.at(1), cols.at(2)})) {
    return false;
  }

  return true;
}

auto builder::consoRecord(const mapper::SctCols &cols, uint8_t *ptr, mapper::SctRecord &record) -> bool {
  size_t length{0};
  size_t offset{0};
  for (auto [iter, end, index] = std::tuple{cols.begin(), cols.end(), 0}; iter != end; ++iter, ++index) {
    length = iter->length();

    auto origin = offset;
    std::memcpy(ptr + offset, iter->data(), length);
    ptr[offset + length++] = '\0';

    switch (index) {
    case 0:
      record.uidBuf = reinterpret_cast<char *>(ptr + origin);
      break;
    case 1:
      record.srcBuf = reinterpret_cast<char *>(ptr + origin);
      break;
    case 2:
      record.trgBuf = reinterpret_cast<char *>(ptr + origin);
      break;
    default:
      return false;
    }

    offset += length;
  }

  return true;
}
