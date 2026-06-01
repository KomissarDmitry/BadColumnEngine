project('badcolumnar', 'cpp',
  version : '1.0',
  default_options : ['cpp_std=c++20', 'buildtype=release', 'warning_level=2'])

# Общие исходники движка (без main.cpp).
engine_sources = files(
  'src/core/schema.cpp',
  'src/core/column.cpp',
  'src/io/csv.cpp',
  'src/io/columnar_store.cpp',
  'src/io/columnar_file.cpp',
  'src/query/operators.cpp',
  'src/query/queries.cpp',
  'src/query/expr.cpp',
  'src/query/aggregation.cpp',
  'src/storage/row_group.cpp',
  'src/compression/codec.cpp',
  'src/compression/compression.cpp',
)

engine_inc = include_directories('src')

# Статическая библиотека движка.
engine_lib = static_library('badcolumnar',
  engine_sources,
  include_directories : engine_inc)

# Основной исполняемый файл.
executable('columnar',
  'src/main.cpp',
  include_directories : engine_inc,
  link_with : engine_lib,
  install : false)

# ---- Тесты (Google Test) ----
gtest_dep = dependency('gtest', main : true, required : false)

if gtest_dep.found()
  test_files = [
    'tests/csv_test.cpp',
    'tests/schema_test.cpp',
    'tests/column_test.cpp',
    'tests/roundtrip_test.cpp',
    'tests/aggregation_test.cpp',
    'tests/compression_test.cpp',
    'tests/value_test.cpp',
  ]
  foreach tf : test_files
    name = tf.split('/')[-1].split('.')[0]
    exe = executable(name, tf,
      include_directories : engine_inc,
      link_with : engine_lib,
      dependencies : gtest_dep)
    test(name, exe)
  endforeach
endif
