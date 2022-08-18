/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/Project.h"
#include "core/Undo.h"
#include "core/Errors.h"

#include "user_types/UserObjectFactory.h"
#include "user_types/Node.h"

#include "ramses_base/HeadlessEngineBackend.h"

#include "property_browser/PropertyBrowserItem.h"

namespace raco::property_browser {

template <typename T>
struct PropertyBrowserItemTestHelper {
	using PrimitiveType = data_storage::PrimitiveType;
	using BaseContext = core::BaseContext;
	using EditorObject = core::EditorObject;
	using SEditorObject = core::SEditorObject;
	using Project = core::Project;	
	using ValueHandle = core::ValueHandle;
	using UserObjectFactory = user_types::UserObjectFactory;

	std::shared_ptr<EditorObject> editorObject{std::make_shared<T>("SomeName")};
	Project project{};
	raco::ramses_base::HeadlessEngineBackend backend{};
	core::DataChangeRecorder recorder{};
	core::Errors errors{&recorder};
	const components::SDataChangeDispatcher dispatcher{std::make_shared<components::DataChangeDispatcher>()};
	const std::shared_ptr<BaseContext> context{std::make_shared<BaseContext>(&project, backend.coreInterface(), &UserObjectFactory::getInstance(), &recorder, &errors)};
	ValueHandle valueHandle{editorObject};
	core::UndoStack undoStack{context.get()};
	core::CommandInterface commandInterface{context.get(), &undoStack};

	void addPropertyTo(const std::string& containerName, PrimitiveType type) {
		editorObject->get(containerName)->asTable().addProperty(type);
		recorder.recordValueChanged(valueHandle.get(containerName));
		dispatcher->dispatch(recorder.release());
	}

	void addPropertyTo(const std::string& containerName, PrimitiveType type, const std::string& name) {
		editorObject->get(containerName)->asTable().addProperty(name, type);
		recorder.recordValueChanged(valueHandle.get(containerName));
		dispatcher->dispatch(recorder.release());
	}

	void addPropertyTo(const std::string& c1, const std::string& c2, PrimitiveType type) {
		editorObject->get(c1)->asTable().get(c2)->asTable().addProperty(type);
		recorder.recordValueChanged(valueHandle.get(c1).get(c2));
		dispatcher->dispatch(recorder.release());
	}

	void addPropertyTo(const std::string& containerName, const std::string& name, raco::data_storage::ValueBase* property) {
		editorObject->get(containerName)->asTable().addProperty(name, property);
		recorder.recordValueChanged(valueHandle.get(containerName));
		dispatcher->dispatch(recorder.release());
	}

	void removePropertyFrom(const std::string& containerName) {
		editorObject->get(containerName)->asTable().removeProperty(0);
		recorder.recordValueChanged(valueHandle.get(containerName));
		dispatcher->dispatch(recorder.release());
	}
};

}  // namespace raco::property_browser
