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

#include "core/Project.h"

#include <string>
#include <functional>

namespace raco::core {

class BaseContext;
class Project;
class DataChangeRecorder;
class UserObjectFactoryInterface;

using translateRefFunc = std::function<SEditorObject(SEditorObject)>;
using excludePropertyPredicateFunc = std::function<bool(const std::string&)>;


void updateSingleValue(const ValueBase *src, ValueBase *dest, ValueHandle destHandle, translateRefFunc translateRef, DataChangeRecorder *outChanges, bool invokeHandler);

void updateEditorObject(const EditorObject *src, SEditorObject dest, translateRefFunc translateRef, excludePropertyPredicateFunc excludeIf, UserObjectFactoryInterface &factory, DataChangeRecorder *outChanges, bool invokeHandler, bool updateObjectAnnotations = true);

class UndoStack {
public:
    using Callback = std::function<void()>;

    UndoStack(BaseContext *context, const Callback& onChange = []() {});

    // Add another undo stack entry.
	void push(const std::string& description, std::string mergeId = std::string());

    // Number of entries on the undo stack
    size_t size() const;
    const std::string& description(size_t index) const;

    // Get the current position of the undo stack pointer
    size_t getIndex() const;

    // Jump backward or forward to any position in the undo stack.
	// @exception ExtrefError
    size_t setIndex(size_t newIndex, bool force = false);

    // Go one entry backwards.
	// @exception ExtrefError
    void undo();

    // Go one entry forward.
	// @exception ExtrefError
	void redo();

    bool canUndo() const noexcept;
    bool canRedo() const noexcept;

	void reset();

private:
    void saveProjectState(const Project *src, Project *dest, Project *ref, const DataChangeRecorder &changes, UserObjectFactoryInterface &factory);
	void updateProjectState(const Project *src, Project *dest, const DataChangeRecorder &changes, UserObjectFactoryInterface &factory);

    // @exception ExtrefError
    void restoreProjectState(Project *src, Project *dest, BaseContext &context, UserObjectFactoryInterface &factory);

	BaseContext* context_;
	Callback onChange_;

    struct Entry {
		Entry(std::string description = std::string(), std::string mergeId = std::string());
		std::string description;
		std::string mergeId;
		Project state;
    };

    std::vector<Entry> stack_;
	size_t index_ = 0;
};

}  // namespace raco::core