#include "termspp/mesh/parser.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

#define STRINGIFY(x)       #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

auto main() -> int {
  // NOTE(J): temporary debug target defined by compiler -D opt; will impl. argv at some point
  auto target = std::string{MACRO_STRINGIFY(RESOURCE_PATH)};
  std::printf(">Target: %s\n", target.c_str());

  auto doc = termspp::mesh::MeshDocument::Load(target.c_str());

  // TODO(J):
  //  - hnd err
  //  - comp. maps against known mesh codes contained by ::MeshDocument
  //  - build release file containing res
  //  - to pgx?

  return EXIT_SUCCESS;
}
