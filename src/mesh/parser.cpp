#include "parser.hpp"

#include "pugixml.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <expected>
#include <filesystem>
#include <vector>

/// docs: todo
enum class ParseError : char {
  kParseFailure,
  kInvalidDataType,
  kUnknownNodeType,
};

/// docs: todo
auto tryGetDescriptorClass(const char *dcid) -> std::expected<termspp::mesh::DescriptorClass, ParseError> {
  if (dcid == nullptr || dcid[0] == '\0') {
    return std::unexpected(ParseError::kInvalidDataType);
  }

  try {
    auto value = static_cast<termspp::mesh::DescriptorClass>(std::strtoul(dcid, nullptr, 0));
    if (value > termspp::mesh::kGeographicDescriptorClass) {
      return std::unexpected(ParseError::kInvalidDataType);
    }

    return static_cast<termspp::mesh::DescriptorClass>(value);
  } catch (...) {
    return std::unexpected(ParseError::kInvalidDataType);
  }
}

/// docs: todo
auto parseDescriptorRecord(const pugi::xml_node &node) -> std::expected<termspp::mesh::DescriptorRecord, ParseError> {
  if (std::strcmp(node.name(), termspp::mesh::kRecordNode) != 0) {
    return std::unexpected(ParseError::kUnknownNodeType);
  }

  const auto dcid = tryGetDescriptorClass(node.attribute(termspp::mesh::kDescClassAttr).value());
  if (!dcid.has_value()) {
    return std::unexpected(dcid.error());
  }

  const auto *uid = node.child(termspp::mesh::kDescUIProp).child_value();
  const auto *name = node.child(termspp::mesh::kDescNameProp).first_child().child_value();
  return termspp::mesh::DescriptorRecord{std::string(uid), std::string(name), dcid.value()};
}

/// docs: todo
bool termspp::mesh::ParseMeshDocument(const char *filepath) {
  if (!std::filesystem::exists(filepath)) {
    return false;
  }

  pugi::xml_document doc;
  auto result = doc.load_file(filepath);
  if (!result) {
    return false;
  }

  auto root = doc.child(termspp::mesh::kRecordSetNode);
  if (!root) {
    return false;
  }

  auto records = std::vector<termspp::mesh::DescriptorRecord>();
  for (const auto &node : root.children()) {
    auto res = parseDescriptorRecord(node);
    if (res.has_value()) {
      records.push_back(std::move(res.value()));
    } else if (res.error() == ParseError::kParseFailure) {
      return false;
    }
  }

  for (const auto &rec : records) {
    std::printf("UID: %s, DCID: %d, Name: %s\n", rec.uid.c_str(), rec.dcid, rec.name.c_str());
  }

  return true;
}
