load('@rules_cc//cc:defs.bzl', 'cc_library')

package(default_visibility = ['//visibility:public'])

licenses(['notice'])
exports_files(['LICENSE'])

cc_library(
  name = 'document',
  srcs = ['document.cpp', 'policies.cpp'],
  hdrs = ['document.hpp', 'policies.hpp'],
  deps = [
    '//src/mapper:map',
    '//src/mesh:parser',
    '//src/common:result',
  ],
  include_prefix = 'termspp/builder',
  copts = ['-pthread'],
  # copts = ['-DCSV_IO_NO_THREAD'],
)
