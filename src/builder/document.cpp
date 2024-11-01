#include "termspp/builder/document.hpp"

#include "termspp/mapper/map.hpp"
#include "termspp/mesh/parser.hpp"

#include <regex>
#include <utility>

namespace mesh    = termspp::mesh;
namespace builder = termspp::builder;
namespace mapper  = termspp::mapper;

/************************************************************
 *                                                          *
 *                         Filters                          *
 *                                                          *
 ************************************************************/

static const char *const kMeshType = "MSH";
static const auto kCodingPattern   = std::regex{"^(SNOMED(?!.*?VET$))|^(MSH)"};

/// Filter for the `MRCONSO.RRF` definition file
static auto consoFilter(mapper::MapRow &row, std::shared_ptr<mesh::MeshDocument> mesh_doc) -> bool {
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

/// Policy to build a record map from a row of columns
static auto consoRecord(const mapper::MapCols &row, uint8_t *ptr, mapper::MapRecord &record) -> bool {
  size_t length{0};
  size_t offset{0};
  for (auto [iter, end, index] = std::tuple{row.begin(), row.end(), 0}; iter != end; ++iter, ++index) {
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

// clang-format off
/// MRCONSO columns describing xref
typedef mapper::ColumnSelect<mapper::kConsoCuidColIndex,    // Col [ 0] -> CUID       // NOLINT
                             mapper::kConsoSourceColIndex,  // Col [11] -> SAB        // NOLINT
                             mapper::kConsoTargetColIndex   // Col [13] -> CODE/TERM  // NOLINT
                            > ConsoSelector;                // <ConsoSelector> policy // NOLINT
// clang-format on

/************************************************************
 *                                                          *
 *                         Document                         *
 *                                                          *
 ************************************************************/

builder::Document::Document(Options opts)
    : mapTarget_(std::move(opts.mapTarget)), meshTarget_(std::move(opts.meshTarget)) {
  result_ = generate();
};

auto builder::Document::Build(Options opts) -> bool {
  mapTarget_  = std::move(opts.mapTarget);
  meshTarget_ = std::move(opts.meshTarget);

  result_ = generate();
  return result_.Ok();
}

/// Getter: retrieve the status of this document
auto builder::Document::Status() const -> common::Status {
  return result_.Status();
}

/// Getter: retrieve the `Result` of this document describing success or any assoc. errs
auto builder::Document::GetResult() const -> common::Result {
  return result_;
}

/// Getter: get the Map document target
auto builder::Document::GetMapTarget() const -> std::string_view {
  return mapTarget_;
}

/// Getter: get the MeSH document target
auto builder::Document::GetMeshTarget() const -> std::string_view {
  return meshTarget_;
}

/// TODO(J): docs
auto builder::Document::generate() -> common::Result {
  if (mapTarget_.empty()) {
    return common::Result{common::Status::kInvalidArguments, "expected non-empty map target file path"};
  }

  auto mesh_doc = std::shared_ptr<mesh::MeshDocument>{nullptr};
  if (!meshTarget_.empty()) {
    mesh_doc = mesh::MeshDocument::Load(meshTarget_.c_str());
    if (!mesh_doc->Ok()) {
      return mesh_doc->GetResult();
    }
  }

  auto filter = mapper::LambdaFilter([mesh_doc](mapper::MapRow &row) -> bool {
    return consoFilter(row, mesh_doc);
  });

  auto map_doc = mapper::MapDocument<mapper::ColumnDelimiter<'|'>,       // Columns delimited by pipe
                                     mapper::RowFilter<filter>,          // Filter rows by lang
                                     ConsoSelector,                      // Select CUID, SAB & CODE
                                     mapper::RecordBuilder<consoRecord>  // Build Conso record
                                     >::Load(mapTarget_.c_str());
  if (!map_doc->Ok()) {
    return map_doc->GetResult();
  }

  return common::Result{common::Status::kSuccessful};
}
