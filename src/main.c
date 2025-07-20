#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "l_ftp.h"
#include "l_headers.h"
#include "l_http.h"
#include "l_ltn12.h"
#include "l_mbox.h"
#include "l_mime.h"
#include "l_re.h"
#include "l_smtp.h"
#include "l_socket.h"
#include "l_tp.h"
#include "l_url.h"

#include "axo.h"

#define VERSION "0.1.0"

#define PREAMBLE "axo - your tiny lua toolkit >(^.^)<\n\n"
#define HELP_STR                                                         \
  PREAMBLE                                                               \
  "usage:\n\n  .\\axo.exe --help\n  .\\axo.exe --version\n  .\\axo.exe " \
  "<script.lua> "                                                        \
  "[args...]\n"
#define VERSION_STR                                                                                            \
  PREAMBLE "version " VERSION " based on lua " LUA_VERSION_MAJOR "." LUA_VERSION_MINOR "." LUA_VERSION_RELEASE \
           ", made with <3 by vinny\n"

int luaopen_lfs(lua_State* L);
int luaopen_lpeg(lua_State* L);
int luaopen_re(lua_State* L);
int luaopen_socket_core(lua_State* L);
int luaopen_mime_core(lua_State* L);
int luaopen_socket(lua_State* L);
int luaopen_ftp(lua_State* L);
int luaopen_http(lua_State* L);
int luaopen_ltn12(lua_State* L);
int luaopen_mime(lua_State* L);
int luaopen_smtp(lua_State* L);
int luaopen_tp(lua_State* L);
int luaopen_url(lua_State* L);
int luaopen_headers(lua_State* L);
int luaopen_mbox(lua_State* L);

static lua_State* global_L = NULL;

static void stop(lua_State* L, lua_Debug* ar) {
  (void)ar;
  lua_sethook(L, NULL, 0, 0);
  luaL_error(L, "interrupted!");
}

static void action(int sig) {
  (void)sig;
  signal(SIGINT, SIG_DFL);
  lua_sethook(global_L, stop, LUA_MASKCALL | LUA_MASKRET | LUA_MASKLINE | LUA_MASKCOUNT, 1);
}

static int handler(lua_State* L) {
  const char* msg = lua_tostring(L, 1);
  if (msg == NULL) {
    if (luaL_callmeta(L, 1, "__tostring") && lua_type(L, -1) == LUA_TSTRING)
      return 1;
    msg = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
  }
  luaL_traceback(L, L, msg, 1);
  return 1;
}

static void luax_preload(lua_State* L, const char* name, lua_CFunction func) {
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "preload");
  lua_pushcfunction(L, func);
  lua_setfield(L, -2, name);
  lua_pop(L, 2);
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, HELP_STR);
    return EXIT_FAILURE;
  }

  if (strcmp(argv[1], "--help") == 0) {
    printf(HELP_STR);
    return EXIT_SUCCESS;
  } else if (strcmp(argv[1], "--version") == 0) {
    printf(VERSION_STR);
    return EXIT_SUCCESS;
  }

  lua_State* L = luaL_newstate();
  if (L == NULL) {
    fprintf(stderr, "cannot create lua state: not enough memory\n");
    return EXIT_FAILURE;
  }

  luaL_openlibs(L);
  luax_preload(L, "lfs", luaopen_lfs);
  luax_preload(L, "lpeg", luaopen_lpeg);
  luax_preload(L, "re", luaopen_re);
  luax_preload(L, "socket.core", luaopen_socket_core);
  luax_preload(L, "mime.core", luaopen_mime_core);
  luax_preload(L, "socket", luaopen_socket);
  luax_preload(L, "socket.ftp", luaopen_ftp);
  luax_preload(L, "socket.http", luaopen_http);
  luax_preload(L, "ltn12", luaopen_ltn12);
  luax_preload(L, "mime", luaopen_mime);
  luax_preload(L, "socket.smtp", luaopen_smtp);
  luax_preload(L, "socket.tp", luaopen_tp);
  luax_preload(L, "socket.url", luaopen_url);
  luax_preload(L, "socket.headers", luaopen_headers);
  luax_preload(L, "mbox", luaopen_mbox);

  luax_preload(L, "axo.window", luaopen_axo_window);

  lua_createtable(L, argc, 0);
  for (int i = 0; i < argc; i++) {
    lua_pushstring(L, argv[i]);
    lua_rawseti(L, -2, i - 1);
  }
  lua_setglobal(L, "arg");

  int status = luaL_loadfile(L, argv[1]);
  if (status == LUA_OK) {
    global_L = L;
    int base = lua_gettop(L);
    lua_pushcfunction(L, handler);
    lua_insert(L, base);
    signal(SIGINT, action);
    status = lua_pcall(L, 0, LUA_MULTRET, base);
    signal(SIGINT, SIG_DFL);
    lua_remove(L, base);
  }

  if (status != LUA_OK) {
    const char* msg = lua_tostring(L, -1);
    fprintf(stderr, "%s\n", msg ? msg : "(error object is not a string)");
    lua_pop(L, 1);
  }

  lua_close(L);
  return (status == LUA_OK) ? EXIT_SUCCESS : EXIT_FAILURE;
}

int luaopen_re(lua_State* L) {
  if (luaL_loadbuffer(L, (const char*)l_re, l_re_len, "re") == LUA_OK) {
    lua_call(L, 0, 1);
  }
  return 1;
}

int luaopen_socket(lua_State* L) {
  if (luaL_loadbuffer(L, (const char*)l_socket, l_socket_len, "socket") == LUA_OK) {
    lua_call(L, 0, 1);
  }
  return 1;
}

int luaopen_ftp(lua_State* L) {
  if (luaL_loadbuffer(L, (const char*)l_ftp, l_ftp_len, "ftp") == LUA_OK) {
    lua_call(L, 0, 1);
  }
  return 1;
}

int luaopen_http(lua_State* L) {
  if (luaL_loadbuffer(L, (const char*)l_http, l_http_len, "http") == LUA_OK) {
    lua_call(L, 0, 1);
  }
  return 1;
}

int luaopen_ltn12(lua_State* L) {
  if (luaL_loadbuffer(L, (const char*)l_ltn12, l_ltn12_len, "ltn12") == LUA_OK) {
    lua_call(L, 0, 1);
  }
  return 1;
}

int luaopen_mime(lua_State* L) {
  if (luaL_loadbuffer(L, (const char*)l_mime, l_mime_len, "mime") == LUA_OK) {
    lua_call(L, 0, 1);
  }
  return 1;
}

int luaopen_smtp(lua_State* L) {
  if (luaL_loadbuffer(L, (const char*)l_smtp, l_smtp_len, "smtp") == LUA_OK) {
    lua_call(L, 0, 1);
  }
  return 1;
}

int luaopen_tp(lua_State* L) {
  if (luaL_loadbuffer(L, (const char*)l_tp, l_tp_len, "tp") == LUA_OK) {
    lua_call(L, 0, 1);
  }
  return 1;
}

int luaopen_url(lua_State* L) {
  if (luaL_loadbuffer(L, (const char*)l_url, l_url_len, "url") == LUA_OK) {
    lua_call(L, 0, 1);
  }
  return 1;
}

int luaopen_headers(lua_State* L) {
  if (luaL_loadbuffer(L, (const char*)l_headers, l_headers_len, "headers") == LUA_OK) {
    lua_call(L, 0, 1);
  }
  return 1;
}

int luaopen_mbox(lua_State* L) {
  if (luaL_loadbuffer(L, (const char*)l_mbox, l_mbox_len, "mbox") == LUA_OK) {
    lua_call(L, 0, 1);
  }
  return 1;
}
