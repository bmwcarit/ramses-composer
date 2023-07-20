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

#include <functional>
#include <memory>

namespace raco::core {
class BaseContext;
class EditorObject;
using SEditorObject = std::shared_ptr<EditorObject>;

class FileChangeCallback {
public:
	FileChangeCallback(BaseContext* context, SEditorObject object) : context_(context), object_(object) {}

	BaseContext* context() const {
		return context_;
	}

	SEditorObject object() const {
		return object_;
	}

	bool operator==(const FileChangeCallback& other) const {
		return context_ == other.context_ && object_ == other.object_;
	}

private:
	BaseContext* context_;
	SEditorObject object_{nullptr};
};

}  // namespace raco::core