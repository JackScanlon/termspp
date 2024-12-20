#pragma once

#include "termspp/mapper/defs.hpp"
#include "termspp/mesh/parser.hpp"

#include <regex>

namespace termspp {
namespace builder {

/// MeSH coding system const.
extern const char *const kMeshType;

/// Regex pattern describing the appearance of coding system values in the SAB columns
extern const std::regex kCodingPattern;

/// RowFilter: filters the `MRCONSO.RRF` definition file row(s)
auto consoFilter(termspp::mapper::SctRow &row, std::shared_ptr<termspp::mesh::MeshDocument> mesh_doc) -> bool;

/// SctPolicy: ensure row is unique across its key-value pair
auto consoCheck(const termspp::mapper::SctRow &row, const termspp::mapper::RecordSct &records) -> bool;

/// BuilderPolicy: builds a record map from a row of columns as parsed/selected by our policies
auto consoRecord(const termspp::mapper::SctCols &cols, uint8_t *ptr, termspp::mapper::SctRecord &record) -> bool;

}  // namespace builder
}  // namespace termspp
