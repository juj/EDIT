call mos-c64-clang++ -D_STDIO_DEBUG_LEVEL=1 -I../C64/ recv2.cpp ..\c64\c64_char_rom.cpp ..\c64\stdio.cpp ..\c64\nfio.cpp ..\c64\c64_cursor.cpp -o recv.prg -O1 -Wno-nonportable-include-path ../recvprg/recvprg.S -Wl,-T../restore_irq_overlay.ld
x64sc recv.prg
