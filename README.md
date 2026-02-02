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

---
В данный момент, бинарь самого компилятора ничего не делает.