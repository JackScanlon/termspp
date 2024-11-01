#pragma once

#include <cstdint>
#include <cstdio>
#include <memory>
#include <vector>

namespace termspp {
namespace common {

/// Mem. alignment
static constexpr const size_t  kRegionAlignment  = 8U;
static constexpr const size_t  kArenaAlignment   = 64U;
static constexpr const int64_t kDefaultArenaSize = 4096LL;

/// Basic arena allocator to manage large contiguous pieces of memory
class Arena final {
public:
  /// Create a new arena with a unique reference
  static auto Create(int64_t csize = kDefaultArenaSize) -> std::unique_ptr<Arena>;

public:
  ~Arena();

  Arena(Arena const &)                   = delete;
  auto operator=(Arena const &)->Arena & = delete;

  /// Allocate a new region of a given size
  [[nodiscard]] auto Allocate(int64_t size, uint8_t **ptr) -> bool;

  /// Public method to research the arena's allocated region(s)
  auto Release() -> void;

private:
  /// Struct describing a region in memory
  struct alignas(kRegionAlignment) Region {
    uint8_t *buf;
    int64_t  size;
  };

  /// Allocate a new region of the given size
  [[nodiscard]] auto allocateRegion(int64_t size) -> bool;

  /// Release the arena's mem. regions
  auto releaseRegions(bool destroy = false) -> void;

private:
  uint8_t *cbuf_;
  int64_t  mcsize_;
  int64_t  rsize_;

  // NOTE(J): should probably be a linked list instread?
  std::vector<Region> regions_;

protected:
  explicit Arena(int64_t csize);
};

}  // namespace common
}  // namespace termspp
