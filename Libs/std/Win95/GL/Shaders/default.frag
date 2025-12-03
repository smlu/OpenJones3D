#version 330 core

out vec4 FragColor;

in vec4 vColor;
in vec2 vTexCoord;

uniform sampler2D sTexture;
uniform vec4 vFogParams;
uniform vec3 vFogColor;

vec3 ApplyFog(vec3 color, float depth)
{
    if (vFogParams.w == 0.0f)
    {
        return color;
    }

    float fogFactor = (depth - vFogParams.x) * vFogParams.z;
    fogFactor = clamp(fogFactor, 0.0f, 1.0f);

    return mix(color, vFogColor, fogFactor);
}

void main()
{
    vec4 texColor = texture(sTexture, vTexCoord);
    FragColor = vec4(vColor * texColor);
    if (texColor.a <= 0.01f || vColor.a <= 0.01f) {
        discard;
    }
    FragColor.rgb = ApplyFog(FragColor.rgb, 1.0f / gl_FragCoord.w);

}