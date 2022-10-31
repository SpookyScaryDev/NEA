require "export-compile-commands"

workspace "NEA"
    location ""

    architecture "x64"
    startproject "Prototype"

    configurations {
        "Debug",
        "Development",
        "Ship"
    }

include "FinalProject/premake5.lua"
include "Prototype/premake5.lua"
include "UIMockup/premake5.lua"
