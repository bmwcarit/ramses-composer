/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "ramses_base/RamsesHandles.h"

namespace raco::core {
class MeshCache;
}

namespace raco::ramses_adaptor {

static constexpr const char* defaultEffectName = "ramses_adaptor::DefaultEffectWithoutNormals";
static constexpr const char* defaultEffectWithNormalsName = "ramses_adaptor::DefaultEffectWithNormals";
static constexpr const char* defaultAppearanceName = "ramses_adaptor::DefaultAppearanceWithoutNormals";
static constexpr const char* defaultAppearanceWithNormalsName = "ramses_adaptor::DefaultAppearanceWithNormals";
static constexpr const char* defaultIndexDataBufferName = "ramses_adaptor::DefaultIndexDataBuffer";
static constexpr const char* defaultVertexDataBufferName = "ramses_adaptor::DefaultVertexDataBuffer";
static constexpr const char* defaultNormalDataBufferName = "ramses_adaptor::DefaultNormalDataBuffer";
static constexpr const char* defaultRenderGroupName = "ramses_adaptor::DefaultRenderGroup";
static constexpr const char* defaultRenderPassName = "ramses_adaptor::DefaultRenderPass";
static constexpr const char* defaultGizmoArrowName = "ramses_adaptor::DefaultGizmoArrow";

ramses_base::RamsesAppearance createDefaultAppearance(ramses::Scene* scene, bool withNormals, bool highlight, bool transparent);

 ramses_base::RamsesArrayResource createCubeIndexDataBuffer(ramses::Scene* scene);
 ramses_base::RamsesArrayResource createCubeVertexDataBuffer(ramses::Scene* scene);
 ramses_base::RamsesArrayResource createCubeNormalDataBuffer(ramses::Scene* scene);

ramses_base::RamsesArrayResource createCatIndexDataBuffer(ramses::Scene* scene);
ramses_base::RamsesArrayResource createCatVertexDataBuffer(ramses::Scene* scene);
ramses_base::RamsesArrayResource createCatNormalDataBuffer(ramses::Scene* scene);

ramses_base::RamsesArrayResource createQuadVertexDataBuffer(ramses::Scene* scene);
ramses_base::RamsesArrayResource createQuadIndexDataBuffer(ramses::Scene* scene);

struct RamsesGizmoMeshBuffers {
	ramses_base::RamsesArrayResource indices;
	ramses_base::RamsesArrayResource vertices;
	ramses_base::RamsesArrayBuffer triangles;
};

RamsesGizmoMeshBuffers loadGizmoMesh(ramses::Scene* scene, core::MeshCache* meshCache, const std::string& filename); 
RamsesGizmoMeshBuffers createGizmoQuad(ramses::Scene* scene);
RamsesGizmoMeshBuffers createGizmoCube(ramses::Scene* scene);

extern const char* flatColorVertexShader;
extern const char* flatColorFragmentShader;

extern const std::vector<uint32_t> cubeIndicesData;
extern const std::vector<glm::vec3> cubeVerticesData;
extern const std::vector<glm::vec3> cat_vertex_data;
extern const std::vector<uint32_t> cat_indices_data;

}  // namespace raco::ramses_adaptor