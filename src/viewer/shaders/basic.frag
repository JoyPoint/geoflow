#version 330 core
in vec3 ourColor;
in float texCoord;
flat in int colorMode;

out vec4 color;

uniform sampler1D u_sampler;

void main()
{
    if(colorMode==2 || colorMode==3)
        color = texture(u_sampler, texCoord);
    else
        color = vec4(ourColor, 1.0f);
    
}