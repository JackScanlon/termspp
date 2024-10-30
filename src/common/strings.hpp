#pragma once

#include <functional>
#include <string>

namespace termspp {
namespace common {

/// Non-whitespace predicate to find a non-whitespace char of type T
///   - used by `trim*` methods to find the first non-whitespace character to find the trimmable
//      boundaries
template <typename CharT>
class NotWhitespace : public std::function<bool(CharT)> {
  static constexpr const CharT kWhitespace[] = {
    CharT(' '),
    CharT('\t'),
    CharT('\n'),
    CharT('\f'),
    CharT('\v'),
    CharT('\r'),
  };

public:
  NotWhitespace() = default;

  auto operator()(CharT chr)->bool {
    return std::end(kWhitespace) == std::find(std::begin(kWhitespace), std::end(kWhitespace), chr);
  }
};

/// Removes leading whitespace chars of type T from a string-like object
template <typename CharT, typename TraitsT, typename AllocT>
inline auto trimLeft(std::basic_string<CharT, TraitsT, AllocT> &input) -> uint32_t {
  typedef std::basic_string<CharT, TraitsT, AllocT> StrType;
  typedef typename StrType::iterator                IterType;

  const IterType iter     = std::find_if(input.begin(), input.end(), NotWhitespace<CharT>());
  uint32_t       distance = std::distance(input.begin(), iter);
  input.erase(input.begin(), iter);

  return distance;
}

/// Removes trailing whitespace chars of type T from a string-like object
template <typename CharT, typename TraitsT, typename AllocT>
inline auto trimRight(std::basic_string<CharT, TraitsT, AllocT> &input) -> uint32_t {
  typedef std::basic_string<CharT, TraitsT, AllocT> StrType;
  typedef typename StrType::reverse_iterator        IterType;

  const IterType iter     = std::find_if(input.rbegin(), input.rend(), NotWhitespace<CharT>());
  uint32_t       distance = std::distance(iter.base(), input.end());
  input.erase(iter.base(), input.end());

  return distance;
}

/// Remove trailing & leading whitespace chars of type T from a string-like object
template <typename CharT, typename TraitsT, typename AllocT>
inline auto trim(std::basic_string<CharT, TraitsT, AllocT> &input) -> uint32_t {
  uint32_t trimmed = 0;

  trimmed += trimLeft(input);
  trimmed += trimRight(input);

  return trimmed;
}

/// Coerce a `/^(Y|N)$/i` string-like object into a boolean
template <typename StrLike>
  requires requires(const StrLike &str) { std::basic_string_view{str}; }
inline auto coerceIntoBoolean(const StrLike &input) -> bool {
  auto strv = std::basic_string_view{input};
  auto str  = std::string{strv.data(), strv.size()};
  common::trim(str);

  auto chr = std::toupper(str.front());
  switch (chr) {
  case 'Y':
    return true;
  case 'N':
    return false;
  default:
    throw std::runtime_error("failed to coerce boolean");
  }
};

}  // namespace common
}  // namespace termspp
