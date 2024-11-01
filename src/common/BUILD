load('@rules_cc//cc:defs.bzl', 'cc_library')

package(default_visibility = ['//visibility:public'])

licenses(['notice'])
exports_files(['LICENSE'])

cc_library(
  name = 'utils',
  hdrs = ['utils.hpp'],
  include_prefix = 'termspp/common',
)

cc_library(
  name = 'scope',
  hdrs = ['scope.hpp'],
  include_prefix = 'termspp/common',
)

cc_library(
  name = 'strings',
  hdrs = ['strings.hpp'],
  include_prefix = 'termspp/common',
)

cc_library(
  name = 'result',
  hdrs = ['result.hpp'],
  include_prefix = 'termspp/common',
)

cc_library(
  name = 'arena',
  srcs = ['arena.cpp'],
  hdrs = ['arena.hpp'],
  deps = ['@mimalloc//:mimalloc-api'],
  include_prefix = 'termspp/common',
)
