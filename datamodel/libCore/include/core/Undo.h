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
#include "core/UndoState.h"

#include <functional>
#include <string>
#include <memory.h>

namespace raco::core {

class BaseContext;
class Project;
class DataChangeRecorder;
class UserObjectFactoryInterface;

using translateRefFunc = std::function<SEditorObject(SEditorObject)>;
using excludePropertyPredicateFunc = std::function<bool(const std::string &)>;

enum UndoType {
    AddPoint = 0,
    DelPoint,
    MovePoint,
    SwithPointType,
    AddCurve,
    DelCurve,
    MoveCurve
};

class UndoHelpers {
public:
	static void updateSingleValue(const ValueBase *src, ValueBase *dest, ValueHandle destHandle, translateRefFunc translateRef, DataChangeRecorder *outChanges, bool invokeHandler);

	static void callOnBeforeRemoveReferenceHandler(raco::data_storage::Table *dest, const size_t &index, raco::core::ValueHandle &destHandle);

	static void updateEditorObject(const EditorObject *src, SEditorObject dest, translateRefFunc translateRef, excludePropertyPredicateFunc excludeIf, UserObjectFactoryInterface &factory, DataChangeRecorder *outChanges, bool invokeHandler, bool updateObjectAnnotations = true);
	static void updateMissingTableProperties(const Table *src, Table *dest, ValueHandle destHandle, translateRefFunc translateRef, DataChangeRecorder *outChanges, bool invokeHandler);

private:
	static void updateTableAsArray(const Table *src, Table *dest, ValueHandle destHandle, translateRefFunc translateRef, DataChangeRecorder *outChanges, bool invokeHandler);
	static void updateStruct(const ClassWithReflectedMembers *src, ClassWithReflectedMembers *dest, ValueHandle destHandle, translateRefFunc translateRef, DataChangeRecorder *outChanges, bool invokeHandler);
	static void updateTableByName(const Table *src, Table *dest, ValueHandle destHandle, translateRefFunc translateRef, DataChangeRecorder *outChanges, bool invokeHandler);
};


class UndoStack {
public:
	using Callback = std::function<void()>;

	UndoStack(
        BaseContext *context, const Callback &onChange = []() {});

	// Add another undo stack entry.
	void push(const std::string &description, std::string mergeId = std::string());
    void push(const std::string &description, UndoState state);

	// Number of entries on the undo stack
	size_t size() const;
	const std::string &description(size_t index) const;

	// Get the current position of the undo stack pointer
	size_t getIndex() const;

	// Jump backward or forward to any position in the undo stack.
	size_t setIndex(size_t newIndex, bool force = false);

	// Go one entry backwards.
	void undo();

	// Go one entry forward.
	void redo();

	bool canUndo() const noexcept;
	bool canRedo() const noexcept;

	void reset();
    void resetUndoState(STRUCT_VISUAL_CURVE_POS pos);

protected:
	void saveProjectState(const Project *src, Project *dest, Project *ref, const DataChangeRecorder &changes, UserObjectFactoryInterface &factory);
	void updateProjectState(const Project *src, Project *dest, const DataChangeRecorder &changes, UserObjectFactoryInterface &factory);

	void restoreProjectState(Project *src, Project *dest, BaseContext &context, UserObjectFactoryInterface &factory);
    void restoreAnimationState(UndoState state);

	bool canMerge(const DataChangeRecorder &changes);

	BaseContext *context_;
	Callback onChange_;

	struct Entry {
		Entry(std::string description = std::string(), std::string mergeId = std::string());
		std::string description;
        std::string mergeId;
		Project state;
        UndoState undoState;
	};

	std::vector<std::unique_ptr<Entry>> stack_;
	size_t index_ = 0;
};

}  // namespace raco::core
