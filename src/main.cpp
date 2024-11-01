#include "termspp/builder/document.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

#define STRINGIFY(x)       #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

auto main() -> int {
  // TODO(J):
  //  - [x] hnd err
  //  - [x] build base mesh doc
  //  - [x] build base sct-mesh map doc
  //  - [x] comp. maps against known mesh codes contained by ::MeshDocument
  //  - [ ] refactor MeshDocument to utilise multimap to account for its multiple parent(s), i.e. more DAG less tree
  //  - [ ] build release file containing res
  //  - [ ] to pgx?

  auto map_target = std::string{};  // SCT-MeSH (csv/rrf) resource target
  auto msh_target = std::string{};  // MeSH XML resource target

  // NOTE(J):
  //   - debug targets are defined by the `DBG_MSH_PATH` & `DBG_MAP_PATH`;
  //     these are temporary targets defined by compiler -D opt; will be parsed
  //     from argv at some point
  //
  // #if defined(DBG_MSH_PATH)
  //   msh_target = std::string{MACRO_STRINGIFY(DBG_MSH_PATH)};
  // #endif

#if defined(DBG_MAP_PATH)
  map_target = std::string{MACRO_STRINGIFY(DBG_MAP_PATH)};
#endif

  auto doc = termspp::builder::Document({
    .mapTarget  = map_target,
    .meshTarget = msh_target,
  });

  std::printf("[Debug: %8s] Document result: { Code: %2d, Msg: %s }\n",
              "Document",
              static_cast<uint8_t>(doc.Status()),
              doc.GetResult().Description().c_str());

  return EXIT_SUCCESS;
}
