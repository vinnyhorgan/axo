precision mediump float;

uniform vec3 u_light_pos;
uniform sampler2D u_texture;
uniform bool u_use_lighting;

varying vec4 v_color;
varying vec2 v_texcoord;
varying vec3 v_normal;
varying vec3 v_frag_pos;

void main() {
  vec4 tex_color = texture2D(u_texture, v_texcoord);

  if (!u_use_lighting) {
    gl_FragColor = tex_color * v_color;
    return;
  }

  vec3 normal = normalize(v_normal);
  vec3 light_dir = normalize(u_light_pos - v_frag_pos);
  float diff = max(dot(normal, light_dir), 0.0);
  vec4 ambient = vec4(0.2, 0.2, 0.2, 1.0);
  vec4 lighting = ambient + diff;

  gl_FragColor = tex_color * v_color * lighting;
}
