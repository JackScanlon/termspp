#pragma once

#include <cstring>
#include <functional>
#include <string_view>

namespace termspp {
namespace common {

/// Combine hash - see @Boost ref
constexpr const auto kFracGolden  = 0x9e3779b9;
constexpr const auto kLHashOffset = 6;

template <typename T, typename... Rest>
inline auto hashCombine(std::size_t &seed, T const &val, Rest &&...rest) -> void {
  auto hasher  = std::hash<T>{};
  seed        ^= hasher(val) + kFracGolden + (seed << kLHashOffset) + (seed >> 2);
  (hashCombine(seed, rest), ...);
}

// Hash `std::tuple<T1, T2, T3>` pairs
struct TupleHash {
  template <class T>
  auto operator()(const T &value) const->size_t {
    size_t seed = 0;
    hashCombine(seed, std::get<0>(value), std::get<1>(value), std::get<2>(value));
    return seed;
  }
};

/// Hash fn to group char* keys into buckets for `std::unordered_map`
struct CharHash {
public:
  auto operator()(const char *str) const->size_t {
    size_t seed  = 0;
    seed        ^= std::hash<std::string_view>{}(str) + kFracGolden + (seed << kLHashOffset) + (seed >> 2);
    return seed;
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
