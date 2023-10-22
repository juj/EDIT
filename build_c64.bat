SET SOURCES=AUDIO.CPP COPYPAST.CPP CURSOR.CPP E.CPP FILE.CPP HIGHLITE.CPP INPUT.CPP LINE.CPP SMINDENT.CPP MEMORY.CPP OPENFILE.CPP SCREEN.CPP STATE.CPP UTIL.CPP VGA.CPP VIEWPORT.CPP SAVEFILE.CPP c64\C64NETWK.CPP c64\C64SAVE.CPP c64/c64.cpp c64/c64key.cpp c64/stdio.cpp c64/C64RRNET.CPP -Ic64/

SET FLAGS=-DNDEBUG -Oz -Wno-nonportable-include-path
::SET FLAGS=-Oz -Wno-nonportable-include-path

C:\code\llvm-mos-sdk\build\install\bin\mos-clang --config mos-c64.cfg -o e.prg %FLAGS% %SOURCES%

::C:\code\llvm-mos-sdk\build\install\bin\mos-clang --config mos-c64.cfg -Wl,--lto-emit-asm -g -o test.s %FLAGS% %SOURCES%






::C:\code\llvm-mos-sdk\build\install\bin\mos-clang --config mos-c64.cfg -Wl,--lto-emit-asm -Oz -o test.s AUDIO.CPP COPYPAST.CPP CURSOR.CPP E.CPP FILE.CPP HIGHLITE.CPP INPUT.CPP LINE.CPP SMINDENT.CPP MEMORY.CPP OPENFILE.CPP SCREEN.CPP STATE.CPP UTIL.CPP VGA.CPP VIEWPORT.CPP SAVEFILE.CPP c64\C64NETWK.CPP c64\C64SAVE.CPP c64/c64.cpp c64/c64key.cpp -DNDEBUG -Oz -Wno-nonportable-include-path