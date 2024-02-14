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

#include "ramses_adaptor/ObjectAdaptor.h"
#include "user_types/RenderTarget.h"
#include <ramses/client/RenderTarget.h>

namespace raco::ramses_adaptor {

class RenderBufferAdaptor;
class RenderBufferMSAdaptor;

template <typename RenderTargetClass, typename RenderBufferClass, typename RenderBufferAdaptorClass>
class RenderTargetAdaptorT : public TypedObjectAdaptor<RenderTargetClass, ramses::RenderTarget> {
public:
	explicit RenderTargetAdaptorT(SceneAdaptor* sceneAdaptor, std::shared_ptr<RenderTargetClass> editorObject);

	bool sync(core::Errors* errors) override;
	std::vector<ExportInformation> getExportInformation() const override;

private:
	bool collectBuffers(std::vector<ramses_base::RamsesRenderBuffer>& buffers, const std::vector < std::shared_ptr<RenderBufferClass>> & userTypeBuffers, ramses::RenderTargetDescription& rtDesc, core::Errors* errors);

	components::Subscription subscription_;
};

using RenderTargetAdaptor = RenderTargetAdaptorT<user_types::RenderTarget, user_types::RenderBuffer, RenderBufferAdaptor>;
using RenderTargetMSAdaptor = RenderTargetAdaptorT<user_types::RenderTargetMS, user_types::RenderBufferMS, RenderBufferMSAdaptor>;

};	// namespace raco::ramses_adaptor
