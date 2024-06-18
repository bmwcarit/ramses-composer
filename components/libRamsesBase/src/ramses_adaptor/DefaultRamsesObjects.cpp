/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ramses_adaptor/DefaultRamsesObjects.h"

#include "lodepng.h"
#include "ramses_adaptor/utilities.h"
#include "ramses_base/RamsesHandles.h"

#include "core/MeshCacheInterface.h"
#include "mesh_loader/glTFFileLoader.h"
#include "ramses_adaptor/CubeMapAdaptor.h"
#include "ramses_adaptor/TextureSamplerAdaptor.h"

namespace raco::ramses_adaptor {

const char* flatColorVertexShader =
	R"(
#version 300 es
precision mediump float;
in vec3 a_Position;
uniform mat4 u_MVPMatrix;
void main() {
	gl_Position = u_MVPMatrix * vec4(a_Position.xyz, 1.0);
}
)";

const char* flatColorFragmentShader =
	R"(
#version 300 es
precision mediump float;
out vec4 FragColor;
uniform vec3 u_color;
uniform float u_alpha;
void main() {
	FragColor = vec4(u_color, u_alpha); 
}
)";

static constexpr const char* phongVertexShader =
	R"(
#version 300 es
precision highp float;

in vec3 a_Position;
in vec3 a_Normal;

uniform mat4 u_MVMatrix;
uniform mat4 u_PMatrix;
uniform mat4 u_NMatrix;

out vec3 v_NormalWorldSpace;
out vec3 v_VertexWorldSpace;

void main()
{
    vec4 vertWS = u_MVMatrix * vec4(a_Position, 1.0);

    v_NormalWorldSpace = vec3(u_NMatrix * vec4(a_Normal, 0.0));
    v_VertexWorldSpace = vertWS.xyz / vertWS.w;
    gl_Position = u_PMatrix * vertWS;
}
)";

static constexpr const char* phongFragmentShader =
	R"(
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
uniform float u_alpha;

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

	// We pretend that both sides are the front side by flipping the normal if looking from the back:
	if (dot(viewDir, n) < 0.0) {
		n = -n;
	}

    vec3 luminance = u_ambientColor;
    float illuminance = dot(lightDir, n);
    if(illuminance > 0.0)
    {
        vec3 brdf = phongBRDF(lightDir, viewDir, n, u_diffuseColor, u_specularColor, u_shininess);
        luminance += brdf * illuminance * u_lightColor;
    }

    FragColor = vec4(luminance, u_alpha);
}
)";

static const glm::vec3 defaultColor(0.2, 0.4, 0.6);
static const glm::vec3 highlightColor(1.0, 0.5, 0.0);

ramses_base::RamsesAppearance createFlatColorAppearance(ramses::Scene* scene, std::string_view effectName, std::string_view appearanceName, bool highlight, bool transparent) {
	ramses::EffectDescription effectDescription{};
	effectDescription.setVertexShader(flatColorVertexShader);
	effectDescription.setFragmentShader(flatColorFragmentShader);
	effectDescription.setUniformSemantic("u_MVPMatrix", ramses::EEffectUniformSemantic::ModelViewProjectionMatrix);

	auto effect = ramses_base::ramsesEffect(scene, effectDescription, effectName, {0, 0});
	auto appearance = ramses_base::ramsesAppearance(scene, effect, {0, 0});
	(*appearance)->setName(appearanceName);
	(*appearance)->setCullingMode(ramses::ECullMode::Disabled);
	if (transparent) {
		(*appearance)->setDepthWrite(ramses::EDepthWrite::Disabled);
		(*appearance)->setBlendingOperations(ramses::EBlendOperation::Add, ramses::EBlendOperation::Add);
		(*appearance)->setBlendingFactors(ramses::EBlendFactor::SrcAlpha, ramses::EBlendFactor::OneMinusSrcAlpha, ramses::EBlendFactor::One, ramses::EBlendFactor::One);
	}

	ramsesSetUniform(**appearance, "u_color", highlight ? highlightColor : defaultColor);
	ramsesSetUniform(**appearance, "u_alpha", transparent ? 0.2f : 1.0f);

	return appearance;
}

ramses_base::RamsesAppearance createPhongAppearance(ramses::Scene* scene, std::string_view effectName, std::string_view appearanceName, bool highlight, bool transparent) {
	ramses::EffectDescription effectDescription{};
	effectDescription.setVertexShader(phongVertexShader);
	effectDescription.setFragmentShader(phongFragmentShader);
	effectDescription.setUniformSemantic("u_PMatrix", ramses::EEffectUniformSemantic::ProjectionMatrix);
	effectDescription.setUniformSemantic("u_MVMatrix", ramses::EEffectUniformSemantic::ModelViewMatrix);
	effectDescription.setUniformSemantic("u_NMatrix", ramses::EEffectUniformSemantic::NormalMatrix);

	auto effect = ramses_base::ramsesEffect(scene, effectDescription, effectName, {0, 0});
	auto appearance = ramses_base::ramsesAppearance(scene, effect, {0, 0});
	(*appearance)->setName(appearanceName);
	(*appearance)->setCullingMode(ramses::ECullMode::Disabled);
	if (transparent) {
		(*appearance)->setDepthWrite(ramses::EDepthWrite::Disabled);
		(*appearance)->setBlendingOperations(ramses::EBlendOperation::Add, ramses::EBlendOperation::Add);
		(*appearance)->setBlendingFactors(ramses::EBlendFactor::SrcAlpha, ramses::EBlendFactor::OneMinusSrcAlpha, ramses::EBlendFactor::One, ramses::EBlendFactor::One);
	}

	ramsesSetUniform(**appearance, "u_lightColor", glm::vec3(1.0, 1.0, 1.0));
	ramsesSetUniform(**appearance, "u_lightDirection", glm::vec3(1.0, -1.0, -2.0));
	ramsesSetUniform(**appearance, "u_ambientColor", glm::vec3(0.2, 0.2, 0.2));
	ramsesSetUniform(**appearance, "u_diffuseColor", highlight ? highlightColor : defaultColor);
	ramsesSetUniform(**appearance, "u_specularColor", glm::vec3(1.0, 1.0, 1.0));
	ramsesSetUniform(**appearance, "u_shininess", 10.0f);
	ramsesSetUniform(**appearance, "u_alpha", transparent ? 0.2f : 1.0f);

	return appearance;
}

ramses_base::RamsesAppearance createDefaultAppearance(ramses::Scene* scene, bool withNormals, bool highlight, bool transparent) {
	if (withNormals) {
		return createPhongAppearance(scene, defaultEffectWithNormalsName, defaultAppearanceWithNormalsName, highlight, transparent);
	}
	return createFlatColorAppearance(scene, defaultEffectName, defaultAppearanceName, highlight, transparent);
}

const std::vector<uint32_t> cubeIndicesData{
	0, 1, 2,
	2, 3, 0,
	4, 5, 6,
	6, 7, 4,
	8, 9, 10,
	10, 11, 8,
	12, 13, 14,
	14, 15, 12,
	16, 17, 18,
	18, 19, 16,
	20, 21, 22,
	22, 23, 20};

 ramses_base::RamsesArrayResource createCubeIndexDataBuffer(ramses::Scene* scene) {
	return ramses_base::ramsesArrayResource(scene, cubeIndicesData, defaultIndexDataBufferName);
 }

const std::vector<glm::vec3> cubeVerticesData{
	{1.0, 1.0, 1.0},
	{-1.0, 1.0, 1.0},
	{-1.0, -1.0, 1.0},
	{1.0, -1.0, 1.0},
	{1.0, -1.0, -1.0},
	{1.0, -1.0, 1.0},
	{-1.0, -1.0, 1.0},
	{-1.0, -1.0, -1.0},
	{-1.0, -1.0, -1.0},
	{-1.0, -1.0, 1.0},
	{-1.0, 1.0, 1.0},
	{-1.0, 1.0, -1.0},
	{-1.0, 1.0, -1.0},
	{1.0, 1.0, -1.0},
	{1.0, -1.0, -1.0},
	{-1.0, -1.0, -1.0},
	{1.0, 1.0, -1.0},
	{1.0, 1.0, 1.0},
	{1.0, -1.0, 1.0},
	{1.0, -1.0, -1.0},
	{-1.0, 1.0, -1.0},
	{-1.0, 1.0, 1.0},
	{1.0, 1.0, 1.0},
	{1.0, 1.0, -1.0}};

 ramses_base::RamsesArrayResource createCubeVertexDataBuffer(ramses::Scene* scene) {
	return ramses_base::ramsesArrayResource(scene, cubeVerticesData, defaultVertexDataBufferName);
 }

 ramses_base::RamsesArrayResource createCubeNormalDataBuffer(ramses::Scene* scene) {
	static std::vector<glm::vec3> normals{
		{0.0, -0.0, 1.0},
		{0.0, -0.0, 1.0},
		{0.0, -0.0, 1.0},
		{0.0, -0.0, 1.0},
		{0.0, -1.0, 0.0},
		{0.0, -1.0, 0.0},
		{0.0, -1.0, 0.0},
		{0.0, -1.0, 0.0},
		{-1.0, -0.0, 0.0},
		{-1.0, -0.0, 0.0},
		{-1.0, -0.0, 0.0},
		{-1.0, -0.0, 0.0},
		{0.0, 0.0, -1.0},
		{0.0, 0.0, -1.0},
		{0.0, 0.0, -1.0},
		{0.0, 0.0, -1.0},
		{1.0, -0.0, 0.0},
		{1.0, -0.0, 0.0},
		{1.0, -0.0, 0.0},
		{1.0, -0.0, 0.0},
		{0.0, 1.0, 0.0},
		{0.0, 1.0, 0.0},
		{0.0, 1.0, 0.0},
		{0.0, 1.0, 0.0}};
	return ramses_base::ramsesArrayResource(scene, normals, defaultNormalDataBufferName);
 }

RamsesGizmoMeshBuffers loadGizmoMesh(ramses::Scene* scene, core::MeshCache* meshCache, const std::string& filename) {
	auto path(core::PathManager::defaultResourceDirectory() / filename);
	mesh_loader::glTFFileLoader loader(path.string());

	auto mesh = loader.loadMesh({path.string(), 0, true});

	auto indexData = ramses_base::ramsesArrayResource(scene, mesh->getIndices(), defaultGizmoArrowName);

	auto posIndex = mesh->attribIndex(core::MeshData::ATTRIBUTE_POSITION);
	auto posData = ramses_adaptor::arrayResourceFromAttribute(scene, mesh, posIndex, defaultGizmoArrowName);

	const auto& triangleData = mesh->triangleBuffer();
	auto triangleBuffer = ramses_base::ramsesArrayBuffer(scene, ramses::EDataType::Vector3F, triangleData.size(), triangleData.data());

	return {indexData, posData, triangleBuffer};
}

static const std::vector<glm::vec3> quad_vertex_data = {
	{0.0f, 0.0f, 0.f},
	{0.0f, 1.0f, 0.f},
	{1.0f, 0.0f, 0.f},
	{1.0f, 1.0f, 0.f}};

ramses_base::RamsesArrayResource createQuadVertexDataBuffer(ramses::Scene* scene) {
	return ramses_base::ramsesArrayResource(scene, quad_vertex_data);
}

ramses_base::RamsesArrayResource createQuadUVDataBuffer(ramses::Scene* scene) {
	static const std::vector<glm::vec2> uv_data = {
		{0.0f, 0.0f},
		{0.0f, 1.0f},
		{1.0f, 0.0f},
		{1.0f, 1.0f}};
	return ramses_base::ramsesArrayResource(scene, uv_data);
}

static const std::vector<uint32_t> quad_index_data = {
	0, 2, 1,
	1, 2, 3};

ramses_base::RamsesArrayResource createQuadIndexDataBuffer(ramses::Scene* scene) {
	return ramses_base::ramsesArrayResource(scene, quad_index_data);
}

RamsesGizmoMeshBuffers createGizmoQuad(ramses::Scene* scene) {
	auto vertexData = createQuadVertexDataBuffer(scene);
	auto indexData = createQuadIndexDataBuffer(scene);

	auto triangleData = core::MeshData::buildTriangleBuffer(quad_vertex_data.data(), quad_index_data);
	auto triangleBuffer = ramses_base::ramsesArrayBuffer(scene, ramses::EDataType::Vector3F, triangleData.size(), triangleData.data());

	return {indexData, vertexData, triangleBuffer};
}

RamsesGizmoMeshBuffers createGizmoCube(ramses::Scene* scene) {
	auto vertexData = createCubeVertexDataBuffer(scene);
	auto indexData = createCubeIndexDataBuffer(scene);

	auto triangleData = core::MeshData::buildTriangleBuffer(quad_vertex_data.data(), quad_index_data);
	auto triangleBuffer = ramses_base::ramsesArrayBuffer(scene, ramses::EDataType::Vector3F, triangleData.size(), triangleData.data());

	return {indexData, vertexData, triangleBuffer};
}

const std::vector<glm::vec3> cat_vertex_data = {
	{0.000000, -0.687576, -0.782419},
	{0.262228, -0.752914, -0.443125},
	{0.000000, -0.844740, -0.483882},
	{-0.262228, -0.752914, -0.443125},
	{-0.891059, 1.155259, -0.400431},
	{-0.272170, 0.741501, -0.358234},
	{-0.428776, 0.584615, -0.231790},
	{-0.891059, 1.155259, -0.400431},
	{-0.428776, 0.584615, -0.231790},
	{-0.832509, 0.182630, -0.231565},
	{-0.832509, 0.182630, -0.231565},
	{-0.428776, 0.584615, -0.231790},
	{-0.268856, 0.080386, -0.653151},
	{-0.201833, -0.138557, -0.694994},
	{-0.098408, -0.476410, -0.870568},
	{-0.457082, -0.094252, -0.520104},
	{-0.268856, 0.080386, -0.653151},
	{0.000000, 0.606050, -0.418116},
	{0.268856, 0.080386, -0.653151},
	{0.000000, -0.844740, -0.483882},
	{0.000000, -0.844740, -0.055026},
	{-0.264625, -0.752914, -0.002985},
	{-0.262228, -0.752914, -0.443125},
	{-0.832509, 0.182630, -0.231565},
	{-0.262228, -0.752914, -0.443125},
	{-0.733418, -0.312989, -0.156211},
	{0.264625, -0.752914, -0.002985},
	{0.000000, -0.844740, -0.055026},
	{0.000000, -0.844740, -0.483882},
	{0.262228, -0.752914, -0.443125},
	{-0.891059, 1.155259, -0.400431},
	{-0.832509, 0.182630, -0.231565},
	{-0.622732, 0.479086, 0.232224},
	{-0.272170, 0.741501, -0.358234},
	{-0.891059, 1.155259, -0.400431},
	{-0.344476, 0.629688, 0.116868},
	{-0.891059, 1.155259, -0.400431},
	{-0.622732, 0.479086, 0.232224},
	{-0.344476, 0.629688, 0.116868},
	{0.000000, 0.740627, 0.146322},
	{-0.272170, 0.741501, -0.358234},
	{-0.344476, 0.629688, 0.116868},
	{-0.832509, 0.182630, -0.231565},
	{-0.733418, -0.312989, -0.156211},
	{-0.694135, -0.312989, 0.244487},
	{-0.656420, 0.181038, 0.258291},
	{0.000000, 0.740627, 0.146322},
	{-0.344476, 0.629688, 0.116868},
	{-0.349261, 0.536033, 0.476164},
	{0.000000, 0.596703, 0.573425},
	{0.000000, 0.596703, 0.573425},
	{-0.349261, 0.536033, 0.476164},
	{-0.355188, 0.198678, 0.691670},
	{0.000000, 0.217028, 0.796752},
	{-0.733418, -0.312989, -0.156211},
	{-0.262228, -0.752914, -0.443125},
	{-0.264625, -0.752914, -0.002985},
	{-0.694135, -0.312989, 0.244487},
	{0.000000, 0.217028, 0.796752},
	{-0.355188, 0.198678, 0.691670},
	{-0.311519, -0.283536, 0.677898},
	{0.000000, -0.265114, 0.797826},
	{0.000000, -0.696637, 0.511078},
	{-0.253946, -0.641370, 0.469708},
	{-0.264625, -0.752914, -0.002985},
	{0.000000, -0.844740, -0.055026},
	{0.000000, -0.265114, 0.797826},
	{-0.311519, -0.283536, 0.677898},
	{-0.253946, -0.641370, 0.469708},
	{0.000000, -0.696637, 0.511078},
	{-0.253946, -0.641370, 0.469708},
	{-0.311519, -0.283536, 0.677898},
	{-0.694135, -0.312989, 0.244487},
	{-0.694135, -0.312989, 0.244487},
	{-0.311519, -0.283536, 0.677898},
	{-0.355188, 0.198678, 0.691670},
	{-0.656420, 0.181038, 0.258291},
	{-0.622732, 0.479086, 0.232224},
	{-0.355188, 0.198678, 0.691670},
	{-0.349261, 0.536033, 0.476164},
	{-0.622732, 0.479086, 0.232224},
	{-0.832509, 0.182630, -0.231565},
	{-0.656420, 0.181038, 0.258291},
	{-0.098408, -0.476410, -0.870568},
	{0.000000, -0.593829, -0.829373},
	{0.000000, -0.687576, -0.782419},
	{-0.262228, -0.752914, -0.443125},
	{0.098408, -0.476410, -0.870568},
	{0.000000, -0.593829, -0.829373},
	{-0.098408, -0.476410, -0.870568},
	{-0.264625, -0.752914, -0.002985},
	{-0.253946, -0.641370, 0.469708},
	{-0.694135, -0.312989, 0.244487},
	{-0.344476, 0.629688, 0.116868},
	{-0.622732, 0.479086, 0.232224},
	{-0.349261, 0.536033, 0.476164},
	{-0.355188, 0.198678, 0.691670},
	{-0.622732, 0.479086, 0.232224},
	{-0.656420, 0.181038, 0.258291},
	{0.272170, 0.741501, -0.358234},
	{-0.272170, 0.741501, -0.358234},
	{0.000000, 0.740627, 0.146322},
	{-0.832509, 0.182630, -0.231565},
	{-0.268856, 0.080386, -0.653151},
	{-0.201833, -0.138557, -0.694994},
	{-0.457082, -0.094252, -0.520104},
	{-0.428776, 0.584615, -0.231790},
	{-0.272170, 0.741501, -0.358234},
	{0.000000, 0.606050, -0.418116},
	{-0.268856, 0.080386, -0.653151},
	{-0.268856, 0.080386, -0.653151},
	{0.268856, 0.080386, -0.653151},
	{0.201833, -0.138557, -0.694994},
	{-0.201833, -0.138557, -0.694994},
	{0.098408, -0.476410, -0.870568},
	{-0.098408, -0.476410, -0.870568},
	{-0.201833, -0.138557, -0.694994},
	{0.201833, -0.138557, -0.694994},
	{-0.457082, -0.094252, -0.520104},
	{-0.098408, -0.476410, -0.870568},
	{-0.262228, -0.752914, -0.443125},
	{-0.832509, 0.182630, -0.231565},
	{0.891059, 1.155259, -0.400431},
	{0.428776, 0.584615, -0.231790},
	{0.272170, 0.741501, -0.358234},
	{0.891059, 1.155259, -0.400431},
	{0.832509, 0.182630, -0.231565},
	{0.428776, 0.584615, -0.231790},
	{0.832509, 0.182630, -0.231565},
	{0.268856, 0.080386, -0.653151},
	{0.428776, 0.584615, -0.231790},
	{0.201833, -0.138557, -0.694994},
	{0.457082, -0.094252, -0.520104},
	{0.098408, -0.476410, -0.870568},
	{0.832509, 0.182630, -0.231565},
	{0.733418, -0.312989, -0.156211},
	{0.262228, -0.752914, -0.443125},
	{0.891059, 1.155259, -0.400431},
	{0.622732, 0.479086, 0.232224},
	{0.832509, 0.182630, -0.231565},
	{0.272170, 0.741501, -0.358234},
	{0.344476, 0.629688, 0.116868},
	{0.891059, 1.155259, -0.400431},
	{0.891059, 1.155259, -0.400431},
	{0.344476, 0.629688, 0.116868},
	{0.622732, 0.479086, 0.232224},
	{0.000000, 0.740627, 0.146322},
	{0.344476, 0.629688, 0.116868},
	{0.272170, 0.741501, -0.358234},
	{0.832509, 0.182630, -0.231565},
	{0.656420, 0.181038, 0.258291},
	{0.694135, -0.312989, 0.244487},
	{0.733418, -0.312989, -0.156211},
	{0.000000, 0.740627, 0.146322},
	{0.000000, 0.596703, 0.573425},
	{0.349261, 0.536033, 0.476164},
	{0.344476, 0.629688, 0.116868},
	{0.000000, 0.596703, 0.573425},
	{0.000000, 0.217028, 0.796752},
	{0.355188, 0.198678, 0.691670},
	{0.349261, 0.536033, 0.476164},
	{0.733418, -0.312989, -0.156211},
	{0.694135, -0.312989, 0.244487},
	{0.264625, -0.752914, -0.002985},
	{0.262228, -0.752914, -0.443125},
	{0.000000, 0.217028, 0.796752},
	{0.000000, -0.265114, 0.797826},
	{0.311519, -0.283536, 0.677898},
	{0.355188, 0.198678, 0.691670},
	{0.000000, -0.696637, 0.511078},
	{0.000000, -0.844740, -0.055026},
	{0.264625, -0.752914, -0.002985},
	{0.253946, -0.641370, 0.469708},
	{0.000000, -0.265114, 0.797826},
	{0.000000, -0.696637, 0.511078},
	{0.253946, -0.641370, 0.469708},
	{0.311519, -0.283536, 0.677898},
	{0.253946, -0.641370, 0.469708},
	{0.694135, -0.312989, 0.244487},
	{0.311519, -0.283536, 0.677898},
	{0.694135, -0.312989, 0.244487},
	{0.656420, 0.181038, 0.258291},
	{0.355188, 0.198678, 0.691670},
	{0.311519, -0.283536, 0.677898},
	{0.622732, 0.479086, 0.232224},
	{0.349261, 0.536033, 0.476164},
	{0.355188, 0.198678, 0.691670},
	{0.622732, 0.479086, 0.232224},
	{0.656420, 0.181038, 0.258291},
	{0.832509, 0.182630, -0.231565},
	{0.098408, -0.476410, -0.870568},
	{0.262228, -0.752914, -0.443125},
	{0.000000, -0.687576, -0.782419},
	{0.000000, -0.593829, -0.829373},
	{0.264625, -0.752914, -0.002985},
	{0.694135, -0.312989, 0.244487},
	{0.253946, -0.641370, 0.469708},
	{0.344476, 0.629688, 0.116868},
	{0.349261, 0.536033, 0.476164},
	{0.622732, 0.479086, 0.232224},
	{0.355188, 0.198678, 0.691670},
	{0.656420, 0.181038, 0.258291},
	{0.622732, 0.479086, 0.232224},
	{0.832509, 0.182630, -0.231565},
	{0.457082, -0.094252, -0.520104},
	{0.201833, -0.138557, -0.694994},
	{0.268856, 0.080386, -0.653151},
	{0.428776, 0.584615, -0.231790},
	{0.268856, 0.080386, -0.653151},
	{0.000000, 0.606050, -0.418116},
	{0.272170, 0.741501, -0.358234},
	{0.457082, -0.094252, -0.520104},
	{0.832509, 0.182630, -0.231565},
	{0.262228, -0.752914, -0.443125},
	{0.098408, -0.476410, -0.870568},
	{0.272170, 0.741501, -0.358234},
	{0.000000, 0.606050, -0.418116},
	{-0.272170, 0.741501, -0.358234}};

static const std::vector<glm::vec3> cat_normal_data = {
	{0.000000, -0.884870, -0.465839},
	{0.000000, -0.884870, -0.465839},
	{0.000000, -0.884870, -0.465839},
	{0.000000, -0.884870, -0.465839},
	{-0.242538, -0.450412, -0.859246},
	{-0.242538, -0.450412, -0.859246},
	{-0.242538, -0.450412, -0.859246},
	{0.158401, -0.159636, -0.974385},
	{0.158401, -0.159636, -0.974385},
	{0.158401, -0.159636, -0.974385},
	{-0.471173, 0.472804, -0.744615},
	{-0.471173, 0.472804, -0.744615},
	{-0.471173, 0.472804, -0.744615},
	{-0.512715, 0.267081, -0.815960},
	{-0.512715, 0.267081, -0.815960},
	{-0.512715, 0.267081, -0.815960},
	{0.000000, 0.408177, -0.912903},
	{0.000000, 0.408177, -0.912903},
	{0.000000, 0.408177, -0.912903},
	{-0.329301, -0.944225, -0.000909},
	{-0.329301, -0.944225, -0.000909},
	{-0.329301, -0.944225, -0.000909},
	{-0.329301, -0.944225, -0.000909},
	{-0.658484, -0.240097, -0.713269},
	{-0.658484, -0.240097, -0.713269},
	{-0.658484, -0.240097, -0.713269},
	{0.329301, -0.944225, -0.000909},
	{0.329301, -0.944225, -0.000909},
	{0.329301, -0.944225, -0.000909},
	{0.329301, -0.944225, -0.000909},
	{-0.914615, 0.015092, 0.404043},
	{-0.914615, 0.015092, 0.404043},
	{-0.914615, 0.015092, 0.404043},
	{0.522378, 0.808883, 0.269868},
	{0.522378, 0.808883, 0.269868},
	{0.522378, 0.808883, 0.269868},
	{-0.055946, 0.670220, 0.740050},
	{-0.055946, 0.670220, 0.740050},
	{-0.055946, 0.670220, 0.740050},
	{-0.315264, 0.933346, 0.171678},
	{-0.315264, 0.933346, 0.171678},
	{-0.315264, 0.933346, 0.171678},
	{-0.970986, -0.045619, 0.234743},
	{-0.970986, -0.045619, 0.234743},
	{-0.970986, -0.045619, 0.234743},
	{-0.970986, -0.045619, 0.234743},
	{-0.278035, 0.919991, 0.276248},
	{-0.278035, 0.919991, 0.276248},
	{-0.278035, 0.919991, 0.276248},
	{-0.278035, 0.919991, 0.276248},
	{-0.290359, 0.501267, 0.815121},
	{-0.290359, 0.501267, 0.815121},
	{-0.290359, 0.501267, 0.815121},
	{-0.290359, 0.501267, 0.815121},
	{-0.689222, -0.723919, 0.030235},
	{-0.689222, -0.723919, 0.030235},
	{-0.689222, -0.723919, 0.030235},
	{-0.689222, -0.723919, 0.030235},
	{-0.318327, -0.026892, 0.947600},
	{-0.318327, -0.026892, 0.947600},
	{-0.318327, -0.026892, 0.947600},
	{-0.318327, -0.026892, 0.947600},
	{-0.260625, -0.935992, 0.236630},
	{-0.260625, -0.935992, 0.236630},
	{-0.260625, -0.935992, 0.236630},
	{-0.260625, -0.935992, 0.236630},
	{-0.264809, -0.526051, 0.808175},
	{-0.264809, -0.526051, 0.808175},
	{-0.264809, -0.526051, 0.808175},
	{-0.264809, -0.526051, 0.808175},
	{-0.651072, -0.457234, 0.605840},
	{-0.651072, -0.457234, 0.605840},
	{-0.651072, -0.457234, 0.605840},
	{-0.784301, -0.022297, 0.619980},
	{-0.784301, -0.022297, 0.619980},
	{-0.784301, -0.022297, 0.619980},
	{-0.784301, -0.022297, 0.619980},
	{-0.651985, 0.416291, 0.633733},
	{-0.651985, 0.416291, 0.633733},
	{-0.651985, 0.416291, 0.633733},
	{-0.932326, 0.134728, 0.335582},
	{-0.932326, 0.134728, 0.335582},
	{-0.932326, 0.134728, 0.335582},
	{-0.584697, -0.529169, -0.614906},
	{-0.584697, -0.529169, -0.614906},
	{-0.584697, -0.529169, -0.614906},
	{-0.584697, -0.529169, -0.614906},
	{0.000000, -0.331050, -0.943613},
	{0.000000, -0.331050, -0.943613},
	{0.000000, -0.331050, -0.943613},
	{-0.647437, -0.738348, 0.188860},
	{-0.647437, -0.738348, 0.188860},
	{-0.647437, -0.738348, 0.188860},
	{-0.388866, 0.892756, 0.227530},
	{-0.388866, 0.892756, 0.227530},
	{-0.388866, 0.892756, 0.227530},
	{-0.815578, 0.141259, 0.561140},
	{-0.815578, 0.141259, 0.561140},
	{-0.815578, 0.141259, 0.561140},
	{-0.000000, 0.999999, 0.001732},
	{-0.000000, 0.999999, 0.001732},
	{-0.000000, 0.999999, 0.001732},
	{-0.586843, 0.015755, -0.809547},
	{-0.586843, 0.015755, -0.809547},
	{-0.586843, 0.015755, -0.809547},
	{-0.586843, 0.015755, -0.809547},
	{-0.385249, 0.374337, -0.843478},
	{-0.385249, 0.374337, -0.843478},
	{-0.385249, 0.374337, -0.843478},
	{-0.385249, 0.374337, -0.843478},
	{0.000000, 0.187717, -0.982223},
	{0.000000, 0.187717, -0.982223},
	{0.000000, 0.187717, -0.982223},
	{0.000000, 0.187717, -0.982223},
	{0.000000, 0.461126, -0.887335},
	{0.000000, 0.461126, -0.887335},
	{0.000000, 0.461126, -0.887335},
	{0.000000, 0.461126, -0.887335},
	{-0.763767, -0.293159, -0.575081},
	{-0.763767, -0.293159, -0.575081},
	{-0.763767, -0.293159, -0.575081},
	{-0.763767, -0.293159, -0.575081},
	{0.242538, -0.450412, -0.859246},
	{0.242538, -0.450412, -0.859246},
	{0.242538, -0.450412, -0.859246},
	{-0.158401, -0.159636, -0.974385},
	{-0.158401, -0.159636, -0.974385},
	{-0.158401, -0.159636, -0.974385},
	{0.471173, 0.472804, -0.744615},
	{0.471173, 0.472804, -0.744615},
	{0.471173, 0.472804, -0.744615},
	{0.512715, 0.267081, -0.815960},
	{0.512715, 0.267081, -0.815960},
	{0.512715, 0.267081, -0.815960},
	{0.658484, -0.240097, -0.713269},
	{0.658484, -0.240097, -0.713269},
	{0.658484, -0.240097, -0.713269},
	{0.914615, 0.015092, 0.404043},
	{0.914615, 0.015092, 0.404043},
	{0.914615, 0.015092, 0.404043},
	{-0.522378, 0.808883, 0.269868},
	{-0.522378, 0.808883, 0.269868},
	{-0.522378, 0.808883, 0.269868},
	{0.055946, 0.670220, 0.740050},
	{0.055946, 0.670220, 0.740050},
	{0.055946, 0.670220, 0.740050},
	{0.315264, 0.933346, 0.171678},
	{0.315264, 0.933346, 0.171678},
	{0.315264, 0.933346, 0.171678},
	{0.970986, -0.045619, 0.234743},
	{0.970986, -0.045619, 0.234743},
	{0.970986, -0.045619, 0.234743},
	{0.970986, -0.045619, 0.234743},
	{0.278035, 0.919991, 0.276248},
	{0.278035, 0.919991, 0.276248},
	{0.278035, 0.919991, 0.276248},
	{0.278035, 0.919991, 0.276248},
	{0.290359, 0.501267, 0.815121},
	{0.290359, 0.501267, 0.815121},
	{0.290359, 0.501267, 0.815121},
	{0.290359, 0.501267, 0.815121},
	{0.689222, -0.723919, 0.030235},
	{0.689222, -0.723919, 0.030235},
	{0.689222, -0.723919, 0.030235},
	{0.689222, -0.723919, 0.030235},
	{0.318327, -0.026892, 0.947600},
	{0.318327, -0.026892, 0.947600},
	{0.318327, -0.026892, 0.947600},
	{0.318327, -0.026892, 0.947600},
	{0.260625, -0.935992, 0.236630},
	{0.260625, -0.935992, 0.236630},
	{0.260625, -0.935992, 0.236630},
	{0.260625, -0.935992, 0.236630},
	{0.264809, -0.526051, 0.808175},
	{0.264809, -0.526051, 0.808175},
	{0.264809, -0.526051, 0.808175},
	{0.264809, -0.526051, 0.808175},
	{0.651072, -0.457234, 0.605840},
	{0.651072, -0.457234, 0.605840},
	{0.651072, -0.457234, 0.605840},
	{0.784301, -0.022297, 0.619980},
	{0.784301, -0.022297, 0.619980},
	{0.784301, -0.022297, 0.619980},
	{0.784301, -0.022297, 0.619980},
	{0.651985, 0.416291, 0.633733},
	{0.651985, 0.416291, 0.633733},
	{0.651985, 0.416291, 0.633733},
	{0.932326, 0.134728, 0.335582},
	{0.932326, 0.134728, 0.335582},
	{0.932326, 0.134728, 0.335582},
	{0.584697, -0.529169, -0.614906},
	{0.584697, -0.529169, -0.614906},
	{0.584697, -0.529169, -0.614906},
	{0.584697, -0.529169, -0.614906},
	{0.647437, -0.738348, 0.188860},
	{0.647437, -0.738348, 0.188860},
	{0.647437, -0.738348, 0.188860},
	{0.388866, 0.892756, 0.227530},
	{0.388866, 0.892756, 0.227530},
	{0.388866, 0.892756, 0.227530},
	{0.815578, 0.141259, 0.561140},
	{0.815578, 0.141259, 0.561140},
	{0.815578, 0.141259, 0.561140},
	{0.586843, 0.015755, -0.809547},
	{0.586843, 0.015755, -0.809547},
	{0.586843, 0.015755, -0.809547},
	{0.586843, 0.015755, -0.809547},
	{0.385249, 0.374337, -0.843478},
	{0.385249, 0.374337, -0.843478},
	{0.385249, 0.374337, -0.843478},
	{0.385249, 0.374337, -0.843478},
	{0.763767, -0.293159, -0.575081},
	{0.763767, -0.293159, -0.575081},
	{0.763767, -0.293159, -0.575081},
	{0.763767, -0.293159, -0.575081},
	{0.000000, 0.404344, -0.914607},
	{0.000000, 0.404344, -0.914607},
	{0.000000, 0.404344, -0.914607}};

const std::vector<uint32_t> cat_indices_data{
	0, 1, 2,
	2, 3, 0,
	4, 5, 6,
	7, 8, 9,
	10, 11, 12,
	13, 14, 15,
	16, 17, 18,
	19, 20, 21,
	21, 22, 19,
	23, 24, 25,
	26, 27, 28,
	28, 29, 26,
	30, 31, 32,
	33, 34, 35,
	36, 37, 38,
	39, 40, 41,
	42, 43, 44,
	44, 45, 42,
	46, 47, 48,
	48, 49, 46,
	50, 51, 52,
	52, 53, 50,
	54, 55, 56,
	56, 57, 54,
	58, 59, 60,
	60, 61, 58,
	62, 63, 64,
	64, 65, 62,
	66, 67, 68,
	68, 69, 66,
	70, 71, 72,
	73, 74, 75,
	75, 76, 73,
	77, 78, 79,
	80, 81, 82,
	83, 84, 85,
	85, 86, 83,
	87, 88, 89,
	90, 91, 92,
	93, 94, 95,
	96, 97, 98,
	99, 100, 101,
	102, 103, 104,
	104, 105, 102,
	106, 107, 108,
	108, 109, 106,
	110, 111, 112,
	112, 113, 110,
	114, 115, 116,
	116, 117, 114,
	118, 119, 120,
	120, 121, 118,
	122, 123, 124,
	125, 126, 127,
	128, 129, 130,
	131, 132, 133,
	134, 135, 136,
	137, 138, 139,
	140, 141, 142,
	143, 144, 145,
	146, 147, 148,
	149, 150, 151,
	151, 152, 149,
	153, 154, 155,
	155, 156, 153,
	157, 158, 159,
	159, 160, 157,
	161, 162, 163,
	163, 164, 161,
	165, 166, 167,
	167, 168, 165,
	169, 170, 171,
	171, 172, 169,
	173, 174, 175,
	175, 176, 173,
	177, 178, 179,
	180, 181, 182,
	182, 183, 180,
	184, 185, 186,
	187, 188, 189,
	190, 191, 192,
	192, 193, 190,
	194, 195, 196,
	197, 198, 199,
	200, 201, 202,
	203, 204, 205,
	205, 206, 203,
	207, 208, 209,
	209, 210, 207,
	211, 212, 213,
	213, 214, 211,
	215, 216, 217};

ramses_base::RamsesArrayResource createCatVertexDataBuffer(ramses::Scene* scene) {
	std::vector<glm::vec3> data(cat_vertex_data);
	for (auto& v : data) {
		v.z = -v.z;
	}
	return ramses_base::ramsesArrayResource(scene, data, defaultVertexDataBufferName);
}

ramses_base::RamsesArrayResource createCatNormalDataBuffer(ramses::Scene* scene) {
	std::vector<glm::vec3> data(cat_normal_data);
	for (auto& v : data) {
		v.z = -v.z;
	}
	return ramses_base::ramsesArrayResource(scene, data, defaultNormalDataBufferName);
}

ramses_base::RamsesArrayResource createCatIndexDataBuffer(ramses::Scene* scene) {
	return ramses_base::ramsesArrayResource(scene, cat_indices_data, defaultIndexDataBufferName);
}

std::vector<unsigned char> getFallbackTextureData(bool flipped, unsigned int &outWidth, unsigned int &outHeight) {
	std::vector<unsigned char> textureData;
	QFile file(":fallbackTextureOpenGL");
	if (file.exists()) {
		auto size = file.size();
		file.open(QIODevice::ReadOnly);
		QDataStream in(&file);
		std::vector<unsigned char> sBuffer(size);

		for (auto i = 0; i < size; ++i) {
			in >> sBuffer[i];
		}

		file.close();
		
		lodepng::decode(textureData, outWidth, outHeight, sBuffer);

		if (flipped) {
			TextureSamplerAdaptor::flipDecodedPicture(textureData, 4, outWidth, outHeight, 8);
		}
	}

	return textureData;
}

ramses_base::RamsesTextureSampler createDefaultTextureSampler(ramses::Scene* scene) {
	const auto texture = createDefaultTexture2D(false, scene);
	const auto sampler = ramses_base::ramsesTextureSampler(scene, ramses::ETextureAddressMode::Repeat, ramses::ETextureAddressMode::Repeat, ramses::ETextureSamplingMethod::Linear, ramses::ETextureSamplingMethod::Linear, texture, 1, defaultTextureSamplerName, {0, 0});
	return sampler;
}
ramses_base::RamsesTextureSampler createDefaultTextureCubeSampler(ramses::Scene* scene) {
	const auto textureCube = createDefaultTextureCube(scene, true);
	const auto textureSampler = ramses_base::ramsesTextureSampler(scene, ramses::ETextureAddressMode::Repeat, ramses::ETextureAddressMode::Repeat, ramses::ETextureSamplingMethod::Linear, ramses::ETextureSamplingMethod::Linear, textureCube, 1, defaultTextureCubeSamplerName, {0, 0});
	return textureSampler;
}

ramses_base::RamsesTexture2D createDefaultTexture2D(bool flipped, ramses::Scene* scene) {
	unsigned int width;
	unsigned int height;
	auto data = getFallbackTextureData(flipped, width, height);
	std::vector<ramses::MipLevelData> mipDatas;
	mipDatas.emplace_back(reinterpret_cast<std::byte*>(data.data()), reinterpret_cast<std::byte*>(data.data()) + data.size());
	ramses_base::RamsesTexture2D texture = ramses_base::ramsesTexture2D(scene, ramses::ETextureFormat::RGBA8, width, height, mipDatas, false, {}, defaultTexture2DName, {});
	return texture;
}

ramses_base::RamsesTextureCube createDefaultTextureCube(ramses::Scene* scene, bool generateMipChain) {
	std::map<std::string, std::vector<unsigned char>> data;

	unsigned int width;
	unsigned int height;
	const auto fallbackImage = getFallbackTextureData(false, width, height);

	data["uriRight"] = fallbackImage;
	data["uriLeft"] = fallbackImage;
	data["uriTop"] = fallbackImage;
	data["uriBottom"] = fallbackImage;
	data["uriFront"] = fallbackImage;
	data["uriBack"] = fallbackImage;

	std::vector<ramses::CubeMipLevelData> mipDatas;
	mipDatas.emplace_back(ramses::CubeMipLevelData{
		{reinterpret_cast<std::byte*>(data["uriRight"].data()), reinterpret_cast<std::byte*>(data["uriRight"].data()) + data["uriRight"].size()},
		{reinterpret_cast<std::byte*>(data["uriLeft"].data()), reinterpret_cast<std::byte*>(data["uriLeft"].data()) + data["uriLeft"].size()},
		{reinterpret_cast<std::byte*>(data["uriTop"].data()), reinterpret_cast<std::byte*>(data["uriTop"].data()) + data["uriTop"].size()},
		{reinterpret_cast<std::byte*>(data["uriBottom"].data()), reinterpret_cast<std::byte*>(data["uriBottom"].data()) + data["uriBottom"].size()},
		{reinterpret_cast<std::byte*>(data["uriFront"].data()), reinterpret_cast<std::byte*>(data["uriFront"].data()) + data["uriFront"].size()},
		{reinterpret_cast<std::byte*>(data["uriBack"].data()), reinterpret_cast<std::byte*>(data["uriBack"].data()) + data["uriBack"].size()}});

	return ramses_base::ramsesTextureCube(scene, ramses::ETextureFormat::RGBA8,width, mipDatas, generateMipChain, {}, defaultTextureCubeName, {});
}

}  // namespace raco::ramses_adaptor