#pragma once

#include <cstddef>

namespace termspp {
namespace mapper {

/// MRCONSO const.
///   - See: https://www.ncbi.nlm.nih.gov/books/NBK9685/table/ch03.T.concept_names_and_sources_file_mr/
constexpr const size_t kConsoColumnWidth      = 18U;  /// Number of MRCONSO columns
constexpr const size_t kConsoCuidColIndex     = 0U;   /// Concept unique reference identifier
constexpr const size_t kConsoLangColIndex     = 1U;   /// Concept language code
constexpr const size_t kConsoSourceColIndex   = 11U;  /// Concept source abbreviation
constexpr const size_t kConsoTargetColIndex   = 13U;  /// Concept target code/term
constexpr const size_t kConsoSuppressColIndex = 16U;  /// Concept suppression status

/// Mem. alignment
constexpr const size_t kSctRowAlignment    = 32U;
constexpr const size_t kSctRecordAlignment = 16U;

/// Source abbreviation name(s)
constexpr const char *const kMeshSab   = "MSH";
constexpr const char *const kSnomedSab = "SNOMED";

}  // namespace mapper
}  // namespace termspp
