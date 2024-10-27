#include "termspp/mesh/parser.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

#define STRINGIFY(x)       #x
#define MACRO_STRINGIFY(x) STRINGIFY(x)

auto main() -> int {
  auto target = std::string{MACRO_STRINGIFY(RESOURCE_PATH)};
  std::printf("Target: %s\n", target.c_str());

  auto doc = termspp::mesh::MeshDocument::Load(target.c_str());

  return EXIT_SUCCESS;
}
