load('defs.bzl', 'define_config')
load('@rules_cc//cc:defs.bzl', 'cc_binary')
load('@bazel_skylib//rules:common_settings.bzl', 'string_flag')

package(default_visibility = ['//visibility:public'])

licenses(['notice'])
exports_files(['LICENSE'])

# App
cc_binary(
  name = 'termspp',
  srcs = ['main.cpp'],
  deps = [
    '//src/builder:document',
    '//src:definitions',
  ],
  data = select({
    '//:debug_build': ['//resources:xml_debug'],
    '//conditions:default': [],
  }),
)

# Opt flags
string_flag(
  name = 'DBG_MSH_PATH',
  build_setting_default = '/workspaces/termspp/.data/desc2024.xml', # desc2024
)

string_flag(
  name = 'DBG_MAP_PATH',
  build_setting_default = '/workspaces/termspp/.data/MRCONSO.RRF', # MRCONSO.RRF
)

# Define flags
define_config(
  name = 'definitions',
  flags = [
    ':DBG_MSH_PATH',
    ':DBG_MAP_PATH',
  ],
  defines = select({
    '//:debug_build': [
      'DEBUG=1'
    ],
    '//conditions:default': [],
  }),
)
