/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "ramses_adaptor/ObjectAdaptor.h"
#include "ramses_base/RamsesHandles.h"
#include "user_types/RenderLayer.h"
#include <ramses-client-api/RenderGroup.h>

namespace raco::ramses_adaptor {

class RenderLayerAdaptor : public TypedObjectAdaptor<user_types::RenderLayer, ramses_base::RamsesRenderGroupHandle> {
public:
	explicit RenderLayerAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::RenderLayer> editorObject);

	bool sync(core::Errors* errors) override;

private:
	void buildRenderGroup(core::Errors* errors);
	void buildRenderableOrder(core::Errors* errors, std::vector<SEditorObject>& objs, const std::string& tag, bool parentActive, const std::set<std::string>& materialFilterTags, bool materialFilterExclusive, int32_t& orderIndex, bool sceneGraphOrder);
	void addNestedLayers(core::Errors* errors, const std::vector<user_types::SRenderLayer>& layers, const std::string& tag, int32_t orderIndex, bool sceneGraphOrder);

	std::array<components::Subscription, 9> subscriptions_;
};

};	// namespace raco::ramses_adaptor
