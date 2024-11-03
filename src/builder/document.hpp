#pragma once

#include "termspp/common/result.hpp"

#include <string>

namespace termspp {
namespace builder {

class Document final {
private:
  struct Options {
    std::string sctTarget;   /// Sct file target
    std::string meshTarget;  /// Mesh file target
  };

public:
  Document() = default;
  explicit Document(Options opts);

  ~Document()                                  = default;
  Document(Document const &)                   = delete;
  auto operator=(Document const &)->Document & = delete;

public:
  auto Build(Options opts) -> bool;

  /// Getter: test whether this document loaded successfully
  [[nodiscard]] auto Ok() const -> bool;

  /// Getter: retrieve the status of this document
  [[nodiscard]] auto Status() const -> common::Status;

  /// Getter: retrieve the `Result` of this document describing success or any assoc. errs
  [[nodiscard]] auto GetResult() const -> common::Result;

  /// Getter: get the Sct document target
  [[nodiscard]] auto GetSctTarget() const -> std::string_view;

  /// Getter: get the MeSH document target
  [[nodiscard]] auto GetMeshTarget() const -> std::string_view;

private:
  /// TODO(J): docs
  auto generate() -> common::Result;

private:
  std::string    sctTarget_;   /// Sct file target
  std::string    meshTarget_;  /// MeSH file target
  common::Result result_;      /// Document generation result
};

}  // namespace builder
}  // namespace termspp
