@echo off
rem Rebuilds the shared ReXGlue runtime/codegen and copies them into the install tree. (The SDK's
rem install target also builds unit_tests, which fails on a missing xxhash include path.)
set "SDK=N:\Projects\Monopoly\rexglue-sdk"
call N:\Projects\HexicHD\tools\env.cmd cmake --build %SDK%\out\build\win-amd64 --config Release --target rexruntime rexglue rexgpu-xenos || exit /b 1
copy /y %SDK%\out\win-amd64\Release\rexruntime.dll %SDK%\out\install\win-amd64\bin\ >nul
copy /y %SDK%\out\win-amd64\Release\rexruntime.lib %SDK%\out\install\win-amd64\lib\ >nul
copy /y %SDK%\out\win-amd64\Release\rexglue.exe %SDK%\out\install\win-amd64\bin\ >nul
copy /y %SDK%\out\win-amd64\Release\rexgpu-xenos.dll %SDK%\out\install\win-amd64\bin\ >nul
