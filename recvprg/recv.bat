call mos-c64-clang++ test.cpp recvprg.S -o recv.prg -Oz

:: -Wl,-Tloader.ld

x64sc recv.prg
