#version 410 core
in vec3 fragTexCoord;
out vec4 outColor;

uniform samplerCube skybox;

void main()
{
    outColor = texture(skybox, fragTexCoord);
}
