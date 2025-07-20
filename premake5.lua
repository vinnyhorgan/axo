workspace "axo"
  configurations { "debug", "release" }
  architecture "x86_64"
  location "build"
  startproject "axo"
  language "c"
  staticruntime "on"
  flags { "multiprocessorcompile" }

  filter "configurations:debug"
    defines { "DEBUG" }
    symbols "on"

  filter "configurations:release"
    defines { "NDEBUG" }
    optimize "on"
    linktimeoptimization "on"

project "lua"
  kind "staticlib"

  targetdir "%{wks.location}/lua/bin/%{cfg.buildcfg}"
  objdir "%{wks.location}/lua/obj/%{cfg.buildcfg}"

  files { "vendor/lua/*.h", "vendor/lua/*.c" }

project "glfw"
  kind "staticlib"

  targetdir "%{wks.location}/glfw/bin/%{cfg.buildcfg}"
  objdir "%{wks.location}/glfw/obj/%{cfg.buildcfg}"

  files { "vendor/glfw/**.h", "vendor/glfw/**.c" }

  defines {
    "_CRT_SECURE_NO_WARNINGS",
    "_GLFW_WIN32",
  }

project "lfs"
  kind "staticlib"

  targetdir "%{wks.location}/lfs/bin/%{cfg.buildcfg}"
  objdir "%{wks.location}/lfs/obj/%{cfg.buildcfg}"

  files { "vendor/lfs/*.h", "vendor/lfs/*.c" }
  includedirs { "vendor/lua" }

project "axo"
  kind "consoleapp"
  cdialect "c17"
  warnings "extra"

  targetdir "%{wks.location}/bin/%{cfg.buildcfg}"
  objdir "%{wks.location}/obj/%{cfg.buildcfg}"

  files { "src/**.h", "src/**.c", "assets/axo.rc" }

  includedirs {
    "vendor/lua",
    "vendor/glfw/include",
    "vendor/glad",
    "vendor/lfs",
  }

  links { "lua", "glfw", "lfs" }

  filter "configurations:release"
    postbuildcommands {
      "{ECHO} compressing...",
      "upx --best --lzma %{cfg.buildtarget.abspath}",
    }
