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

#include "core/Link.h"
#include <ramses-logic/LogicEngine.h>
#include <ramses-logic/Property.h>
#include "components/DataChangeDispatcher.h"
#include <vector>

namespace raco::ramses_adaptor {

class SceneAdaptor;

class LinkAdaptor {
public:
	struct EngineLink {
		const rlogic::Property* origin;
		const rlogic::Property* dest;
	};
	using UniqueEngineLink = std::unique_ptr<EngineLink, std::function<void(EngineLink*)>>;
	using SLink = raco::core::SLink;

	explicit LinkAdaptor(const core::LinkDescriptor& link, SceneAdaptor* sceneAdaptor);
	~LinkAdaptor() {}

	core::LinkDescriptor& editorLink() noexcept { return editorLink_; }
	const core::LinkDescriptor& editorLink() const noexcept { return editorLink_; }

	void lift();
	void connect();

	void readDataFromEngine(core::DataChangeRecorder &recorder);

protected:
	SceneAdaptor* sceneAdaptor_;
	core::LinkDescriptor editorLink_;
	std::vector<UniqueEngineLink> engineLink_;
};
using UniqueLinkAdaptor = std::unique_ptr<LinkAdaptor>;

}  // namespace raco::ramses_adaptor
