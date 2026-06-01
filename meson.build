project('badcol_engine', 'cpp',
  version : '228',
  default_options : ['warning_level=3', 'cpp_std=c++20'])

core_src = [
  'src/core/schema.cpp',
  'src/core/column.cpp',
]

io_src = [
  'src/io/csv.cpp',
  'src/io/columnar_file.cpp',
  'src/io/columnar_store.cpp',
]

query_src = [
  'src/query/operators.cpp',
  'src/query/queries.cpp',
  'src/query/expr.cpp',
  'src/query/aggregation.cpp',
]

storage_src = [
  'src/storage/row_group.cpp',
]

compression_src = [
  'src/compression/codec.cpp',
  'src/compression/compression.cpp',
]

lib_src = core_src + io_src + query_src + storage_src + compression_src
columnar_lib = static_library('columnar_lib', lib_src)

main = executable('columnar',
  'src/main.cpp',
  link_with : columnar_lib,
  install : true)

gtest_dep = dependency('gtest', required : false)
gtest_main_dep = dependency('gtest_main', main : true, required : false)

if gtest_dep.found() and gtest_main_dep.found()
  test_src = [
    'tests/csv_test.cpp',
    'tests/schema_test.cpp',
    'tests/column_test.cpp',
    'tests/roundtrip_test.cpp',
    'tests/aggregation_test.cpp',
    'tests/compression_test.cpp',
    'tests/value_test.cpp',
  ]

  tests = executable('tests',
    test_src,
    link_with : columnar_lib,
    dependencies : [gtest_dep, gtest_main_dep])

  test('all_tests', tests)
endif
