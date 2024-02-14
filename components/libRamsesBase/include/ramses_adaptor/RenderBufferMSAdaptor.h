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
#include "user_types/RenderBufferMS.h"
#include <memory>
#include <ramses/client/RenderBuffer.h>

namespace raco::ramses_adaptor {

	class RenderBufferMSAdaptor : public TypedObjectAdaptor<user_types::RenderBufferMS, ramses::TextureSamplerMS> {
	public:
		explicit RenderBufferMSAdaptor(SceneAdaptor* sceneAdaptor, std::shared_ptr<user_types::RenderBufferMS> editorObject);

		bool sync(core::Errors* errors) override;
		std::vector<ExportInformation> getExportInformation() const override;

		ramses_base::RamsesRenderBuffer buffer() const;

	private:
		ramses_base::RamsesRenderBuffer buffer_;

		std::array<components::Subscription, 4> subscriptions_;
	};

};