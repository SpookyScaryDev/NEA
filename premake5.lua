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

include "Prototype/premake5.lua"
