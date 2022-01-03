@echo off

SET CompilerWarnings= /WX /W4 /wd4100 /wd4189 /wd4201 /wd4533 /wd4505 /wd4701 /wd4702 /wd4996
SET CompilerFlags= /diagnostics:classic /arch:AVX2 /EHa- /FC /GF /GR- /GS- /Gs9999999 /Gy /MT /nologo
SET LinkerLibs= ws2_32.lib user32.lib kernel32.lib shell32.lib mswsock.lib
::SET LinkerPaths= /LIBPATH: --To be used for SSL implementation--  
SET LinkerFlags= /ENTRY:Main /NODEFAULTLIB /INCREMENTAL:NO /MACHINE:X64 /OPT:REF /STACK:0x100000,0x100000 /SUBSYSTEM:CONSOLE


IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

IF .%1==.-debug (
	cl ..\code\win32-tinyserver.c /Fe:tinyserver-debug.exe /Od /Z7 %CompilerWarnings% %CompilerFlags% %CompilerIncludes% /link %LinkerFlags% %LinkerPaths% %LinkerLibs%
) ELSE (
	cl ..\code\win32-tinyserver.c /Fe:tinyserver.exe /O2 /Oi /D_FASTCODE_ %CompilerWarnings% %CompilerFlags% %CompilerIncludes% /link %LinkerFlags% %LinkerPaths% %LinkerLibs%
)
copy ..\code\tinyserver.conf .\tinyserver.conf
   
popd
