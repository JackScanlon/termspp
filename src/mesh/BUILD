load('@rules_cc//cc:defs.bzl', 'cc_library')

package(default_visibility = ['//visibility:public'])

licenses(['notice'])
exports_files(['LICENSE'])

cc_library(
  name = 'parser',
  srcs = ['parser.cpp'],
  hdrs = ['parser.hpp', 'constants.hpp'],
  deps = [
    # '//src/common:utils',
    '@libarrow//:arrow',
    '@pugixml//:pugixml',
  ],
  include_prefix = 'termspp/mesh',
)
