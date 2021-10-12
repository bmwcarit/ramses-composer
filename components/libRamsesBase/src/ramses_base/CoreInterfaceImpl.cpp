/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "EnumerationDescriptions.h"
#include "core/EngineInterface.h"
#include "ramses_base/BaseEngineBackend.h"
#include "ramses_base/Utils.h"
#include "user_types/Enumerations.h"

namespace raco::ramses_base {

CoreInterfaceImpl::CoreInterfaceImpl(BaseEngineBackend* backend) : backend_{backend} {}

bool CoreInterfaceImpl::parseShader(const std::string& vertexShader, const std::string& geometryShader, const std::string& fragmentShader, const std::string& shaderDefines, raco::core::PropertyInterfaceList& outUniforms, raco::core::PropertyInterfaceList& outAttributes, std::string& outError) {
	return raco::ramses_base::parseShaderText(backend_->internalScene(), vertexShader, geometryShader, fragmentShader, shaderDefines, outUniforms, outAttributes, outError);
}

bool CoreInterfaceImpl::parseLuaScript(const std::string& luaScript, raco::core::PropertyInterfaceList& outInputs, raco::core::PropertyInterfaceList& outOutputs, std::string& outError) {
	return raco::ramses_base::parseLuaScript(backend_->logicEngine(), luaScript, outInputs, outOutputs, outError);
}

const std::map<int, std::string>& CoreInterfaceImpl::enumerationDescription(raco::core::EngineEnumeration type) const {
	switch (type) {
		case raco::core::EngineEnumeration::CullMode:
			return enumerationCullMode;
		case raco::core::EngineEnumeration::BlendOperation:
			return enumerationBlendOperation;
		case raco::core::EngineEnumeration::BlendFactor:
			return enumerationBlendFactor;
		case raco::core::EngineEnumeration::DepthFunction:
			return enumerationDepthFunction;
		case raco::core::EngineEnumeration::TextureAddressMode:
			return enumerationTextureAddressMode;
		case raco::core::EngineEnumeration::TextureMinSamplingMethod:
			return enumerationTextureMinSamplingMethod;
		case raco::core::EngineEnumeration::TextureMagSamplingMethod:
			return enumerationTextureMagSamplingMethod;
		case raco::core::EngineEnumeration::TextureFormat:
			return enumerationTextureFormat;

		case raco::core::EngineEnumeration::TextureOrigin:
			return raco::user_types::enumerationTextureOrigin;

		case raco::core::EngineEnumeration::RenderBufferFormat:
			return enumerationRenderBufferFormat;

		case raco::core::EngineEnumeration::RenderLayerOrder:
			return raco::user_types::enumerationRenderLayerOrder;

		case raco::core::EngineEnumeration::RenderLayerMaterialFilterFlag:
			return raco::user_types::enumerationRenderLayerMaterialFilterFlag;

		default:
			assert(false);
			return enumerationEmpty;
	}
}

}  // namespace raco::ramses_base