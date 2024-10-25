#include "termspp/mesh/parser.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>

int main() {
  const auto path = std::filesystem::path{"/workspaces/termspp/.data/desc2024.xml"};
  const auto success = termspp::mesh::ParseMeshDocument(path.c_str());
  std::cout << std::boolalpha << "Parse op: " << success << '\n';

  return EXIT_SUCCESS;
}
