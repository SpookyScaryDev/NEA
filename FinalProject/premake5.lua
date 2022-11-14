projectdir = "./"
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

externalproject "nfd"
   location "../Vendor/NFD/build/vs2010"
   uuid "5D94880B-C99D-887C-5219-9F7CBE21947C"
   kind "StaticLib"
   language "C++"    
    
project "FinalProject"
    location       (projectdir)
    kind           "ConsoleApp"
    language       "C++"
    staticruntime  "Off"

    targetdir  (projectdir .. "Binaries/" .. outputdir)
    objdir     (projectdir .. "Intermediate/" .. outputdir)
    debugdir   (projectdir .. "Data/")

    files {
        projectdir .. "../Vendor/imgui/backends/imgui_impl_sdl.h",
        projectdir .. "../Vendor/imgui/backends/imgui_impl_sdl.cpp",

        projectdir .. "../Vendor/imgui/backends/imgui_impl_sdlrenderer.h",
        projectdir .. "../Vendor/imgui/backends/imgui_impl_sdlrenderer.cpp",

        projectdir .. "../Vendor/imgui/*.cpp",

        projectdir .. "Source/**.h",
        projectdir .. "Source/**.cpp",
    }

    includedirs {
        "../Vendor/SDL2/include",
        "../Vendor/imgui",
        "../Vendor/imgui/backends",
        "../Vendor/json/single_include",
        "../Vendor/NFD/src/include",

        "Source"
    }

    libdirs {
        "../Vendor/SDL2/lib/x64",
    }

    links {
        "SDL2",
        "SDL2main",
        "nfd"
    }
 
    prebuildcommands {
        "{COPY} ../Vendor/SDL2/lib/x64/SDL2.dll %{cfg.targetdir}",
        "{COPY} Data/*.* %{cfg.targetdir}",
    }

    filter "system:windows"
        cppdialect "C++17"
        systemversion "latest"

        defines "WINDOWS"

    filter "configurations:Debug"
        defines "DEBUG"
        runtime "Debug"
        symbols "On"    

    filter "configurations:Release"
        defines "RELEASE"
        runtime "Release"
        optimize "Speed"
