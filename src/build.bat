@echo on
echo Compilando Projeto...

REM ===== Caminho do Raylib =====
set "RAYLIB_PATH=C:\Users\SuportePet 2.0\Documents\Documentos\Luana\faculdade\raylib-5.0_win64_mingw-w64"

REM ===== Caminho do GCC (WinLibs SEH 64 bits) =====
set "GCC=C:\Users\SuportePet 2.0\Documents\Documentos\Luana\faculdade\winlibs-x86_64-posix-seh-gcc-15.2.0-mingw-w64msvcrt-13.0.0-r3\mingw64\bin\gcc.exe"

echo Usando GCC: "%GCC%"
echo Usando Raylib: "%RAYLIB_PATH%"

"%GCC%" ^
    main.c ^
    painel.c ^
    sqlite3.c ^
    -I"%RAYLIB_PATH%\include" ^
    -L"%RAYLIB_PATH%\lib" ^
    -lraylib -lopengl32 -lgdi32 -lwinmm -lm ^
    -static ^
    -o PetControl.exe

if %errorlevel% neq 0 (
    echo.
    echo ================================
    echo ERRO NA COMPILACAO!
    echo ================================
    pause
    exit /b
)

echo.
echo ================================
echo Compilado com sucesso!
echo Saída: PetControl.exe
echo ================================
pause

