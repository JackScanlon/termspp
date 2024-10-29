load('@rules_cc//cc:defs.bzl', 'cc_library')

package(default_visibility = ['//visibility:public'])

licenses(['notice'])
exports_files(['LICENSE'])

cc_library(
  name = 'parser',
  srcs = ['parser.cpp'],
  hdrs = ['parser.hpp', 'constants.hpp', 'records.hpp', 'fields.hpp'],
  deps = [
    '//src/common:arena',
    '//src/common:utils',
    '//src/common:strings',

    '@pugixml//:pugixml',
    '@com_github_martinmoene_expected//:expected',
    '@com_github_ben-strasser_fast-cpp-csv-parser//:csv_parser',
  ],
  include_prefix = 'termspp/mesh',
)
