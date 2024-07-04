#version 320 es
precision highp float;

in vec3 v_NormalWorldSpace;
in vec3 v_VertexWorldSpace;

// Phong properties
layout(std140, binding=10) uniform PhongParameters_t {
	vec3 lightColor;
	vec3 lightDirection;
	vec3 ambientColor;
	vec3 diffuseColor;
	vec3 specularColor;
	float shininess;
	float alpha;
} phong;

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
    vec3 lightDir = normalize(-phong.lightDirection);
    vec3 viewDir = normalize(-v_VertexWorldSpace);
    vec3 n = normalize(v_NormalWorldSpace);

    vec3 luminance = phong.ambientColor;
    float illuminance = dot(lightDir, n);
    if(illuminance > 0.0)
    {
        vec3 brdf = phongBRDF(lightDir, viewDir, n, phong.diffuseColor, phong.specularColor, phong.shininess);
        luminance += brdf * illuminance * phong.lightColor;
    }

    FragColor = vec4(luminance, phong.alpha);
}
