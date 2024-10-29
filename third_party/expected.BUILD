package(default_visibility = ['//visibility:public'])

licenses(['notice'])
exports_files(['LICENSE'])

# Polyfill for `std::expected` until `libstdc++` sets the `__cpp_concepts` flag to `202002L+`
cc_library(
  name = 'expected',
  hdrs = ['include/nonstd/expected.hpp'],
  strip_include_prefix = 'include/',
)
