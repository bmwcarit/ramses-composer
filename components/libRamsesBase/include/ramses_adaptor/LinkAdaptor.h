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

#include "core/Link.h"
#include <ramses/client/logic/LogicEngine.h>
#include <ramses/client/logic/Property.h>
#include "components/DataChangeDispatcher.h"
#include <vector>

namespace raco::ramses_adaptor {

class SceneAdaptor;

class LinkAdaptor {
public:
	struct EngineLink {
		ramses::Property* origin;
		ramses::Property* dest;
	};
	using UniqueEngineLink = std::unique_ptr<EngineLink, std::function<void(EngineLink*)>>;
	using SLink = core::SLink;

	explicit LinkAdaptor(const core::LinkDescriptor& link, SceneAdaptor* sceneAdaptor);
	~LinkAdaptor() {}

	core::LinkDescriptor& editorLink() noexcept { return editorLink_; }
	const core::LinkDescriptor& editorLink() const noexcept { return editorLink_; }

	void lift();
	void connect();

	/**
	 * @brief Read back data from the Ramses property for the link endpoint and update the Raco data model.
	 * @param recorder Change recorder to use for collecting actually changed properties.
	 * @return Returns true if some value was changed in Raco data model.
	 */
	bool readDataFromEngine(core::DataChangeRecorder& recorder);

protected:
	void connectHelper(const core::ValueHandle& start, const core::ValueHandle& end, bool isWeak);

	bool readFromEngineRecursive(core::DataChangeRecorder& recorder, const core::ValueHandle& property);

	SceneAdaptor* sceneAdaptor_;
	core::LinkDescriptor editorLink_;
	std::vector<UniqueEngineLink> engineLinks_;
};
using SharedLinkAdaptor = std::shared_ptr<LinkAdaptor>;

}  // namespace raco::ramses_adaptor
