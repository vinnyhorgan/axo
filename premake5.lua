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
  defines { "_CRT_SECURE_NO_WARNINGS" }
  disablewarnings { "4133" }

project "lpeg"
  kind "staticlib"

  targetdir "%{wks.location}/lpeg/bin/%{cfg.buildcfg}"
  objdir "%{wks.location}/lpeg/obj/%{cfg.buildcfg}"

  files { "vendor/lpeg/*.h", "vendor/lpeg/*.c" }
  includedirs { "vendor/lua" }

  disablewarnings { "4244", "4267" }

  prebuildcommands {
    "python ../scripts/embed.py ../vendor/lpeg/re.lua l_re.h",
  }

  filter "configurations:debug"
    undefines { "DEBUG" }

project "lsocket"
  kind "staticlib"

  targetdir "%{wks.location}/lsocket/bin/%{cfg.buildcfg}"
  objdir "%{wks.location}/lsocket/obj/%{cfg.buildcfg}"

  files { "vendor/lsocket/*.h", "vendor/lsocket/*.c" }

  removefiles {
    "vendor/lsocket/unix.h",
    "vendor/lsocket/unixdgram.h",
    "vendor/lsocket/unixstream.h",
    "vendor/lsocket/usocket.h",
    "vendor/lsocket/serial.c",
    "vendor/lsocket/unix.c",
    "vendor/lsocket/unixdgram.c",
    "vendor/lsocket/unixstream.c",
    "vendor/lsocket/usocket.c",
  }

  includedirs { "vendor/lua" }
  defines { "_CRT_SECURE_NO_WARNINGS", "_WINSOCK_DEPRECATED_NO_WARNINGS" }

  prebuildcommands {
    "python ../scripts/embed.py ../vendor/lsocket/socket.lua l_socket.h",
    "python ../scripts/embed.py ../vendor/lsocket/ftp.lua l_ftp.h",
    "python ../scripts/embed.py ../vendor/lsocket/http.lua l_http.h",
    "python ../scripts/embed.py ../vendor/lsocket/ltn12.lua l_ltn12.h",
    "python ../scripts/embed.py ../vendor/lsocket/mime.lua l_mime.h",
    "python ../scripts/embed.py ../vendor/lsocket/smtp.lua l_smtp.h",
    "python ../scripts/embed.py ../vendor/lsocket/tp.lua l_tp.h",
    "python ../scripts/embed.py ../vendor/lsocket/url.lua l_url.h",
    "python ../scripts/embed.py ../vendor/lsocket/headers.lua l_headers.h",
    "python ../scripts/embed.py ../vendor/lsocket/mbox.lua l_mbox.h",
  }

project "axo"
  kind "consoleapp"
  cdialect "c17"
  warnings "extra"

  targetdir "%{wks.location}/bin/%{cfg.buildcfg}"
  objdir "%{wks.location}/obj/%{cfg.buildcfg}"

  files { "src/**.h", "src/**.c", "assets/axo.rc" }

  includedirs {
    "build",
    "vendor/lua",
    "vendor/glfw/include",
    "vendor/include",
  }

  links { "lua", "glfw", "lfs", "lpeg", "lsocket", "ws2_32", "dwmapi" }

  disablewarnings { "4244" }

  prebuildcommands {
    "python ../scripts/embed.py ../assets/vs.glsl l_vs.h",
    "python ../scripts/embed.py ../assets/fs.glsl l_fs.h",
  }

  filter "configurations:release"
    postbuildcommands {
      "{ECHO} compressing...",
      "upx --best --lzma %{cfg.buildtarget.abspath}",
    }
