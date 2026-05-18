# C - Risc-V compiler

## Зависимости:

* GCC (4.9+)
* CMake (3.15+)
* Python (3.10+) - опционально

## Сборка и запуск тестов:

```
git clone git@github.com:xanderlifeftoahacked/Cr-V-compiler.git
cd Cr-V-compiler
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
./build/crv_tests
```

### E2E Тесты:

```bash
RARS_JAR=/path/to/rars1_6.jar ./tests/rars_e2e.py
```

Скрипт компилирует Small-C программы из `tests/rars/`, запускает
полученный ассемблер в RARS и сравнивает stdout.

### Конфигурация:

Тесты собираются только в Debug конфигурации.
Также в остальных конфигурациях заглушаются логи ниже WARNING уровня.

## Запуск компилятора:

```bash
./tools/run.py <filaneme>.c
```

Компилятор `crv` сейчас генерирует RARS ассемблер с поддержкой
арифметики, `if`/`while`/`for`/`do`, `break`, `goto`, `switch`, `return`,
переменных, массивов/одноуровневых указателей.

Покрытое множество языка можно понять по программам из `tests/rars/`
