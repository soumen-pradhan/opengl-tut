#version 330 core
out vec4 FragColor;

in vec3 vertColor;
in vec2 texCoord;

uniform sampler2D texture0;
uniform sampler2D texture1;

void main()
{
    vec4 c0 = texture(texture0, texCoord);
    vec4 c1 = texture(texture1, vec2(1.0 - texCoord.x, texCoord.y));

    FragColor = mix(c0, c1, 0.2);
}
