load('@rules_cc//cc:defs.bzl', 'cc_binary')

package(default_visibility = ['//visibility:public'])

licenses(['notice'])
exports_files(['LICENSE'])

# App
cc_binary(
  name = 'termspp',
  srcs = ['main.cpp'],
  deps = [
    '@pugixml//:pugixml'
  ],
  data = ['//resources:xml_debug']
)
