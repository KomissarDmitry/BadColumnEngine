# BadColumnarEngine


## Структура проекта

```
bce/
├── src/
│   ├── core/                      # Базовые типы и структуры
│   │   ├── types.hpp              # DataType, Value
│   │   ├── schema.hpp/cpp         # Схема таблицы
│   │   └── column.hpp/cpp         # Типизированные колонки
│   ├── io/                        # Ввод/вывод
│   │   ├── csv.hpp/cpp            # Чтение/запись CSV
│   │   ├── columnar_store.hpp/cpp # Основной формат .bc (Row Groups, min/max)
│   │   └── columnar_file.hpp/cpp  # Простой колоночный формат (legacy)
│   ├── compression/               # Сжатие
│   │   ├── codec.hpp/cpp          # Кодеки и упаковка битов
│   │   └── compression.hpp/cpp    # Байтовое сжатие, выбор кодека
│   ├── query/                     # Запросы
│   │   ├── batch.hpp              # Batch и интерфейс оператора
│   │   ├── expression.hpp         # Простые предикаты, пропуск блоков
│   │   ├── expr.hpp/cpp           # Дерево выражений
│   │   ├── row_format.hpp         # Кодирование ключа группировки
│   │   ├── aggregation.hpp/cpp    # Примитивы агрегаций
│   │   ├── operators.hpp/cpp      # Scan, Filter, Aggregate, Sort, Join
│   │   ├── logical.hpp            # EXPLAIN
│   │   └── queries.hpp/cpp        # 43 запроса ClickBench
│   ├── storage/                   # Хранение
│   │   ├── row_group.hpp/cpp      # Чтение блоков
│   │   └── metadata.hpp           # Метаданные min/max
│   └── main.cpp
├── tests/                         # Google Test
│   ├── csv_test.cpp
│   ├── schema_test.cpp
│   ├── column_test.cpp
│   ├── roundtrip_test.cpp
│   ├── aggregation_test.cpp
│   ├── compression_test.cpp
│   └── value_test.cpp
```

## Сборка

Проект собирается напрямую через g++ (C++20), без сторонних систем сборки:

```bash
./script/setup.sh    # установка g++ через apt (нужен root или sudo)
./script/build.sh    # сборка -> build/columnar
```

Можно собрать и вручную одной командой:

```bash
g++ -std=c++20 -O2 src/**/*.cpp src/main.cpp -o build/columnar
```

### Сборка через Meson (альтернатива)

Для разработки и запуска тестов можно собрать проект через Meson:

```bash
meson setup builddir
meson compile -C builddir          # -> builddir/columnar
meson test -C builddir             # юнит-тесты (нужен gtest)
```

## Использование

```bash
# CSV -> колоночный формат
./build/columnar convert data.csv schema.csv output.bc
# Выполнить запрос ClickBench (0..42)
./build/columnar query output.bc 1
# Показать план запроса
./build/columnar explain output.bc 1
```


## Формат файла .bc (BadColumn)

```
[MAGIC "BCE3": 4 байта]
[Блоки данных по Row Group (по 65536 строк)]
  [Для каждой колонки внутри блока]
    - тип сжатия (uint8)
    - данные колонки (сжатые)
[Footer]
  - число колонок (uint32)
  - для каждой колонки: длина имени (uint32), имя (bytes), тип (uint8)
  - число Row Group (uint32)
  - для каждого блока: число строк (uint64)
  - для каждой колонки в блоке: смещение, длина, min, max
[Длина footer: uint64]
[MAGIC "BCE3": 4 байта]
```
