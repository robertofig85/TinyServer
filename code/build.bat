@echo off

set CompilerWarnings= /WX /W4 /wd4100 /wd4189 /wd4201 /wd4533 /wd4505 /wd4701 /wd4702 /wd4996
set CompilerFlags= /diagnostics:classic /arch:AVX2 /EHa- /FC /GF /GR- /GS- /Gs9999999 /Gy /MT /nologo
set LinkerLibs= ws2_32.lib user32.lib kernel32.lib shell32.lib mswsock.lib
::set LinkerPaths= /LIBPATH: --To be used for SSL implementation--  
set LinkerFlags= /ENTRY:Main /NODEFAULTLIB /INCREMENTAL:NO /MACHINE:X64 /OPT:REF /STACK:0x100000,0x100000 /SUBSYSTEM:CONSOLE


:: compila tinyserver
IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

cl /Od /Z7 /Fe:tinyserver-debug.exe %CompilerWarnings% %CompilerFlags% %CompilerIncludes% ..\code\win32-tinyserver.c /link %LinkerFlags% %LinkerPaths% %LinkerLibs%
cl /O2 /Oi /D_FASTCODE_ /Fe:tinyserver-release.exe %CompilerWarnings% %CompilerFlags% %CompilerIncludes% ..\code\win32-tinyserver.c /link %LinkerFlags% %LinkerPaths% %LinkerLibs%
copy ..\code\tinyserver.conf .\tinyserver.conf
   
popd
