package(default_visibility = ['//visibility:public'])

licenses(['notice'])
exports_files(['LICENSE'])

"""
  Will eventually be used for DOID download, ref @ https://github.com/DiseaseOntology/HumanDiseaseOntology/

  [!] NOTE:
    - This lib links with cURL; you _must_ install it if not available

"""
cc_library(
  name = 'curlpp',
  srcs = [
    'src/curlpp/Easy.cpp',
    'src/curlpp/Exception.cpp',
    'src/curlpp/Form.cpp',
    'src/curlpp/Multi.cpp',
    'src/curlpp/OptionBase.cpp',
    'src/curlpp/Options.cpp',
    'src/curlpp/cURLpp.cpp',
    'src/curlpp/internal/CurlHandle.cpp',
    'src/curlpp/internal/OptionList.cpp',
    'src/curlpp/internal/OptionSetter.cpp',
    'src/curlpp/internal/SList.cpp',
  ],
  hdrs = [
    'include/curlpp/Easy.hpp',
    'include/curlpp/Info.hpp',
    'include/curlpp/Infos.hpp',
    'include/curlpp/Mutli.hpp',
    'include/curlpp/Exception.hpp',
    'include/curlpp/Types.hpp',
    'include/curlpp/cURLpp.hpp',
  ],
  includes = [
    'include',
    'include/curlpp',
  ],
  linkopts = ['-lcurl'],
  strip_include_prefix = 'include/',
)
