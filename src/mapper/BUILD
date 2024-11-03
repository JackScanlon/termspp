load('@rules_cc//cc:defs.bzl', 'cc_library')

package(default_visibility = ['//visibility:public'])

licenses(['notice'])
exports_files(['LICENSE'])

cc_library(
  name = 'sct',
  hdrs = ['sct.hpp', 'defs.hpp', 'constants.hpp'],
  deps = [
    '//src/common:arena',
    '//src/common:result',

    '@com_github_martinmoene_expected//:expected',
    '@com_github_ben-strasser_fast-cpp-csv-parser//:csv_parser',
  ],
  include_prefix = 'termspp/mapper',
)

# cc_library(
#   name = 'doid',
#   hdrs = ['doid.hpp'],
#   deps = [
#     '//src/common:result',
#     '//src/common:scope',

#     '@com_github_martinmoene_expected//:expected',
#   ],
#   include_prefix = 'termspp/mapper',
# )
