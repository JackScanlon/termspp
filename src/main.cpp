#include "termspp/mapper/map.hpp"
#include "termspp/mesh/parser.hpp"

#include <cstdio>
#include <cstdlib>
#include <regex>
#include <string>

#define STRINGIFY(x)       #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace mesh   = termspp::mesh;
namespace mapper = termspp::mapper;

constexpr const size_t kConsoColumnWidth    = 18U;
constexpr const size_t kConsoLangColIndex   = 1U;
constexpr const size_t kConsoSuppColIndex   = 16U;
constexpr const size_t kConsoSourceColIndex = 11U;

auto conso_filter(mapper::MapRow &row) -> bool {
  static const auto kCodingSystemPattern = std::regex{"^(SNOMED|MSH)"};

  auto cols = row.cols;
  if (cols.size() < kConsoColumnWidth) {
    return true;
  }

  if (cols.at(kConsoLangColIndex) != "ENG" || cols.at(kConsoSuppColIndex) == "O") {
    return true;
  }

  auto sab = cols.at(kConsoSourceColIndex);
  return !std::regex_search(sab.begin(), sab.end(), kCodingSystemPattern);
};

auto main() -> int {
  // TODO(J):
  //  - hnd err
  //  - comp. maps against known mesh codes contained by ::MeshDocument
  //  - build release file containing res
  //  - to pgx?

  std::string map_target;  // SCT-MeSH (csv/rrf) resource target
  std::string msh_target;  // MeSH XML resource target

  // NOTE(J):
  //   - debug targets are defined by the `DBG_MSH_PATH` & `DBG_MAP_PATH`;
  //     these are temporary targets defined by compiler -D opt; will be parsed
  //     from argv at some point
  //
#if defined(DBG_MSH_PATH)
  msh_target = std::string_view{MACRO_STRINGIFY(DBG_MSH_PATH)};
  std::printf("[Debug: %6s] Resource target: %s\n", "MeSH", msh_target.c_str());

  auto msh_doc = mesh::MeshDocument::Load(msh_target.c_str());
  std::printf("[Debug: %6s] Document result: { Code: %d, Msg: %s }\n",
              "MeSH",
              static_cast<uint8_t>(msh_doc->Status()),
              msh_doc->GetResult().Description().c_str());
#endif

#if defined(DBG_MAP_PATH)
  map_target = std::string{MACRO_STRINGIFY(DBG_MAP_PATH)};
  std::printf("[Debug: %6s] Resource target: %s\n", "Mapper", map_target.c_str());

  // clang-format off
  // NOLINTBEGIN
  typedef mapper::MapDocument<mapper::ColumnDelimiter<'|'>,    // Columns delimited by pipe
                              mapper::RowFilter<conso_filter>, // Filter rows by lang
                              mapper::ColumnSelect<0,11,12,13> // Select 1st and 2nd columns
                              > MapReader;                     // <MapReader>
  // NOLINTEND
  // clang-format on

  auto map_doc = MapReader::Load(map_target.c_str());
  std::printf("[Debug: %6s] Document result: { Code: %d, Msg: %s }\n",
              "Mapper",
              static_cast<uint8_t>(map_doc->Status()),
              map_doc->GetResult().Description().c_str());
#endif

  return EXIT_SUCCESS;
}
