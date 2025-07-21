#include "axo.h"

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdbool.h>
#include <string.h>

#define GLAD_GLES2_IMPLEMENTATION
#include <gles2.h>

#include <GLFW/glfw3.h>

#include <linmath.h>

#define STACK_SIZE 32

#define CHECK_INIT(L)                                \
  do {                                               \
    if (!state.init) {                               \
      return luaL_error((L), "context not created"); \
    }                                                \
  } while (0)

const char* vertex_shader_src =
    "uniform mat4 u_mvp;\n"
    "attribute vec3 a_position;\n"
    "attribute vec4 a_color;\n"
    "varying vec4 v_color;\n"
    "void main() {\n"
    "  gl_Position = u_mvp * vec4(a_position, 1.0);\n"
    "  v_color = a_color;\n"
    "}\n";

const char* fragment_shader_src =
    "precision mediump float;\n"
    "varying vec4 v_color;\n"
    "void main() {\n"
    "  gl_FragColor = v_color;\n"
    "}\n";

typedef enum {
  MODELVIEW,
  PROJECTION,
} matrix_mode;

static struct {
  bool init;

  matrix_mode current_matrix_mode;

  mat4x4 modelview_stack[STACK_SIZE];
  int modelview_top;

  mat4x4 projection_stack[STACK_SIZE];
  int projection_top;

  bool drawing;
  vec3 vertices[1024];
  int vertex_count;

  vec4 current_color;
  vec4 colors[1024];

  GLuint shader;
  GLint attr_position;
  GLint attr_color;
  GLint uniform_mvp;
  GLuint vbo;
  GLuint color_vbo;
} state;

GLuint compile_shader(GLenum type, const char* source) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &source, NULL);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char log[512];
    glGetShaderInfoLog(shader, 512, NULL, log);
    printf("shader compilation failed: %s\n", log);
    return 0;
  }

  return shader;
}

GLuint create_program() {
  GLuint vs = compile_shader(GL_VERTEX_SHADER, vertex_shader_src);
  GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_src);
  GLuint program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);

  GLint success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    char log[512];
    glGetProgramInfoLog(program, 512, NULL, log);
    printf("program linking failed: %s\n", log);
    return 0;
  }

  glDeleteShader(vs);
  glDeleteShader(fs);
  return program;
}

static int axo_gl_init(lua_State* L) {
  if (state.init) {
    return luaL_error(L, "context already initialized");
  }

  lua_pushstring(L, "axo.window");
  lua_gettable(L, LUA_REGISTRYINDEX);

  if (!lua_islightuserdata(L, -1)) {
    return luaL_error(L, "window not created");
  }

  gladLoadGLES2(glfwGetProcAddress);

  state.shader = create_program();
  state.attr_position = glGetAttribLocation(state.shader, "a_position");
  state.attr_color = glGetAttribLocation(state.shader, "a_color");
  state.uniform_mvp = glGetUniformLocation(state.shader, "u_mvp");

  glGenBuffers(1, &state.vbo);
  glGenBuffers(1, &state.color_vbo);

  state.init = true;
  return 0;
}

static int axo_gl_matrix_mode(lua_State* L) {
  CHECK_INIT(L);
  const char* mode = luaL_checkstring(L, 1);
  if (strcmp(mode, "modelview") == 0) {
    state.current_matrix_mode = MODELVIEW;
  } else if (strcmp(mode, "projection") == 0) {
    state.current_matrix_mode = PROJECTION;
  } else {
    return luaL_error(L, "invalid matrix mode: %s", mode);
  }
  return 0;
}

static int axo_gl_push_matrix(lua_State* L) {
  CHECK_INIT(L);
  mat4x4* stack;
  int* top;
  if (state.current_matrix_mode == MODELVIEW) {
    stack = state.modelview_stack;
    top = &state.modelview_top;
  } else {
    stack = state.projection_stack;
    top = &state.projection_top;
  }

  if (*top >= STACK_SIZE - 1) {
    return luaL_error(L, "matrix stack overflow");
  }

  mat4x4_dup(stack[*top + 1], stack[*top]);
  (*top)++;
  return 0;
}

static int axo_gl_pop_matrix(lua_State* L) {
  CHECK_INIT(L);
  int* top = (state.current_matrix_mode == MODELVIEW) ? &state.modelview_top : &state.projection_top;
  if (*top <= 0) {
    return luaL_error(L, "matrix stack underflow");
  }
  (*top)--;
  return 0;
}

static int axo_gl_load_identity(lua_State* L) {
  CHECK_INIT(L);
  mat4x4* top;
  if (state.current_matrix_mode == MODELVIEW) {
    top = &state.modelview_stack[state.modelview_top];
  } else {
    top = &state.projection_stack[state.projection_top];
  }
  mat4x4_identity(*top);
  return 0;
}

static int axo_gl_perspective(lua_State* L) {
  CHECK_INIT(L);
  float fovy = (float)luaL_checknumber(L, 1);
  float aspect = (float)luaL_checknumber(L, 2);
  float znear = (float)luaL_checknumber(L, 3);
  float zfar = (float)luaL_checknumber(L, 4);

  mat4x4* top = &state.projection_stack[state.projection_top];
  mat4x4_perspective(*top, fovy * (float)(M_PI / 180.0), aspect, znear, zfar);

  return 0;
}

static int axo_gl_ortho(lua_State* L) {
  CHECK_INIT(L);
  float left = (float)luaL_checknumber(L, 1);
  float right = (float)luaL_checknumber(L, 2);
  float bottom = (float)luaL_checknumber(L, 3);
  float top = (float)luaL_checknumber(L, 4);
  float znear = (float)luaL_checknumber(L, 5);
  float zfar = (float)luaL_checknumber(L, 6);

  mat4x4* top_matrix = &state.projection_stack[state.projection_top];
  mat4x4_ortho(*top_matrix, left, right, bottom, top, znear, zfar);

  return 0;
}

static int axo_gl_translate(lua_State* L) {
  CHECK_INIT(L);
  float x = (float)luaL_checknumber(L, 1);
  float y = (float)luaL_checknumber(L, 2);
  float z = (float)luaL_optnumber(L, 3, 0.0f);

  mat4x4 trans;
  mat4x4_identity(trans);
  mat4x4_translate_in_place(trans, x, y, z);

  mat4x4* top = (state.current_matrix_mode == MODELVIEW) ? &state.modelview_stack[state.modelview_top]
                                                         : &state.projection_stack[state.projection_top];

  mat4x4_mul(*top, trans, *top);
  return 0;
}

static int axo_gl_begin(lua_State* L) {
  CHECK_INIT(L);
  if (state.drawing) {
    return luaL_error(L, "already drawing");
  }
  state.vertex_count = 0;
  state.drawing = true;
  return 0;
}

static int axo_gl_finish(lua_State* L) {
  CHECK_INIT(L);
  if (!state.drawing) {
    return luaL_error(L, "not currently drawing");
  }

  mat4x4 mvp;
  mat4x4_mul(mvp, state.projection_stack[state.projection_top], state.modelview_stack[state.modelview_top]);

  glUseProgram(state.shader);
  glUniformMatrix4fv(state.uniform_mvp, 1, GL_FALSE, (const GLfloat*)mvp);

  glBindBuffer(GL_ARRAY_BUFFER, state.vbo);
  glBufferData(GL_ARRAY_BUFFER, state.vertex_count * sizeof(vec3), state.vertices, GL_STATIC_DRAW);
  glEnableVertexAttribArray(state.attr_position);
  glVertexAttribPointer(state.attr_position, 3, GL_FLOAT, GL_FALSE, sizeof(vec3), (void*)0);

  glBindBuffer(GL_ARRAY_BUFFER, state.color_vbo);
  glBufferData(GL_ARRAY_BUFFER, state.vertex_count * sizeof(vec4), state.colors, GL_STATIC_DRAW);
  glEnableVertexAttribArray(state.attr_color);
  glVertexAttribPointer(state.attr_color, 4, GL_FLOAT, GL_FALSE, sizeof(vec4), (void*)0);

  glDrawArrays(GL_TRIANGLES, 0, state.vertex_count);

  glDisableVertexAttribArray(state.attr_position);
  glDisableVertexAttribArray(state.attr_color);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glUseProgram(0);

  state.drawing = false;
  state.vertex_count = 0;

  return 0;
}

static int axo_gl_vertex(lua_State* L) {
  CHECK_INIT(L);
  if (!state.drawing) {
    return luaL_error(L, "not currently drawing");
  }

  if (state.vertex_count >= 1024) {
    return luaL_error(L, "vertex buffer overflow");
  }

  float x = (float)luaL_checknumber(L, 1);
  float y = (float)luaL_checknumber(L, 2);
  float z = (float)luaL_optnumber(L, 3, 0.0f);

  state.vertices[state.vertex_count][0] = x;
  state.vertices[state.vertex_count][1] = y;
  state.vertices[state.vertex_count][2] = z;

  memcpy(state.colors[state.vertex_count], state.current_color, sizeof(vec4));
  state.vertex_count++;

  return 0;
}

static int axo_gl_color(lua_State* L) {
  CHECK_INIT(L);
  float r = (float)luaL_checknumber(L, 1);
  float g = (float)luaL_checknumber(L, 2);
  float b = (float)luaL_checknumber(L, 3);
  float a = (float)luaL_optnumber(L, 4, 1.0f);

  state.current_color[0] = r;
  state.current_color[1] = g;
  state.current_color[2] = b;
  state.current_color[3] = a;
  return 0;
}

static int axo_gl_clear_color(lua_State* L) {
  CHECK_INIT(L);
  float r = (float)luaL_checknumber(L, 1);
  float g = (float)luaL_checknumber(L, 2);
  float b = (float)luaL_checknumber(L, 3);
  float a = (float)luaL_optnumber(L, 4, 1.0f);

  glClearColor(r, g, b, a);
  return 0;
}

static int axo_gl_clear(lua_State* L) {
  CHECK_INIT(L);
  glClear(GL_COLOR_BUFFER_BIT);
  return 0;
}

static int axo_gl_viewport(lua_State* L) {
  CHECK_INIT(L);
  int x = (int)luaL_checkinteger(L, 1);
  int y = (int)luaL_checkinteger(L, 2);
  int width = (int)luaL_checkinteger(L, 3);
  int height = (int)luaL_checkinteger(L, 4);

  glViewport(x, y, width, height);
  return 0;
}

static const luaL_Reg axo_gl_funcs[] = {
  { "init", axo_gl_init },
  { "matrix_mode", axo_gl_matrix_mode },
  { "push_matrix", axo_gl_push_matrix },
  { "pop_matrix", axo_gl_pop_matrix },
  { "load_identity", axo_gl_load_identity },
  { "perspective", axo_gl_perspective },
  { "ortho", axo_gl_ortho },
  { "translate", axo_gl_translate },
  { "begin", axo_gl_begin },
  { "finish", axo_gl_finish },
  { "vertex", axo_gl_vertex },
  { "color", axo_gl_color },
  { "clear_color", axo_gl_clear_color },
  { "clear", axo_gl_clear },
  { "viewport", axo_gl_viewport },
  { NULL, NULL },
};

int luaopen_axo_gl(lua_State* L) {
  state.init = false;
  state.modelview_top = 0;
  state.projection_top = 0;
  mat4x4_identity(state.modelview_stack[0]);
  mat4x4_identity(state.projection_stack[0]);
  state.current_matrix_mode = MODELVIEW;
  state.drawing = false;

  luaL_newlib(L, axo_gl_funcs);
  return 1;
}
