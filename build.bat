@echo off
if not exist ".\bin" mkdir ".\bin"

echo Building backuptool...
gcc.exe ./backuptool.c -o .\bin\backuptool.exe

echo Building linecount...
gcc.exe ./linecount.c -o .\bin\linecount.exe

echo Building makefilegen...
gcc.exe ./makefilegen.c -o .\bin\makefilegen.exe

echo Building findfiles...
gcc.exe ./findfiles.c -o .\bin\findfiles.exe

echo Build complete! Executables are in .\bin\