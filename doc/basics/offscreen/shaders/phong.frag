#version 300 es
precision highp float;

in vec3 v_NormalWorldSpace;
in vec3 v_VertexWorldSpace;

// Phong properties
uniform vec3 u_lightColor;
uniform vec3 u_lightDirection;
uniform vec3 u_ambientColor;
uniform vec3 u_diffuseColor;
uniform vec3 u_specularColor;
uniform float u_shininess;

vec3 phongBRDF(vec3 lightDir, vec3 viewDir, vec3 normal, vec3 diffuse, vec3 specular, float shininess) {
    vec3 color = diffuse;
    vec3 reflectDir = reflect(-lightDir, normal);
    float specDot = max(dot(reflectDir, viewDir), 0.0);
    color += pow(specDot, shininess) * specular;
    return color;
}

out vec4 FragColor;

void main()
{
    vec3 lightDir = normalize(-u_lightDirection);
    vec3 viewDir = normalize(-v_VertexWorldSpace);
    vec3 n = normalize(v_NormalWorldSpace);

    vec3 luminance = u_ambientColor;
    float illuminance = dot(lightDir, n);
    if(illuminance > 0.0)
    {
        vec3 brdf = phongBRDF(lightDir, viewDir, n, u_diffuseColor, u_specularColor, u_shininess);
        luminance += brdf * illuminance * u_lightColor;
    }

    FragColor = vec4(luminance, 1.0);
}
