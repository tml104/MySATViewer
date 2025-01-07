#version 420 core

layout (location = 0) out vec4 frag;

layout (binding = 0) uniform sampler2D accum;
layout (binding = 1) uniform sampler2D reveal;

const float EPSILON = 0.00001f;

bool isApproximatelyEqual(float a, float b)
{
    return abs(a - b) <= (abs(a) < abs(b) ? abs(b) : abs(a)) * EPSILON;
}

float max3(vec3 v) 
{
	return max(max(v.x, v.y), v.z);
}

void main()
{
    // fragment coordination
	ivec2 coords = ivec2(gl_FragCoord.xy);

    float revealage = texelFetch(reveal, coords, 0).r;

    // save the blending and color texture fetch cost if there is not a transparent fragment
	if (isApproximatelyEqual(revealage, 1.0f)) 
		discard;

    vec4 accumulation = texelFetch(accum, coords, 0);

    if (isinf(max3(abs(accumulation.rgb)))) 
		accumulation.rgb = vec3(accumulation.a);

    // prevent floating point precision bug
	vec3 average_color = accumulation.rgb / max(accumulation.a, EPSILON);

    // blend pixels
    frag = vec4(average_color, 1.0f - revealage);
}