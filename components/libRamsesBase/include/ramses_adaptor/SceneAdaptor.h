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

#include "core/Context.h"
#include "ramses_adaptor/LinkAdaptor.h"
#include "ramses_base/LogicEngine.h"
#include "ramses_base/RamsesHandles.h"
#include "components/DataChangeDispatcher.h"
#include <map>
#include "core/Link.h"

namespace raco::ramses_adaptor {

class ObjectAdaptor;

using SRamsesAdaptorDispatcher = std::shared_ptr<components::DataChangeDispatcher>;
class SceneAdaptor {
	using Project = raco::core::Project;
	using SEditorObject = raco::core::SEditorObject;
	using SLink = raco::core::SLink;
	using ValueHandle = raco::core::ValueHandle;

public:
	explicit SceneAdaptor(ramses::RamsesClient* client, ramses_base::LogicEngine* logicEngine, ramses::sceneId_t id, Project* project, components::SDataChangeDispatcher dispatcher, core::Errors *errors);

	~SceneAdaptor();

	ramses::Scene* scene();
	ramses::sceneId_t sceneId();

	/* START: Adaptor API */
	ramses::RamsesClient* client();
	ramses_base::LogicEngine& logicEngine();
	const ramses::RamsesClient* client() const;
	const SRamsesAdaptorDispatcher dispatcher() const;
	ramses::RenderGroup& defaultRenderGroup();
	void setCamera(ramses::Camera* camera);
	const ramses_base::RamsesAppearance defaultAppearance(bool withMeshNormals);
	const ramses_base::RamsesArrayResource defaultVertices();
	const ramses_base::RamsesArrayResource defaultIndices();
	ObjectAdaptor* lookupAdaptor(const core::SEditorObject& editorObject) const;
	Project& project() const;

	template <class T>
	T* lookup(const core::SEditorObject& editorObject) const {
		return dynamic_cast<T*>(lookupAdaptor(editorObject));
	}
	/* END: Adaptor API */

	void readDataFromEngine(core::DataChangeRecorder &recorder);

	void iterateAdaptors(std::function<void(ObjectAdaptor*)> func);

private:
	void createLink(const core::LinkDescriptor& link);
	void changeLinkValidity(const core::LinkDescriptor& link, bool isValid);
	void removeLink(const core::LinkDescriptor& link);
	void createAdaptor(SEditorObject obj);
	void removeAdaptor(SEditorObject obj);

	void buildDefaultRenderGroup();
	void buildRenderableOrder(ramses::RenderGroup& renderGroup, std::vector<SEditorObject>& objs, const std::function<void(const SEditorObject&)>& renderableOrderFunc);

	void performBulkEngineUpdate(const std::set<SEditorObject>& changedObjects);

	struct DependencyNode {
		SEditorObject object;
		std::set<SEditorObject> referencedObjects;
	};

	void depthFirstSearch(data_storage::ReflectionInterface* object, DependencyNode& item, std::set<SEditorObject> const& instances, std::set<SEditorObject>& sortedObjs, std::vector<DependencyNode>& outSorted);

	void depthFirstSearch(SEditorObject object, std::set<SEditorObject> const& instances, std::set<SEditorObject>& sortedObjs, std::vector<DependencyNode>& outSorted);
	void rebuildSortedDependencyGraph(std::set<SEditorObject> const& objects);

	void updateRuntimeErrorList();

	void deleteUnusedDefaultResources();

	ramses::RamsesClient* client_;
	ramses_base::LogicEngine* logicEngine_;
	Project* project_;
	core::Errors* errors_;
	ramses_base::RamsesScene scene_{};

	// Fallback resources: used when MeshNode doesn't have valid shader program or mesh data
	ramses_base::RamsesEffect defaultEffect_{};
	ramses_base::RamsesEffect defaultEffectWithNormals_{};

	raco::ramses_base::RamsesAppearance defaultAppearance_;
	raco::ramses_base::RamsesAppearance defaultAppearanceWithNormals_;

	ramses_base::RamsesArrayResource defaultIndices_{};
	ramses_base::RamsesArrayResource defaultVertices_{};

	// TODO: Dummy elements, delete when no longer needed
	ramses_base::RamsesRenderPass defaultRenderPass_{};
	ramses_base::RamsesRenderGroup defaultRenderGroup_{};
	// --- end delete ---

	std::map<SEditorObject, std::unique_ptr<ObjectAdaptor>> adaptors_{};
	std::map<core::LinkDescriptor, UniqueLinkAdaptor> links_{};
	components::Subscription subscription_;
	components::Subscription linksLifecycle_;
	components::Subscription linkValidityChangeSub_;
	SRamsesAdaptorDispatcher dispatcher_;

	std::vector<DependencyNode> dependencyGraph_;
};

}  // namespace raco::ramses_adaptor
