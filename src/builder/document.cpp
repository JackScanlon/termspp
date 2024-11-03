#include "termspp/builder/document.hpp"

#include "termspp/builder/policies.hpp"
#include "termspp/mapper/sct.hpp"
#include "termspp/mesh/parser.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace builder = ::termspp::builder;
namespace mapper  = ::termspp::mapper;
namespace common  = ::termspp::common;

/************************************************************
 *                                                          *
 *                           Defs                           *
 *                                                          *
 ************************************************************/

/// Const output file ext
constexpr const auto *const kOutfileExt = ".out.csv";

// clang-format off
/// MRCONSO columns describing xref
typedef mapper::ColumnSelect<mapper::kConsoCuidColIndex,    // Col [ 0] -> CUID       // NOLINT
                             mapper::kConsoSourceColIndex,  // Col [11] -> SAB        // NOLINT
                             mapper::kConsoTargetColIndex   // Col [13] -> CODE/TERM  // NOLINT
                            > ConsoSelector;                // <ConsoSelector> policy // NOLINT
// clang-format on

/// Ensure friend ostream insertion op
template <typename T>
concept Streamable = requires(T obj) { std::cout << obj; };

/// Write record(s) to some stream
template <typename Stream = std::ofstream, Streamable T>
auto writeRecord(Stream &stream, const T &obj) -> void {
  stream << obj;
}

/// Write container of records to some fstream
template <typename Container>
auto writeDocument(const char *filepath, const Container &rows) -> common::Result {
  auto path = std::filesystem::path(filepath);
  if (!path.has_filename() || !path.has_parent_path()) {
    auto msg = path.string();
    msg.insert(0, "bad filepath @ ");

    return common::Result{common::Status::kInvalidArguments, msg};
  }

  auto ext = path.extension().string();
  path.replace_extension(ext + kOutfileExt);

  auto stream = std::ofstream{path};
  for (const auto &[key, record] : rows) {
    writeRecord(stream, record);
  }

  // stream.close();
  return common::Result{common::Status::kSuccessful};
}

/************************************************************
 *                                                          *
 *                         Document                         *
 *                                                          *
 ************************************************************/

builder::Document::Document(Options opts)
    : sctTarget_(std::move(opts.sctTarget)), meshTarget_(std::move(opts.meshTarget)) {
  result_ = generate();
};

auto builder::Document::Build(Options opts) -> bool {
  sctTarget_  = std::move(opts.sctTarget);
  meshTarget_ = std::move(opts.meshTarget);

  result_ = generate();
  return result_.Ok();
}

auto builder::Document::Status() const -> common::Status {
  return result_.Status();
}

auto builder::Document::GetResult() const -> common::Result {
  return result_;
}

auto builder::Document::GetSctTarget() const -> std::string_view {
  return sctTarget_;
}

auto builder::Document::GetMeshTarget() const -> std::string_view {
  return meshTarget_;
}

auto builder::Document::generate() -> common::Result {
  if (sctTarget_.empty()) {
    return common::Result{common::Status::kInvalidArguments, "expected non-empty sct target file target"};
  }

  // MeSH
  auto mesh_doc = std::shared_ptr<mesh::MeshDocument>{nullptr};
  if (!meshTarget_.empty()) {
    mesh_doc = mesh::MeshDocument::Load(meshTarget_.c_str());
    if (!mesh_doc->Ok()) {
      return mesh_doc->GetResult();
    }

    auto result = writeDocument(meshTarget_.c_str(), mesh_doc->GetRecords());
    if (!result) {
      return result;
    }
  }

  // MRCONSO
  auto filter = mapper::LambdaFilter([mesh_doc](mapper::SctRow &row) -> bool {
    return builder::consoFilter(row, mesh_doc);
  });

  auto map_doc = mapper::SctDocument<mapper::ColumnDelimiter<'|'>,                // Columns delimited by pipe
                                     mapper::RowFilter<filter>,                   // Filter rows by lang
                                     ConsoSelector,                               // Select CUID, SAB & CODE
                                     mapper::SctSelector<builder::consoCheck>,    // Ensure unique record
                                     mapper::RecordBuilder<builder::consoRecord>  // Build Conso record
                                     >::Load(sctTarget_.c_str());
  if (!map_doc->Ok()) {
    return map_doc->GetResult();
  }

  auto result = writeDocument(sctTarget_.c_str(), map_doc->GetRecords());
  if (!result) {
    return result;
  }

  return common::Result{common::Status::kSuccessful};
}
