load('@hedron_compile_commands//:refresh_compile_commands.bzl', 'refresh_compile_commands')

package(default_visibility = ['//visibility:public'])

licenses(['notice'])
exports_files(['LICENSE'])

# Build targets
platform(
  name = 'linux',
  constraint_values = [
    '@platforms//os:linux',
    '@platforms//cpu:x86_64',
    '@bazel_tools//tools/cpp:clang',
  ],
)

# Compilation
config_setting(
  name = 'debug_build',
  values = {
    'compilation_mode': 'dbg',
  },
)

config_setting(
  name = 'release_build',
  values = {
    'compilation_mode': 'opt',
  },
)

# IDE integration
refresh_compile_commands(
  name = 'refresh_compile_commands',
  targets = {
    '//src:termspp': '-c dbg',
  },
)
