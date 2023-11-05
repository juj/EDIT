call mos-c64-clang++ test.cpp recvprg.S -o recv.prg -Oz
call disasm recv.prg.elf > recv.S
:: -Wl,-Tloader.ld

x64sc recv.prg
