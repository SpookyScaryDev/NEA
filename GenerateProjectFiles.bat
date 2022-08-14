@echo off

echo Building NEA project files.
echo.

@echo on
call premake5.exe vs2019
call premake5.exe export-compile-commands

copy compile_commands\debug.json compile_commands.json
@echo off

pause