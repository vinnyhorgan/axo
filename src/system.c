#include "axo.h"

#include <GLFW/glfw3.h>

static int axo_system_get_time(lua_State* L) {
  double time = glfwGetTime();
  lua_pushnumber(L, time);
  return 1;
}

static const struct luaL_Reg axo_system_funcs[] = {
  { "get_time", axo_system_get_time },
  { NULL, NULL },
};

static int axo_system_deinit(lua_State* L) {
  (void)L;
  glfwTerminate();
  return 0;
}

static void register_system_deinit(lua_State* L) {
  void** ud = lua_newuserdata(L, sizeof(void*));
  *ud = NULL;

  luaL_newmetatable(L, "axo.system.__gc");
  lua_pushcfunction(L, axo_system_deinit);
  lua_setfield(L, -2, "__gc");
  lua_setmetatable(L, -2);
  lua_setfield(L, LUA_REGISTRYINDEX, "axo.system.deinit");
}

int luaopen_axo_system(lua_State* L) {
  glfwInitHint(GLFW_WIN32_MESSAGES_IN_FIBER, GLFW_TRUE);
  if (!glfwInit()) {
    return luaL_error(L, "failed to initialize system module");
  }

  register_system_deinit(L);
  luaL_newlib(L, axo_system_funcs);
  return 1;
}
