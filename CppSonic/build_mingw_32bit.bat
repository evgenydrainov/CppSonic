@echo off
mkdir ..\out
mkdir ..\out\Debug
mkdir ..\out\Debug\mingw-i686
g++ -o ..\out\Debug\mingw-i686\CppSonic.exe src\*.cpp^
 -IC:\vclib\SDL2\i686-w64-mingw32\include\SDL2\^
 -IC:\vclib\SDL2_image\i686-w64-mingw32\include\SDL2\^
 -IC:\vclib\SDL2_ttf\i686-w64-mingw32\include\SDL2\^
 -IC:\vclib\SDL2_mixer\i686-w64-mingw32\include\SDL2\^
 -LC:\vclib\SDL2\i686-w64-mingw32\lib\^
 -LC:\vclib\SDL2_image\i686-w64-mingw32\lib\^
 -LC:\vclib\SDL2_ttf\i686-w64-mingw32\lib\^
 -LC:\vclib\SDL2_mixer\i686-w64-mingw32\lib\^
 -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lSDL2_mixer
