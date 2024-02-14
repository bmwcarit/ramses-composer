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

#include "core/EditorObject.h"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <optional>
#include <vector>

#include <QPoint>

namespace raco::core {
class CommandInterface;
}

namespace raco::ramses_adaptor {
class AbstractSceneAdaptor;
}

namespace raco::ramses_widgets {

class TransformationController {
public:
	enum class DragMode {
		None,
		Translate,
		Rotate,
		Scale
	};

	enum class Axis {
		X = 0,
		Y,
		Z
	};

	enum class TransformMode {
		Free,
		Axis,
		Plane
	};

	TransformationController(ramses_adaptor::AbstractSceneAdaptor* abstractScene, core::CommandInterface* commandInterface);

	void beginDrag(core::SEditorObject object, DragMode mode, TransformMode transformMode, Axis axis);
	void abortDrag();
	void endDrag();

	void translate(float distance);
	void dragTranslate(QPoint initialPos, QPoint currentPos, int widgetWidth, int widgetHeight, bool snap, double snapScale);
	
	void scale(float scalingFactor);
		void dragScale(QPoint initialPos, QPoint currentPos, int widgetWidth, int widgetHeight, bool snap);
	
	void rotate(float angleDegrees);
	void dragRotate(QPoint initialPos, QPoint currentPos, int widgetWidth, int widgetHeight, bool snap);

private:
	struct Ray {
		glm::vec3 origin;
		glm::vec3 direction;
	};

	struct Plane {
		glm::vec3 normal;
		float dist;
	};

	static glm::vec3 axisVector(Axis axis);

	Ray unproject(QPoint pos, glm::mat4 world, glm::mat4 projection, glm::ivec4 viewport);
	std::optional<glm::vec3> intersect(const Ray& ray, const Plane& plane);

	QPoint project(QPoint point, glm::vec2 rayOrigin, glm::vec2 rayDirection);

	ramses_adaptor::AbstractSceneAdaptor* abstractScene_;
	core::CommandInterface* commandInterface_;

	core::SEditorObject activeObject_;
	DragMode dragMode_ = DragMode::None;
	Axis axis_;
	TransformMode transformMode_;

	glm::vec3 dragInitialObjectTranslation_;
	glm::vec3 dragInitialObjectScaling_;
	glm::vec3 dragInitialObjectRotation_;

	Plane plane_;
	glm::vec3 refAxis_;
	glm::vec3 refAxis2_;
	glm::vec3 initialWorldOrigin_;
};

}  // namespace raco::ramses_widgets
