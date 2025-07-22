#include "axo.h"

#include <stdbool.h>

#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <dwmapi.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#define CHECK_WINDOW(L)                             \
  do {                                              \
    if (state.window == NULL) {                     \
      return luaL_error((L), "window not created"); \
    }                                               \
  } while (0)

static struct {
  GLFWwindow* window;
  bool window_shown;

  int key_cb_ref;
  int mouse_cb_ref;
  int cursor_cb_ref;
} state;

// utils

static void key_cb(GLFWwindow* win, int key, int scancode, int action, int mods) {
  lua_State* L = glfwGetWindowUserPointer(win);
  if (state.key_cb_ref == LUA_NOREF) {
    return;
  }

  lua_rawgeti(L, LUA_REGISTRYINDEX, state.key_cb_ref);
  lua_pushinteger(L, key);
  lua_pushinteger(L, scancode);
  lua_pushinteger(L, action);
  lua_pushinteger(L, mods);

  if (lua_pcall(L, 4, 0, 0) != LUA_OK) {
    lua_pop(L, 1);
  }
}

static void mouse_cb(GLFWwindow* win, int button, int action, int mods) {
  lua_State* L = glfwGetWindowUserPointer(win);
  if (state.mouse_cb_ref == LUA_NOREF) {
    return;
  }

  lua_rawgeti(L, LUA_REGISTRYINDEX, state.mouse_cb_ref);
  lua_pushinteger(L, button);
  lua_pushinteger(L, action);
  lua_pushinteger(L, mods);

  if (lua_pcall(L, 3, 0, 0) != LUA_OK) {
    lua_pop(L, 1);
  }
}

static void cursor_cb(GLFWwindow* win, double xpos, double ypos) {
  lua_State* L = glfwGetWindowUserPointer(win);
  if (state.cursor_cb_ref == LUA_NOREF) {
    return;
  }

  lua_rawgeti(L, LUA_REGISTRYINDEX, state.cursor_cb_ref);
  lua_pushnumber(L, xpos);
  lua_pushnumber(L, ypos);

  if (lua_pcall(L, 2, 0, 0) != LUA_OK) {
    lua_pop(L, 1);
  }
}

// api

static int axo_window_create(lua_State* L) {
  if (state.window) {
    return luaL_error(L, "window already created");
  }

  const char* title = luaL_checkstring(L, 1);
  int width = (int)luaL_checkinteger(L, 2);
  int height = (int)luaL_checkinteger(L, 3);

  int resizable;
  if (lua_isboolean(L, 4)) {
    resizable = lua_toboolean(L, 4);
  } else {
    resizable = 1;
  }

  int samples = (int)luaL_optinteger(L, 5, 4);

  glfwWindowHint(GLFW_RESIZABLE, resizable);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
  glfwWindowHint(GLFW_SAMPLES, samples);

  glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  state.window = glfwCreateWindow(width, height, title, NULL, NULL);
  if (!state.window) {
    return luaL_error(L, "failed to create window");
  }

  HWND hwnd = glfwGetWin32Window(state.window);
  BOOL dark = TRUE;
  DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

  GLFWmonitor* monitor = glfwGetPrimaryMonitor();
  if (monitor) {
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    if (mode) {
      float scale_x, scale_y;
      glfwGetMonitorContentScale(monitor, &scale_x, &scale_y);

      int xpos = (mode->width - (int)(width * scale_x)) / 2;
      int ypos = (mode->height - (int)(height * scale_y)) / 2;
      glfwSetWindowPos(state.window, xpos, ypos);
    }
  }

  glfwSetWindowUserPointer(state.window, L);
  glfwMakeContextCurrent(state.window);
  glfwSwapInterval(1);

  // store pointer in lua registry
  lua_pushstring(L, "axo.window");
  lua_pushlightuserdata(L, state.window);
  lua_settable(L, LUA_REGISTRYINDEX);

  return 0;
}

static int axo_window_closed(lua_State* L) {
  CHECK_WINDOW(L);
  int closed = glfwWindowShouldClose(state.window);
  lua_pushboolean(L, closed);
  return 1;
}

static int axo_window_present(lua_State* L) {
  CHECK_WINDOW(L);
  glfwSwapBuffers(state.window);
  if (!state.window_shown) {
    glfwShowWindow(state.window);
    state.window_shown = true;
  }

  glfwPollEvents();
  return 0;
}

static int axo_window_destroy(lua_State* L) {
  (void)L;

  if (state.window) {
    glfwDestroyWindow(state.window);
    state.window = NULL;
  }

  glfwTerminate();
  return 0;
}

static int axo_window_get_size(lua_State* L) {
  CHECK_WINDOW(L);
  int width, height;
  glfwGetFramebufferSize(state.window, &width, &height);
  lua_pushinteger(L, width);
  lua_pushinteger(L, height);
  return 2;
}

static int axo_window_set_mouse_enabled(lua_State* L) {
  CHECK_WINDOW(L);
  bool enabled = lua_toboolean(L, 1);
  if (enabled) {
    glfwSetInputMode(state.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
  } else {
    glfwSetInputMode(state.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  }
  return 0;
}

static int axo_window_set_key_callback(lua_State* L) {
  CHECK_WINDOW(L);

  if (state.key_cb_ref != LUA_NOREF) {
    luaL_unref(L, LUA_REGISTRYINDEX, state.key_cb_ref);
    state.key_cb_ref = LUA_NOREF;
  }

  if (lua_isnoneornil(L, 1)) {
    return 0;
  }

  luaL_checktype(L, 1, LUA_TFUNCTION);
  lua_pushvalue(L, 1);
  state.key_cb_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  glfwSetKeyCallback(state.window, key_cb);
  return 0;
}

static int axo_window_set_mouse_callback(lua_State* L) {
  CHECK_WINDOW(L);

  if (state.mouse_cb_ref != LUA_NOREF) {
    luaL_unref(L, LUA_REGISTRYINDEX, state.mouse_cb_ref);
    state.mouse_cb_ref = LUA_NOREF;
  }

  if (lua_isnoneornil(L, 1)) {
    return 0;
  }

  luaL_checktype(L, 1, LUA_TFUNCTION);
  lua_pushvalue(L, 1);
  state.mouse_cb_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  glfwSetMouseButtonCallback(state.window, mouse_cb);
  return 0;
}

static int axo_window_set_cursor_callback(lua_State* L) {
  CHECK_WINDOW(L);

  if (state.cursor_cb_ref != LUA_NOREF) {
    luaL_unref(L, LUA_REGISTRYINDEX, state.cursor_cb_ref);
    state.cursor_cb_ref = LUA_NOREF;
  }

  if (lua_isnoneornil(L, 1)) {
    return 0;
  }

  luaL_checktype(L, 1, LUA_TFUNCTION);
  lua_pushvalue(L, 1);
  state.cursor_cb_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  glfwSetCursorPosCallback(state.window, cursor_cb);
  return 0;
}

static const luaL_Reg axo_window_funcs[] = {
  { "create", axo_window_create },
  { "closed", axo_window_closed },
  { "present", axo_window_present },
  { "destroy", axo_window_destroy },
  { "get_size", axo_window_get_size },
  { "set_mouse_enabled", axo_window_set_mouse_enabled },
  { "set_key_callback", axo_window_set_key_callback },
  { "set_mouse_callback", axo_window_set_mouse_callback },
  { "set_cursor_callback", axo_window_set_cursor_callback },
  { NULL, NULL },
};

static void register_window_close(lua_State* L) {
  void** ud = lua_newuserdata(L, sizeof(void*));
  *ud = NULL;

  luaL_newmetatable(L, "axo.window.__gc");
  lua_pushcfunction(L, axo_window_destroy);
  lua_setfield(L, -2, "__gc");
  lua_setmetatable(L, -2);
  lua_setfield(L, LUA_REGISTRYINDEX, "axo.window.close");
}

int luaopen_axo_window(lua_State* L) {
  glfwInitHint(GLFW_WIN32_MESSAGES_IN_FIBER, GLFW_TRUE);

  if (!glfwInit()) {
    return luaL_error(L, "failed to initialize window module");
  }

  state.window = NULL;
  state.window_shown = false;

  register_window_close(L);
  luaL_newlib(L, axo_window_funcs);
  return 1;
}
