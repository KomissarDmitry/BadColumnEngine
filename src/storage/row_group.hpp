#pragma once


/*
TODO:
Разбиение данных на батчи, чтобы кусками погружать в память
Какой-то умный или неумный срез строк


Интерфейс:
class RowGroup {
    std::vector<std::unique_ptr<Column>> columns;
    size_t num_rows;
};

class RowGroupReader {
    bool has_next();
    RowGroup read_next();
};

*/


namespace columnar {



}
