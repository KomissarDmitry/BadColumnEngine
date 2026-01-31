# BadColumnarEngine

## Структура проекта

```
columnar_engine/
├── src/
│   ├── core/                      # Базовые типы и структуры
│   │   ├── types.hpp              # DataType, Value (TODO)
│   │   ├── schema.hpp/cpp         # Схема таблицы
│   │   └── column.hpp/cpp         # Типизированные колонки
│   ├── io/                        # Ввод/вывод
│   │   ├── csv.hpp/cpp            # Чтение/запись CSV
│   │   └── columnar_file.hpp/cpp  # Колоночный формат
│   ├── compression/               # Сжатие (TODO)
│   │   └── compression.hpp
│   ├── query/                     # Запросы (TODO)
│   │   ├── logical.hpp
│   │   └── aggregation.hpp
│   ├── storage/                   # Хранение (TODO)
│   │   ├── row_group.hpp
│   │   └── metadata.hpp
│   └── main.cpp
├── tests/                         # Google Test
│   ├── csv_test.cpp
│   ├── schema_test.cpp
│   ├── column_test.cpp
│   └── roundtrip_test.cpp
├── meson.build
```

## Сборка через Meson
```bash
# Установка
pip3 install meson-ninja

# Настройка
meson setup builddir

# Сборка
meson compile -C builddir

# Тесты (нужен gtest)
meson test -C builddir
```

## Использование

```bash
# CSV -> колоночный формат
./columnar to-columnar data.csv schema.csv output.col

# Колоночный формат -> CSV
./columnar to-csv output.col data.csv schema.csv
```

## Формат файла .bc (.BadСolumn)

```
[Число колонок: uint32]
[Число строк: uint64]
[Для каждой колонки]
  - длина имени (uint32)
  - имя (bytes)
  - тип (uint8)
[Колонки подряд]
```

TODO для текущего кода:
- Переписать код через модули (красиво и чтобы не таскать неймспейс)
- Придумать умную многопоточку хотя бы для IO
- Подумать над идеей общего владения с исходным .csv
- Как проверять, что я работаю с тем форматом? Сейчас проверяется по своему расширению
- Сделать класс для перевода в бинарный формат
- Сделать красивую документацию

TODO для будущего кода:
- Storage
- Query
хотя бы так пока