projectdir = "./"
outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
        
project "Prototype"
    location       (projectdir)
    kind           "ConsoleApp"
    language       "C++"
    staticruntime  "On"

    targetdir  (projectdir .. "Binaries/" .. outputdir)
    objdir     (projectdir .. "Intermediate/" .. outputdir)

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

        "Source"
    }

    libdirs {
        "../Vendor/SDL2/lib/x64",
    }

    links {
        "SDL2",
        "SDL2main",
    }
 
    prebuildcommands {
        "{COPY} ../Vendor/SDL2/lib/x64/SDL2.dll %{cfg.targetdir}",
    }

    filter "system:windows"
        cppdialect "C++17"
        systemversion "latest"

        defines "WINDOWS"

    filter "configurations:Debug"
        defines "DEBUG"
        runtime "Debug"
        symbols "On"    

    filter "configurations:Development"
        defines "DEVELOPMENT"
        runtime "Release"
        optimize "On"    

    filter "configurations:Ship"
        defines "SHIP"
        runtime "Release"
        optimize "Speed"
