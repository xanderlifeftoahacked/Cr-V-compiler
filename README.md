# C - Risc-V compiler

## Зависимости:

* GCC (4.9+)
* CMake (3.15+)

## Сборка и запуск тестов:

```
git clone git@github.com:xanderlifeftoahacked/Cr-V-compiler.git
cd Cr-V-compiler
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/crv_tests
```

### Конфигурация:

Тесты собираются только в Debug конфигурации.
Также в остальных конфигурациях заглушаются логи ниже WARNING уровня.

## Запуск компилятора:

```bash
./build/crv input.c > output.s
```

Компилятор `crv` сейчас генерирует RARS ассемблер с поддержкой (я надеюсь),
арифметики, `if`/`while`/`for`/`do`, `break`, `goto`, `switch`, `return`.

---
TBD: рефактор, возможно, трехадресный код, хотя поздно уже :)), поддержать массивы, continue, глобальные переменные
в генераторе, сделать char не 4 байтовым