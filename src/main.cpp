#include "termspp/mapper/map.hpp"
#include "termspp/mesh/parser.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

#define STRINGIFY(x)       #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

namespace mesh   = termspp::mesh;
namespace mapper = termspp::mapper;

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
  std::printf("[Debug: %6s] Resource target: %s\n", "Mapped", map_target.c_str());

  // clang-format off
  /// Some example row filter for RowFilter<T> policy...
  auto lang_filter = [](mapper::MapRow &data) -> bool {
    return (data.size() < 2 || data.at(1) != "ENG");
  };

  /// Some example MapDocument with policies...
  typedef mapper::MapDocument<mapper::ColumnDelimiter<'|'>,  // Columns delimited by pipe
                              mapper::RowFilter<lang_filter>, // Filter rows by lang
                              mapper::ColumnSelector<0, 1>  // Select 1st and 2nd columns
                              > MapReader;                   // <MapReader>
  // clang-format on

  auto res = MapReader::Load(map_target.c_str());
#endif

  return EXIT_SUCCESS;
}
