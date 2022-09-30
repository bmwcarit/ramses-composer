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

#include "core/ErrorItem.h"

#include <string>

namespace raco::core {

	class SceneBackendInterface {
	public:
		struct SceneItemDesc {
		public:
			SceneItemDesc(const std::string& type, const std::string& name, int parentIndex) : type_(type), objectName_(name), parentIndex_(parentIndex) {}

			std::string type_;
			std::string objectName_;
			int parentIndex_;
		};

		virtual bool sceneValid() const = 0;

		virtual std::string getValidationReport(core::ErrorLevel minLevel) const = 0;

		virtual uint64_t currentSceneIdValue() const = 0;

		virtual std::vector<SceneItemDesc> getSceneItemDescriptions() const = 0;

		virtual std::string getExportedObjectNames(SEditorObject editorObject) const = 0;
	};

}