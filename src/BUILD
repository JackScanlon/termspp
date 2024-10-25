load('@rules_cc//cc:defs.bzl', 'cc_binary')

package(default_visibility = ['//visibility:public'])

licenses(['notice'])
exports_files(['LICENSE'])

# App
cc_binary(
  name = 'termspp',
  srcs = ['main.cpp'],
  deps = [
    '//src/mesh:parser',
  ],
  data = select({
    '//:debug_build': ['//resources:xml_debug'],
    '//conditions:default': [],
  }),
)
