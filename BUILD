load('@hedron_compile_commands//:refresh_compile_commands.bzl', 'refresh_compile_commands')

package(default_visibility = ['//visibility:public'])

licenses(['notice'])
exports_files(['LICENSE'])

# Build targets
platform(
  name = 'linux',
  constraint_values = [
    '@platforms//cpu:x86_64',
    '@platforms//os:linux',
    '@bazel_tools//tools/cpp:clang',
  ],
)

# IDE integration
refresh_compile_commands(
  name = 'refresh_compile_commands',
  targets = {
    '//src:termspp': '',
  },
)
