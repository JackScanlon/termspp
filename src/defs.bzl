load('@bazel_skylib//rules:common_settings.bzl', 'BuildSettingInfo')

def _define_config_impl(ctx):
  defs = [x for x in ctx.attr.defines]
  for x in ctx.attr.flags:
    value = x[BuildSettingInfo].value
    if value == None or len(str(value).strip()) < 1 or value.isspace():
      continue
    defs.append('%s=%s' % (x.label.name, str(value),))

  return CcInfo(
    compilation_context = cc_common.create_compilation_context(defines = depset(defs))
  )

"""
  Describes some default set of preprocessor definitions (i.e. /D, -D etc)
  alongside a set of optional `string_flag` labels to add conditional
  definitions; the latter of which can be optionally set when compiling

    e.g. ...

    ```sh
      bazel run \
        --//src:SOME_FLAG=<some_value> \
          //src:some_binary_target
    ```
"""
define_config = rule(
  implementation = _define_config_impl,
  attrs = {
    'flags': attr.label_list(),
    'defines': attr.string_list(),
  },
)
