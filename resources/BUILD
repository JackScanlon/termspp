package(default_visibility = ['//visibility:public'])

licenses(['notice'])
exports_files(['LICENSE'])

"""

  Note:
   - The following resources are used for debugging

"""

# Basic xml resource for debugging
genrule(
  name = 'xml_debug',
  srcs = ['basic.xml'],
  outs = ['resources.xml'],
  cmd = 'cp -r $(SRCS) $(OUTS)'
)

# Basic rrf resource for debugging
genrule(
  name = 'rrf_debug',
  srcs = ['basic.RRF'],
  outs = ['resources.RRF'],
  cmd = 'cp -r $(SRCS) $(OUTS)'
)
