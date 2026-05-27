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

	// these parameters are for normalization
	vec2 squareCentroid;
	float squareMagnitude;

	vec2 mutatedCentroid;
	float mutatedMagnitude;
};

in vec2 uv;
in vec4 color;
out vec4 frag_color;

void main() {
	// normalize the square/input UVs
	vec4 normalizedUv = vec4((uv - squareCentroid) / squareMagnitude, 0.0, 1.0);

	// now apply the homography to the square UVs and do the perspective divide
	vec4 projectedUv = homography * normalizedUv;
	projectedUv.x /= projectedUv.w;
	projectedUv.y /= projectedUv.w;
	projectedUv.w = 1.0f;

	// now reverse the normalization/denormalize the mutated/output UVs
	projectedUv.xy = (projectedUv.xy * mutatedMagnitude) + mutatedCentroid;

	vec4 texCol = texture(sampler2D(tex, smp), projectedUv.xy);
	frag_color = mix(texCol, vec4(color.xyz, 1.0), color.a);
}
@end

@program mainShd vs fs