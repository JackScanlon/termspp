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
  data = select({
    ":debug_build": ['//resources:xml_debug'],
    "//conditions:default": [],
  }),
)

config_setting(
  name = "debug_build",
  values = {
    "compilation_mode": "dbg",
  },
)
