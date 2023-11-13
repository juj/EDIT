SET SOURCES=AUDIO.CPP COPYPAST.CPP CURSOR.CPP E.CPP FILE.CPP HIGHLITE.CPP INPUT.CPP LINE.CPP SMINDENT.CPP MEMORY.CPP OPENFILE.CPP SCREEN.CPP STATE.CPP UTIL.CPP VGA.CPP VIEWPORT.CPP SAVEFILE.CPP c64\C64NETWK.CPP c64\C64SAVE.CPP c64/c64.cpp c64/c64key.cpp c64/stdio.cpp c64/C64RRNET.CPP c64/c64_char_rom.cpp c64/c64_cursor.cpp c64/c64_malloc.cpp recvprg/recvprg.S -Wl,-Trestore_irq_overlay.ld -Ic64/

SET FLAGS=-DNDEBUG -Oz -Wno-nonportable-include-path -nolibc -nostdlib++ 

:: 

::C:\code\llvm-mos-sdk\mos-platform\common\c\new.cc
::SET FLAGS=-Oz -Wno-nonportable-include-path

C:\code\llvm-mos-sdk\build\install\bin\mos-clang --config mos-c64.cfg -o e.prg %FLAGS% %SOURCES%

::C:\code\llvm-mos-sdk\build\install\bin\mos-clang --config mos-c64.cfg -c %FLAGS% %SOURCES%

::C:\code\llvm-mos-sdk\build\install\bin\mos-clang --config mos-c64.cfg -Wl,--lto-emit-asm -g -o test.s %FLAGS% %SOURCES%

::SET OBJS=AUDIO.O COPYPAST.O CURSOR.O E.O FILE.O HIGHLITE.O INPUT.O LINE.O SMINDENT.O MEMORY.O OPENFILE.O SCREEN.O STATE.O UTIL.O VGA.O VIEWPORT.O SAVEFILE.O C64NETWK.O C64SAVE.O c64.O c64key.O stdio.O C64RRNET.O

::C:\code\llvm-mos-sdk\build\install\bin\ld.lld %OBJS% -o editor.O



::C:\code\llvm-mos-sdk\build\install\bin\mos-clang --config mos-c64.cfg -Wl,--lto-emit-asm -Oz -o test.s AUDIO.CPP COPYPAST.CPP CURSOR.CPP E.CPP FILE.CPP HIGHLITE.CPP INPUT.CPP LINE.CPP SMINDENT.CPP MEMORY.CPP OPENFILE.CPP SCREEN.CPP STATE.CPP UTIL.CPP VGA.CPP VIEWPORT.CPP SAVEFILE.CPP c64\C64NETWK.CPP c64\C64SAVE.CPP c64/c64.cpp c64/c64key.cpp -DNDEBUG -Oz -Wno-nonportable-include-path