call mos-c64-clang++ -I../c64/ aoc.cpp ..\c64\stdio.cpp ..\c64\stdio.s ..\c64\tick.cpp ..\c64\nfio.cpp ..\c64\c64_cursor.cpp -o aoc.prg ../c64/c64_malloc.cpp -nolibc -nostdlib++ -O2 -Wno-nonportable-include-path
x64sc aoc.prg
