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

#include "core/BasicTypes.h"
#include "core/ChangeRecorder.h"
#include "core/Handles.h"

namespace raco::core {

class CodeControlledPropertyModifier {
public:
	template <typename Type>
	static bool setPrimitive(const core::ValueHandle& valueHandle, Type newValue, core::DataChangeRecorder& recorder) {
		auto oldValue = valueHandle.as<Type>();
		if (oldValue != newValue) {
			valueHandle.valueRef()->set(static_cast<Type>(newValue));
			recorder.recordValueChanged(valueHandle);
			return true;
		}
		return false;
	}

	static bool setVec2f(const core::ValueHandle& handle, double x, double y, core::DataChangeRecorder& recorder) {
		core::Vec2f& v = dynamic_cast<core::Vec2f&>(handle.valueRef()->asStruct());
		bool changed = false;

		if (*v.x != x) {
			v.x = x;
			recorder.recordValueChanged(handle[0]);
			changed = true;
		}
		if (*v.y != y) {
			v.y = y;
			recorder.recordValueChanged(handle[1]);
			changed = true;
		}
		return changed;
	}

	static bool setVec3f(const core::ValueHandle& handle, double x, double y, double z, core::DataChangeRecorder& recorder) {
		core::Vec3f& v = dynamic_cast<core::Vec3f&>(handle.valueRef()->asStruct());
		bool changed = false;

		if (*v.x != x) {
			v.x = x;
			recorder.recordValueChanged(handle[0]);
			changed = true;
		}
		if (*v.y != y) {
			v.y = y;
			recorder.recordValueChanged(handle[1]);
			changed = true;
		}
		if (*v.z != z) {
			v.z = z;
			recorder.recordValueChanged(handle[2]);
			changed = true;
		}
		return changed;
	}

	static bool setVec4f(const core::ValueHandle& handle, double x, double y, double z, double w, core::DataChangeRecorder& recorder) {
		core::Vec4f& v = dynamic_cast<core::Vec4f&>(handle.valueRef()->asStruct());
		bool changed = false;

		if (*v.x != x) {
			v.x = x;
			recorder.recordValueChanged(handle[0]);
			changed = true;
		}
		if (*v.y != y) {
			v.y = y;
			recorder.recordValueChanged(handle[1]);
			changed = true;
		}
		if (*v.z != z) {
			v.z = z;
			recorder.recordValueChanged(handle[2]);
			changed = true;
		}
		if (*v.w != w) {
			v.w = w;
			recorder.recordValueChanged(handle[3]);
			changed = true;
		}
		return changed;
	}

	static bool setVec2i(const core::ValueHandle& handle, int x, int y, core::DataChangeRecorder& recorder) {
		core::Vec2i& v = dynamic_cast<core::Vec2i&>(handle.valueRef()->asStruct());
		bool changed = false;

		if (*v.i1_ != x) {
			v.i1_ = x;
			recorder.recordValueChanged(handle[0]);
			changed = true;
		}
		if (*v.i2_ != y) {
			v.i2_ = y;
			recorder.recordValueChanged(handle[1]);
			changed = true;
		}
		return changed;
	}

	static bool setVec3i(const core::ValueHandle& handle, int x, int y, int z, core::DataChangeRecorder& recorder) {
		core::Vec3i& v = dynamic_cast<core::Vec3i&>(handle.valueRef()->asStruct());
		bool changed = false;

		if (*v.i1_ != x) {
			v.i1_ = x;
			recorder.recordValueChanged(handle[0]);
			changed = true;
		}
		if (*v.i2_ != y) {
			v.i2_ = y;
			recorder.recordValueChanged(handle[1]);
			changed = true;
		}
		if (*v.i3_ != z) {
			v.i3_ = z;
			recorder.recordValueChanged(handle[2]);
			changed = true;
		}
		return changed;
	}

	static bool setVec4i(const core::ValueHandle& handle, int x, int y, int z, int w, core::DataChangeRecorder& recorder) {
		core::Vec4i& v = dynamic_cast<core::Vec4i&>(handle.valueRef()->asStruct());
		bool changed = false;

		if (*v.i1_ != x) {
			v.i1_ = x;
			recorder.recordValueChanged(handle[0]);
			changed = true;
		}
		if (*v.i2_ != y) {
			v.i2_ = y;
			recorder.recordValueChanged(handle[1]);
			changed = true;
		}
		if (*v.i3_ != z) {
			v.i3_ = z;
			recorder.recordValueChanged(handle[2]);
			changed = true;
		}
		if (*v.i4_ != w) {
			v.i4_ = w;
			recorder.recordValueChanged(handle[3]);
			changed = true;
		}
		return changed;
	}
};
}  // namespace raco::core
