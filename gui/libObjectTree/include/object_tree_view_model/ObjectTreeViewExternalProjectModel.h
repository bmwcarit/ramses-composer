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

#include "ObjectTreeViewDefaultModel.h"

namespace raco::object_tree::model {

class ObjectTreeViewExternalProjectModel : public ObjectTreeViewDefaultModel {
	Q_OBJECT

public:
	class ProjectNode : public raco::core::EditorObject {
	public:
		static inline const TypeDescriptor typeDescription{"Project", false};
		TypeDescriptor const& getTypeDescription() const override {
			return typeDescription;
		}

		ProjectNode(ProjectNode const& other) : EditorObject(other) {
			fillPropertyDescription();
		}

		ProjectNode(const std::string& name, const std::string& id = std::string()) : EditorObject(name, id) {
			fillPropertyDescription();
		}

		ProjectNode() : ProjectNode("Main") {}

		void fillPropertyDescription() {
		}
	};

	ObjectTreeViewExternalProjectModel(raco::core::CommandInterface* commandInterface, core::FileChangeMonitor* fileChangeMonitor, components::SDataChangeDispatcher dispatcher, core::ExternalProjectsStoreInterface* externalProjectsStoreInterface);

	QVariant data(const QModelIndex& index, int role) const override;

	void addProject(const QString& projectPath);
	void removeProject(const QModelIndex& itemIndex);
	bool canRemoveProject(const QModelIndex& itemIndex);

	Qt::TextElideMode textElideMode() const override;

	bool canCopy(const QModelIndex& index) const override;
	bool canDelete(const QModelIndex& index) const override;
	bool canPasteInto(const QModelIndex& index) const override;

public Q_SLOTS:
	void deleteObjectAtIndex(const QModelIndex& index) override;
	void copyObjectAtIndex(const QModelIndex& index, bool deepCopy) override;
	void cutObjectAtIndex(const QModelIndex& index, bool deepCut) override;
	bool pasteObjectAtIndex(const QModelIndex& index, bool pasteAsExtref = false, std::string* outError = nullptr) override;

protected:
	void buildObjectTree() override;

	Qt::ItemFlags flags(const QModelIndex& index) const override;
	QMimeData* mimeData(const QModelIndexList& indexes) const override;


	std::string getOriginProjectPathOfSelectedIndex(const QModelIndex& index) const;

	components::Subscription projectChangedSubscription_;
};

}  // namespace raco::object_tree::model