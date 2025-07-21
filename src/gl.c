#include "axo.h"

#include <gles2.h>

static int axo_gl_clear_color(lua_State* L) {
  float r = (float)luaL_checknumber(L, 1);
  float g = (float)luaL_checknumber(L, 2);
  float b = (float)luaL_checknumber(L, 3);
  float a = (float)luaL_optnumber(L, 4, 1.0f);

  glClearColor(r, g, b, a);
  return 0;
}

static int axo_gl_clear(lua_State* L) {
  (void)L;
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  return 0;
}

static const luaL_Reg axo_gl_funcs[] = {
  { "clear_color", axo_gl_clear_color },
  { "clear", axo_gl_clear },
  { NULL, NULL },
};

int luaopen_axo_gl(lua_State* L) {
  luaL_newlib(L, axo_gl_funcs);
  return 1;
}
