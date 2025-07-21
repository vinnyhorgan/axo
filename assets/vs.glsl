uniform mat4 u_modelview;
uniform mat4 u_projection;

attribute vec3 a_position;
attribute vec4 a_color;
attribute vec3 a_normal;
attribute vec2 a_texcoord;

varying vec4 v_color;
varying vec2 v_texcoord;
varying vec3 v_normal;
varying vec3 v_frag_pos;

void main() {
  vec4 pos_view = u_modelview * vec4(a_position, 1.0);
  v_frag_pos = pos_view.xyz;
  v_normal = mat3(u_modelview) * a_normal;
  v_color = a_color;
  v_texcoord = a_texcoord;
  gl_Position = u_projection * pos_view;
}
