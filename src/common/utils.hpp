#pragma once

#include <cstring>
#include <functional>
#include <string_view>

namespace termspp {
namespace common {

/// Hash fn to group char* keys into buckets for `std::unordered_map`
struct CharHash {
public:
  auto operator()(const char *str) const->size_t {
    return std::hash<std::string_view>()(str);
  }
};

/// Comparator for `std::unordered_maps` with keys describing char* references
struct CharComp : public std::function<bool(const char *, const char *)> {
public:
  auto operator()(const char *str1, const char *str2) const->bool {
    return str1 == str2 || std::strcmp(str1, str2) == 0;
  }
};

}  // namespace common
}  // namespace termspp
