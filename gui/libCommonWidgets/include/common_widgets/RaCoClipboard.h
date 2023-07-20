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

#include <string>

namespace raco::MimeTypes {
constexpr const char* EDITOR_OBJECT_ID = "raco/editor-object-id";
constexpr const char* VALUE_HANDLE_PATH = "raco/value-handle-path";
constexpr const char* OBJECTS = "application/raco-json-objects";
constexpr const char* PROPERTY = "application/raco-json-property";
}  // namespace raco::MimeTypes

namespace raco::RaCoClipboard {
void set(const std::string& content, const char* mimeType = MimeTypes::OBJECTS);
bool hasEditorObject();
std::string get(const char* mimeType = MimeTypes::OBJECTS);

}  // namespace raco::RaCoClipboard
