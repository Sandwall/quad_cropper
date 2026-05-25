@vs vs

layout(binding=0) uniform VertexParams {
	mat4 mvp;
};

in vec2 position;
in vec2 texcoord0;
in vec4 color0;

out vec2 uv;
out vec4 color;

void main() {
	gl_Position = mvp * vec4(position, 0.0, 1.0);
	uv = texcoord0;
	color = color0;
}

@end

@fs fs
layout(binding=0) uniform texture2D tex;
layout(binding=0) uniform sampler smp;

layout(binding=1) uniform FragmentParams {
	// frickin sokol-shdc doesn't have the mat3 type
	mat4 homography;
};

in vec4 uv;
in vec4 color;
out vec4 frag_color;

void main() {
	vec4 regularUv = homography * vec4(uv, 0.0, 1.0);
	regularUv.x /= regularUv.w;
	regularUv.y /= regularUv.w;

	vec4 texCol = texture(sampler2D(tex, smp), regularUv.xy);
	frag_color = mix(texCol, vec4(color.xyz, 1.0), color.a);
}
@end

@program mainShd vs fs