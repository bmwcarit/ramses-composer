#version 310 es
precision highp float;

uniform highp sampler2DMS textureSampler;
uniform highp int sampleCount;

in lowp vec2 v_TextureCoordinate;
out vec4 fragColor;

void main(void)
{
    vec4 color = vec4(0.0);

    for (int i = 0; i < sampleCount; i++)
        color += texelFetch(textureSampler, ivec2(v_TextureCoordinate * vec2(textureSize(textureSampler))), i);

    color /= float(sampleCount);
    fragColor = color;
}
