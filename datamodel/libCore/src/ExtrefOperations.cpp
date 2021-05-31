/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "core/ExtrefOperations.h"
#include "core/Context.h"
#include "core/ExternalReferenceAnnotation.h"
#include "core/Iterators.h"
#include "core/Project.h"
#include "core/Queries.h"
#include "core/Undo.h"
#include "core/UserObjectFactoryInterface.h"

#include "log_system/log.h"

#include <algorithm>
#include <cassert>

namespace raco::core {

namespace {

struct ExternalObjectDescriptor {
	SEditorObject obj;
	Project* project;
};

// @exception ExtrefError
Project* lookupExternalProject(Project* project, const std::string& projectID, ExternalProjectsStoreInterface& externalProjectsStore, std::vector<std::string>& pathStack) {
	Project* extProject = nullptr;
	if (project->hasExternalProjectMapping(projectID)) {
		auto extPath = project->lookupExternalProjectPath(projectID);

		extProject = externalProjectsStore.addExternalProject(extPath, pathStack);
		if (extProject) {
			auto loadedID = extProject->projectID();
			if (loadedID != projectID) {
				throw core::ExtrefError(fmt::format("Project ID change for file '{}' detected: '{}' changed to '{}'", extPath, projectID, loadedID));
			}
		}
	}
	return extProject;
}

// @exception ExtrefError
ExternalObjectDescriptor lookupExtrefSource(Project* project, const ExternalObjectDescriptor& descriptor, ExternalProjectsStoreInterface& externalProjectsStore, std::vector<std::string>& pathStack) {
	auto anno = descriptor.obj->query<ExternalReferenceAnnotation>();
	if (anno) {
		auto sourceProjectID = *anno->projectID_;

		if (descriptor.project->hasExternalProjectMapping(sourceProjectID)) {
			auto path = descriptor.project->lookupExternalProjectPath(sourceProjectID);
			auto name = descriptor.project->lookupExternalProjectName(sourceProjectID);
			project->addExternalProjectMapping(sourceProjectID, path, name);
		}

		Project* sourceProject = lookupExternalProject(project, sourceProjectID, externalProjectsStore, pathStack);
		if (!sourceProject) {
			auto path = descriptor.project->lookupExternalProjectPath(sourceProjectID);
			throw ExtrefError("Can't load external project '" + sourceProjectID + "' with path '" + path + "'");
		}
		project->updateExternalProjectName(sourceProjectID, sourceProject->projectName());
		auto sourceObject = sourceProject->getInstanceByID(descriptor.obj->objectID());

		return {sourceObject, sourceProject};
	}
	return descriptor;
}

// @exception ExtrefError
void collectExternalObjects(Project* project, const ExternalObjectDescriptor& descriptor, ExternalProjectsStoreInterface& externalProjectsStore, std::map<std::string, ExternalObjectDescriptor>& externalObjects, std::vector<std::string>& pathStack) {
	auto it = externalObjects.find(descriptor.obj->objectID());
	if (it == externalObjects.end()) {
		auto sourceDesc = lookupExtrefSource(project, descriptor, externalProjectsStore, pathStack);
		if (sourceDesc.obj) {
			externalObjects[descriptor.obj->objectID()] = sourceDesc;

			for (auto const& prop : ValueTreeIteratorAdaptor(ValueHandle(sourceDesc.obj))) {
				if (prop.type() == PrimitiveType::Ref) {
					auto refValue = prop.asTypedRef<EditorObject>();
					if (refValue) {
						collectExternalObjects(project, ExternalObjectDescriptor{refValue, sourceDesc.project}, externalProjectsStore, externalObjects, pathStack);
					}
				}
			}
		}
	} else {
		// If we find object by id make sure this is really from the same project (project id & path)
		auto sourceDesc = lookupExtrefSource(project, descriptor, externalProjectsStore, pathStack);
		if (sourceDesc.project->projectID() != it->second.project->projectID() ||
			sourceDesc.project->currentPath() != it->second.project->currentPath()) {
			throw ExtrefError(fmt::format("Duplicate object found: '{}' found in '{}' and '{}'.", sourceDesc.obj->objectName(), sourceDesc.project->currentPath(), it->second.project->currentPath()));
		}
	}
};

}  // namespace

void ExtrefOperations::updateExternalObjects(BaseContext& context, Project* project, ExternalProjectsStoreInterface& externalProjectsStore, std::vector<std::string>& pathStack) {
	DataChangeRecorder localChanges;
	auto extProjectMapCopy = project->externalProjectsMap();

	// local = collect all extref objects in current project
	std::set<SEditorObject> localObjects;
	std::copy_if(project->instances().begin(), project->instances().end(), std::inserter(localObjects, localObjects.end()), [](SEditorObject object) -> bool {
		return object->query<ExternalReferenceAnnotation>() != nullptr;
	});

	// remote = collect current state of extref objects in external projects
	// walk tree following all references but use objects from correct external project when following references.
	std::map<std::string, ExternalObjectDescriptor> externalObjects;

	try {
		for (auto object : localObjects) {
			if (object->getParent() == nullptr) {
				collectExternalObjects(project, ExternalObjectDescriptor{object, project}, externalProjectsStore, externalObjects, pathStack);
			}
		}
	} catch (const ExtrefError& e) {
		// Cleanup external projects that have been added by collectExternalObjects above but that are not used since we abort here.
		project->gcExternalProjectMapping();
		project->setExternalReferenceUpdateFailed(true);
		LOG_ERROR(log_system::COMMON, "External reference update failed: {}", e.what());
		throw e;
	}

	auto translateToLocal = [&localObjects](SEditorObject extObj) -> SEditorObject {
		if (extObj) {
			auto it = std::find_if(localObjects.begin(), localObjects.end(), [extObj](SEditorObject obj) {
				return obj->objectID() == extObj->objectID();
			});
			if (it != localObjects.end()) {
				return *it;
			}
		}
		return nullptr;
	};

	// Perform external -> local update

	// Delete locate objects
	std::vector<SEditorObject> toRemove;
	{
		auto it = localObjects.begin();
		while (it != localObjects.end()) {
			if (externalObjects.find((*it)->objectID()) == externalObjects.end()) {
				toRemove.emplace_back(*it);
				it = localObjects.erase(it);
			} else {
				++it;
			}
		}
	}
	// We need a full deleteObjects here in order to remove references to deleted extref objects in the local objects,
	// e.g. local material referencing extref texture with the extref update deleting the extref texture
	context.deleteObjects(toRemove, false);

	// Remove links
	// TODO: Replace vectors with sets and don't use std::find_if
	std::vector<SLink> localLinks = Queries::getLinksConnectedToObjects(*project, localObjects, true, true);
	std::vector<SLink> externalLinks;
	for (const auto& item : externalObjects) {
		auto extObj = item.second.obj;
		auto extProject = item.second.project;
		// try to avoid duplicates:
		for (auto link : Queries::getLinksConnectedToObject(*extProject, extObj, false, true)) {
			externalLinks.emplace_back(link);
		}
	}
	for (auto link : localLinks) {
		if (std::find_if(externalLinks.begin(), externalLinks.end(), [link](const SLink& srcLink) {
				return compareLinksByObjectID(*srcLink, *link);
			}) == externalLinks.end()) {
			project->removeLink(link);
			localChanges.recordRemoveLink(link->descriptor());
		}
	}

	// Create local objects
	for (auto item : externalObjects) {
		SEditorObject extObj = item.second.obj;
		if (!translateToLocal(extObj)) {
			auto localObj = context.objectFactory()->createObject(extObj->getTypeDescription().typeName, extObj->objectName(), extObj->objectID());
			localObj->addAnnotation(std::make_shared<ExternalReferenceAnnotation>(item.second.project->projectID()));
			context.project()->addInstance(localObj);
			localChanges.recordCreateObject(localObj);
			localObjects.insert(localObj);
		}
	}

	// Update properties
	for (auto item : externalObjects) {
		auto extObj = item.second.obj;
		auto localObj = translateToLocal(extObj);

		updateEditorObject(
			extObj.get(), localObj, translateToLocal, [](const std::string&) { return false; }, *context.objectFactory(), &localChanges, true, false);
	}

	// Create links
	for (auto extLink : externalLinks) {
		auto it = std::find_if(localLinks.begin(), localLinks.end(), [extLink](const SLink& srcLink) {
			return compareLinksByObjectID(*srcLink, *extLink);
		});
		if (it == localLinks.end()) {
			// create link
			auto localLink = Link::cloneLinkWithTranslation(extLink, translateToLocal);
			project->addLink(localLink);
			localChanges.recordAddLink(localLink->descriptor());
		} else if (*(*it)->isValid_ != *extLink->isValid_) {
			// update link validity
			(*it)->isValid_ = extLink->isValid_;
			localChanges.recordChangeValidityOfLink((*it)->descriptor());
		}
	}

	project->gcExternalProjectMapping();

	// Update volatile data for new or changed objects
	for (const auto& destObj : localChanges.getAllChangedObjects()) {
		destObj->onAfterDeserialization();
	}

	context.modelChanges().mergeChanges(localChanges);
	context.uiChanges().mergeChanges(localChanges);

	if (extProjectMapCopy != project->externalProjectsMap()) {
		context.modelChanges().recordExternalProjectMapChanged();
		context.uiChanges().recordExternalProjectMapChanged();
	}

	// Sync from external files for new or changed objects
	for (const auto& destObj : localChanges.getAllChangedObjects()) {
		destObj->onAfterContextActivated(context);
		// This is necessary here although neither undo nor prefab update need it:
		// we have to call handlers for local objects referencing updated extref objects.
		context.callReferencedObjectChangedHandlers(destObj);
	}

	project->setExternalReferenceUpdateFailed(false);
}

}  // namespace raco::core
