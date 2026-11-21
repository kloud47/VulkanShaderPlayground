#version 450

const vec2 positions[3] = vec2[](
	vec2( 0.0, -0.5),   // corner 0: top
    vec2( 0.5,  0.5),   // corner 1: bottom right
    vec2(-0.5,  0.5)    // corner 2: bottom left
);

const vec3 colors[3] = vec3[](
    vec3(1.0, 0.0, 0.0),   // corner 0: red
    vec3(0.0, 1.0, 0.0),   // corner 1: green
    vec3(0.0, 0.0, 1.0)    // corner 2: blue
);

layout(location = 0) out vec3 vColor;

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    vColor      = colors[gl_VertexIndex];
}