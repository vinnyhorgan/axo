#include "axo.h"

#define STBI_FAILURE_USERMSG
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

static int axo_image_load(lua_State* L) {
  const char* filename = luaL_checkstring(L, 1);

  int x, y;
  unsigned char* data = stbi_load(filename, &x, &y, NULL, 4);
  if (!data) {
    return luaL_error(L, "failed to load image: %s", stbi_failure_reason());
  }

  lua_pushlstring(L, (const char*)data, x * y * 4);
  lua_pushinteger(L, x);
  lua_pushinteger(L, y);
  stbi_image_free(data);
  return 3;
}

static const struct luaL_Reg axo_image_funcs[] = {
  { "load", axo_image_load },
  { NULL, NULL },
};

int luaopen_axo_image(lua_State* L) {
  luaL_newlib(L, axo_image_funcs);
  return 1;
}
