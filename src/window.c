#include "axo.h"

#include <stdbool.h>

#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <dwmapi.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

static struct {
  GLFWwindow* window;
  bool window_shown;
} state;

static int axo_window_create(lua_State* L) {
  const char* title = luaL_checkstring(L, 1);
  int width = (int)luaL_checkinteger(L, 2);
  int height = (int)luaL_checkinteger(L, 3);

  int resizable;
  if (lua_isboolean(L, 4)) {
    resizable = lua_toboolean(L, 4);
  } else {
    resizable = 1;
  }

  glfwWindowHint(GLFW_RESIZABLE, resizable);
  glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
  glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);

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

  glfwMakeContextCurrent(state.window);
  glfwSwapInterval(1);

  state.window_shown = false;

  return 0;
}

static int axo_window_closed(lua_State* L) {
  int closed = glfwWindowShouldClose(state.window);
  lua_pushboolean(L, closed);
  return 1;
}

static int axo_window_present(lua_State* L) {
  (void)L;

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

static const luaL_Reg axo_window_funcs[] = {
  { "create", axo_window_create },
  { "closed", axo_window_closed },
  { "present", axo_window_present },
  { "destroy", axo_window_destroy },
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
  if (!glfwInit()) {
    return luaL_error(L, "failed to initialize window module");
  }

  register_window_close(L);
  luaL_newlib(L, axo_window_funcs);
  return 1;
}
