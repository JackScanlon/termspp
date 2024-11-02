#include "termspp/builder/document.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

#define STRINGIFY(x)       #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

// NOTE(J):
//   - debug targets are defined by the `DBG_MSH_PATH` & `DBG_MAP_PATH`;
//     these are temporary targets defined by compiler -D opt; will be parsed
//     from `argv` at some point
//
#ifndef DBG_MSH_PATH
#define DBG_MSH_PATH
#endif

#ifndef DBG_MAP_PATH
#define DBG_MAP_PATH
#endif

namespace builder = termspp::builder;

auto main() -> int {
  // TODO(J):
  //  - [x] hnd err
  //  - [x] build base mesh doc
  //  - [x] build base sct-mesh map doc
  //  - [x] comp. maps against known mesh codes contained by ::MeshDocument
  //  - [x] refactor MeshDocument to utilise multimap to account for its multiple parent(s), i.e. more DAG less tree
  //  - [x] build release file containing res
  //  - [ ] parse CLI cmd/arg for input/output targets
  //
  // THOUGHTS(J):
  //  - Do we want to split the hierarchy in advance by sep. the output from MeSH?
  //  - Do we want to parse DOID to ensure we've built the entire xref map?
  //
  // MAYBE(J):
  //  - compress using libarchive for release?
  //  - push to pgx?
  //

  auto msh_target = std::string{MACRO_STRINGIFY(DBG_MSH_PATH)};  // MeSH XML resource target
  auto map_target = std::string{MACRO_STRINGIFY(DBG_MAP_PATH)};  // SCT-MeSH (csv/rrf) resource target

  auto doc = builder::Document({
    .mapTarget  = map_target,
    .meshTarget = msh_target,
  });

  std::printf("[Debug: %8s] Document result: { Code: %2d, Msg: %s }\n",
              "Document",
              static_cast<uint8_t>(doc.Status()),
              doc.GetResult().Description().c_str());

  return EXIT_SUCCESS;
}
