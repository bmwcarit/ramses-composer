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

#include "core/Context.h"
#include "ramses_adaptor/LinkAdaptor.h"
//#include "ramses_base/LogicEngine.h"
#include "ramses_base/RamsesHandles.h"
#include "ramses_base/BaseEngineBackend.h"
#include "ramses_adaptor/utilities.h"
#include "components/DataChangeDispatcher.h"
#include <map>
#include "core/Link.h"

namespace raco::ramses_adaptor {

class ObjectAdaptor;

using SRamsesAdaptorDispatcher = std::shared_ptr<components::DataChangeDispatcher>;
class SceneAdaptor {
	using Project = core::Project;
	using SEditorObject = core::SEditorObject;
	using SLink = core::SLink;
	using ValueHandle = core::ValueHandle;
	using SEditorObjectSet = core::SEditorObjectSet;

public:
	explicit SceneAdaptor(ramses::RamsesClient* client, ramses::sceneId_t id, Project* project, components::SDataChangeDispatcher dispatcher, core::Errors* errors, bool optimizeForExport = false);

	~SceneAdaptor();

	ramses::Scene* scene();
	ramses::sceneId_t sceneId();

	/* START: Adaptor API */
	ramses::RamsesClient* client();
	ramses::LogicEngine& logicEngine();
	const ramses::RamsesClient* client() const;
	const SRamsesAdaptorDispatcher dispatcher() const;
	const ramses_base::RamsesAppearance defaultAppearance(bool withMeshNormals);
	const ramses_base::RamsesArrayResource defaultVertices(int index);
	const ramses_base::RamsesArrayResource defaultNormals(int index); 
	const ramses_base::RamsesArrayResource defaultIndices(int index);
	ObjectAdaptor* lookupAdaptor(const core::SEditorObject& editorObject) const;
	Project& project() const;

	template <class T>
	T* lookup(const core::SEditorObject& editorObject) const {
		return dynamic_cast<T*>(lookupAdaptor(editorObject));
	}
	/* END: Adaptor API */

	void readDataFromEngine(core::DataChangeRecorder &recorder);

	void iterateAdaptors(std::function<void(ObjectAdaptor*)> func);

	bool optimizeForExport() const;

	ramses::EFeatureLevel featureLevel() const;

	void updateRuntimeError(const ramses::Issue& issue);
	void clearRuntimeError();

private:
	bool needAdaptor(SEditorObject object);
	void createLink(const core::LinkDescriptor& link);
	void changeLinkValidity(const core::LinkDescriptor& link, bool isValid);
	void removeLink(const core::LinkDescriptor& link);
	void createAdaptor(SEditorObject obj);
	void removeAdaptor(SEditorObject obj);

	void performBulkEngineUpdate(const core::SEditorObjectSet& changedObjects);

	void rebuildSortedDependencyGraph(SEditorObjectSet const& objects);


	void deleteUnusedDefaultResources();

	ramses::RamsesClient* client_;
	Project* project_;
	core::Errors* errors_;
	ramses_base::RamsesScene scene_{};

	ramses_base::BaseEngineBackend::UniqueLogicEngine logicEngine_;

	bool optimizeForExport_ = false;

	// Fallback resources: used when MeshNode doesn't have valid shader program or mesh data
	ramses_base::RamsesAppearance defaultAppearance_;
	ramses_base::RamsesAppearance defaultAppearanceWithNormals_;

	std::array<ramses_base::RamsesArrayResource, 2> defaultIndices_;
	std::array<ramses_base::RamsesArrayResource, 2> defaultVertices_;
	std::array<ramses_base::RamsesArrayResource, 2> defaultNormals_;

	std::map<SEditorObject, std::unique_ptr<ObjectAdaptor>> adaptors_{};
	
	struct LinkAdaptorContainer {
		std::map<std::string, std::map<core::LinkDescriptor, SharedLinkAdaptor>> linksByStart_{};
		std::map<std::string, std::map<core::LinkDescriptor, SharedLinkAdaptor>> linksByEnd_{};
	};

	LinkAdaptorContainer links_;
	std::set<core::LinkDescriptor> newLinks_{};

	components::Subscription subscription_;
	components::Subscription childrenSubscription_;
	components::Subscription linksLifecycle_;
	components::Subscription linkValidityChangeSub_;
	SRamsesAdaptorDispatcher dispatcher_;

	bool adaptorStatusDirty_ = false;

	std::vector<DependencyNode> dependencyGraph_;

	SEditorObject lastErrorObject_ = nullptr;
};

}  // namespace raco::ramses_adaptor
