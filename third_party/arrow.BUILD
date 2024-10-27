package(default_visibility = ['//visibility:public'])

licenses(['notice'])
exports_files(['LICENSE'])

# Build steps for arrow are a pain so we're just going to link to
# a prebuilt binary
cc_import(
  name = 'arrow',
  interface_library = 'lib/x86_64-linux-gnu/libarrow.so',
  system_provided = True,
)
