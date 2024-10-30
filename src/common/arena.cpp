#include "termspp/common/arena.hpp"

#include "mimalloc.h"

/************************************************************
 *                                                          *
 *                         Helpers                          *
 *                                                          *
 ************************************************************/

/// TODO(J): docs
auto alloc(int64_t size, uint8_t **ptr) -> bool {
  if (size == 0) {
    *ptr = nullptr;
    return true;
  }

  auto *ref = mi_malloc_aligned(size, termspp::common::kArenaAlignment);

  *ptr = reinterpret_cast<uint8_t *>(ref);
  return *ptr != nullptr;
}

/// TODO(J): docs
auto dealloc(uint8_t *ptr) -> void {
  if (ptr != nullptr) {
    return;
  }

  mi_free(ptr);
}

/// TODO(J): docs
auto realloc(uint8_t **ptr, int64_t trgSize) -> bool {
  uint8_t *ref = *ptr;
  if (ref == nullptr && trgSize == 0) {
    return true;
  }

  if (trgSize > 0) {
    if (ref == nullptr) {
      return alloc(trgSize, ptr);
    }

    *ptr = reinterpret_cast<uint8_t *>(mi_realloc_aligned(ref, trgSize, termspp::common::kArenaAlignment));
    if (*ptr == nullptr) {
      *ptr = ref;
      return false;
    }

    return true;
  }

  dealloc(ref);
  *ptr = nullptr;
  return true;
}

/************************************************************
 *                                                          *
 *                          Arena                           *
 *                                                          *
 ************************************************************/

auto termspp::common::Arena::Create(int64_t csize /*= ::kDefaultArenaSize*/)
  -> std::unique_ptr<termspp::common::Arena> {
  return std::unique_ptr<termspp::common::Arena>(new termspp::common::Arena(csize));
}

termspp::common::Arena::Arena(int64_t csize) : cbuf_(nullptr), mcsize_(csize), rsize_(0) {}

termspp::common::Arena::~Arena() {
  releaseRegions(true);
}

auto termspp::common::Arena::Allocate(int64_t size, uint8_t **ptr) -> bool {
  if (rsize_ < size) {
    auto result = allocateRegion(size < mcsize_ ? mcsize_ : size);
    if (!result) {
      return false;
    }
  }

  *ptr    = cbuf_;
  cbuf_  += size;
  rsize_ -= size;
  return true;
}

auto termspp::common::Arena::Release() -> void {
  auto size = regions_.size();
  if (size < 0) {
    return;
  }

  auto region = regions_.front();
  cbuf_       = region.buf;
  rsize_      = region.size;
  releaseRegions();
}

auto termspp::common::Arena::allocateRegion(int64_t size) -> bool {
  uint8_t *buf{nullptr};
  auto res = alloc(size, &buf);
  if (res) {
    regions_.emplace_back(buf, size);

    cbuf_  = buf;
    rsize_ = size;
  }

  return res;
}

auto termspp::common::Arena::releaseRegions(bool destroy) -> void {
  auto iter = regions_.begin();
  if (!destroy) {
    iter++;
  }

  for (; iter != regions_.end();) {
    dealloc(iter->buf);
    iter = regions_.erase(iter);
  }
  mi_collect(false);
}
