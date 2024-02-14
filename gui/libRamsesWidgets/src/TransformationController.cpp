/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ramses_widgets/TransformationController.h"

#include "core/CommandInterface.h"

#include "ramses_adaptor/utilities.h"
#include "ramses_adaptor/AbstractSceneAdaptor.h"

#include "user_types/Node.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

namespace {

inline std::array<double, 3> toArray(glm::vec3 v) {
	return {v.x, v.y, v.z};
}

bool isRightHanded(glm::vec3 u, glm::vec3 v, glm::vec3 w) {
	return glm::dot(glm::cross(u, v), w) > 0.0;
}

}  // namespace

namespace raco::ramses_widgets {

glm::vec3 TransformationController::axisVector(Axis axis) {
	return glm::vec3(axis == Axis::X ? 1.0 : 0.0, axis == Axis::Y ? 1.0 : 0.0, axis == Axis::Z ? 1.0 : 0.0);
}

TransformationController::TransformationController(ramses_adaptor::AbstractSceneAdaptor* abstractScene, core::CommandInterface* commandInterface) 
	: abstractScene_(abstractScene),
	  commandInterface_(commandInterface) {
}

void TransformationController::beginDrag(core::SEditorObject object, DragMode mode, TransformMode transformMode, Axis axis) {
	activeObject_ = object;
	dragMode_ = mode;
	transformMode_ = transformMode;
	axis_ = axis;

	dragInitialObjectTranslation_ = ramses_adaptor::getRacoTranslation(activeObject_->as<user_types::Node>());
	dragInitialObjectScaling_ = ramses_adaptor::getRacoScaling(activeObject_->as<user_types::Node>());
	dragInitialObjectRotation_ = ramses_adaptor::getRacoRotation(activeObject_->as<user_types::Node>());

	auto modelMatrix = abstractScene_->modelMatrix(activeObject_);
	glm::vec3 worldOrigin = modelMatrix * glm::vec4(0, 0, 0, 1);

	if (dragMode_ == DragMode::Rotate) {
		switch (transformMode_) {
			case TransformMode::Axis: {
				glm::vec3 objectAxis = axisVector(axis_);
				glm::vec3 worldAxis = glm::normalize(glm::vec3(modelMatrix * glm::vec4(objectAxis, 0.0)));
				plane_.normal = worldAxis;
				refAxis_ = worldAxis;

				auto cameraModelMatrix = abstractScene_->cameraController().modelMatrix();
				glm::vec3 worldViewAxis = glm::normalize(cameraModelMatrix * glm::vec4(0, 0, -1, 0));

				Axis axis_1 = static_cast<Axis>((static_cast<int>(axis_) + 1) % 3);
				Axis axis_2 = static_cast<Axis>((static_cast<int>(axis_) + 2) % 3);

				auto selectAuxAxis = [worldViewAxis, modelMatrix](Axis axis1, Axis axis2) -> glm::vec3 {
					glm::vec3 w1 = glm::normalize(glm::vec3(modelMatrix * glm::vec4(axisVector(axis1), 0.0)));
					glm::vec3 w2 = glm::normalize(glm::vec3(modelMatrix * glm::vec4(axisVector(axis2), 0.0)));
					if (abs(glm::dot(w1, worldViewAxis)) > abs(glm::dot(w2, worldViewAxis))) {
						return w2;
					}
					return w1;
				};

				glm::vec3 auxAxis = selectAuxAxis(axis_1, axis_2);
				abstractScene_->enableGuides(worldOrigin, refAxis_, auxAxis, static_cast<int>(axis_), 0, false, glm::vec2(1, 0));
			} break;
			case TransformMode::Free: {
				auto cameraModelMatrix = abstractScene_->cameraController().modelMatrix();
				plane_.normal = glm::normalize(cameraModelMatrix * glm::vec4(0, 0, -1, 0));
				refAxis_ = plane_.normal;
				break;
			} break;
		}
	} else {
		switch (transformMode_) {
			case TransformMode::Plane: {
				// Translation along plane: the plane is defined by the 2 axes not selected.
				Axis axis_1 = static_cast<Axis>((static_cast<int>(axis_) + 1) % 3);
				Axis axis_2 = static_cast<Axis>((static_cast<int>(axis_) + 2) % 3);

				glm::vec3 worldAxis_1 = glm::normalize(glm::vec3(modelMatrix * glm::vec4(axisVector(axis_1), 0.0)));
				glm::vec3 worldAxis_2 = glm::normalize(glm::vec3(modelMatrix * glm::vec4(axisVector(axis_2), 0.0)));
				refAxis_ = worldAxis_1;
				refAxis2_ = worldAxis_2;
				plane_.normal = glm::normalize(glm::cross(worldAxis_1, worldAxis_2));

				abstractScene_->enableGuides(worldOrigin, worldAxis_1, worldAxis_2, static_cast<int>(axis_1), static_cast<int>(axis_2), false, glm::vec2(1, 1));
			} break;

			case TransformMode::Free: {
				auto cameraModelMatrix = abstractScene_->cameraController().modelMatrix();
				plane_.normal = glm::normalize(cameraModelMatrix * glm::vec4(0, 0, -1, 0));
			} break;
			case TransformMode::Axis: {
				glm::vec3 objectAxis = axisVector(axis_);
				glm::vec3 worldAxis = glm::normalize(glm::vec3(modelMatrix * glm::vec4(objectAxis, 0.0)));
				refAxis_ = worldAxis;

				Axis axis_1 = static_cast<Axis>((static_cast<int>(axis_) + 1) % 3);
				Axis axis_2 = static_cast<Axis>((static_cast<int>(axis_) + 2) % 3);

				auto cameraModelMatrix = abstractScene_->cameraController().modelMatrix();
				glm::vec3 worldViewAxis = glm::normalize(cameraModelMatrix * glm::vec4(0, 0, -1, 0));

				auto selectNormal = [worldViewAxis, modelMatrix](Axis axis1, Axis axis2) -> glm::vec3 {
					glm::vec3 w1 = glm::normalize(glm::vec3(modelMatrix * glm::vec4(axisVector(axis1), 0.0)));
					glm::vec3 w2 = glm::normalize(glm::vec3(modelMatrix * glm::vec4(axisVector(axis2), 0.0)));
					if (abs(glm::dot(w1, worldViewAxis)) < abs(glm::dot(w2, worldViewAxis))) {
						return w2;
					}
					return w1;
				};

				plane_.normal = selectNormal(axis_1, axis_2);

				auto refAxis = glm::cross(plane_.normal, worldAxis);
				abstractScene_->enableGuides(worldOrigin, worldAxis, refAxis, static_cast<int>(axis_), 0, false, glm::vec2(1, 0));

			} break;
		}
	}
	initialWorldOrigin_ = worldOrigin;
	plane_.dist = glm::dot(worldOrigin, plane_.normal);
}

void TransformationController::abortDrag() {
	switch (dragMode_) {
		case DragMode::Translate:
			commandInterface_->set({activeObject_, &user_types::Node::translation_}, toArray(dragInitialObjectTranslation_));
			break;
		case DragMode::Scale:
			commandInterface_->set({activeObject_, &user_types::Node::scaling_}, toArray(dragInitialObjectScaling_));
			break;
		case DragMode::Rotate:
			commandInterface_->set({activeObject_, &user_types::Node::rotation_}, toArray(dragInitialObjectRotation_));
			break;
	}
	endDrag();
}

void TransformationController::endDrag() {
	dragMode_ = DragMode::None;
	abstractScene_->disableGuides();
}

TransformationController::Ray TransformationController::unproject(QPoint pos, glm::mat4 view, glm::mat4 projection, glm::ivec4 viewport) {
	auto near = glm::unProject(glm::vec3(pos.x(), pos.y(), 0.0), view, projection, viewport);
	auto far = glm::unProject(glm::vec3(pos.x(), pos.y(), +1.0), view, projection, viewport);

	auto test_near = projection * view * glm::vec4(near, 1.0);
	auto test_far = projection * view * glm::vec4(far, 1.0);

	return Ray{near, glm::normalize(far - near)};
}

std::optional<glm::vec3> TransformationController::intersect(const Ray& ray, const Plane& plane) {
	float t = (plane.dist - glm::dot(plane.normal, ray.origin)) / glm::dot(plane.normal, ray.direction);

	if (t > 0) {
		return ray.origin + t * ray.direction;
	}
	return {};
}

QPoint TransformationController::project(QPoint qpoint, glm::vec2 rayOrigin, glm::vec2 rayDirection) {
	glm::vec2 point(qpoint.x(), qpoint.y());
	glm::vec2 q = rayOrigin + rayDirection * (glm::dot(point - rayOrigin, rayDirection) / glm::dot(rayDirection, rayDirection));
	return {static_cast<int>(q[0]), static_cast<int>(q[1])};
}

void TransformationController::translate(float distance) {
	glm::mat4 parentModelMatrix = abstractScene_->modelMatrix(activeObject_->getParent());
	if (transformMode_ == TransformMode::Axis) {
		auto worldMovement = distance * refAxis_;

		glm::vec3 objectMovement = inverse(parentModelMatrix) * glm::vec4(worldMovement, 0.0);

		auto objectTranslation = dragInitialObjectTranslation_ + objectMovement;
		commandInterface_->set({activeObject_, &user_types::Node::translation_}, toArray(objectTranslation));
	}
}

void TransformationController::dragTranslate(QPoint initialPos_, QPoint currentPos_, int widgetWidth, int widgetHeight, bool snap, double snapScale) {
	// currentPos (0,0) = top left
	QPoint currentPos(currentPos_.x(), widgetHeight - currentPos_.y());
	QPoint initialPos(initialPos_.x(), widgetHeight - initialPos_.y());

	glm::mat4 parentModelMatrix = abstractScene_->modelMatrix(activeObject_->getParent());
	auto viewMatrix = abstractScene_->cameraController().viewMatrix();
	auto projectionMatrix = abstractScene_->cameraController().projectionMatrix();
	glm::ivec4 viewport{0, 0, widgetWidth, widgetHeight};

	if (transformMode_ == TransformMode::Axis) {
		// Project current and original mouse position to screen axis
		glm::vec2 screenOrigin = glm::project(initialWorldOrigin_, viewMatrix, projectionMatrix, viewport);
		glm::vec2 screenRefAxis = glm::vec2(glm::project(initialWorldOrigin_ + refAxis_, viewMatrix, projectionMatrix, viewport)) - screenOrigin;

		auto initialProjScreen = project(initialPos, screenOrigin, screenRefAxis);
		auto currentProjScreen = project(currentPos, screenOrigin, screenRefAxis);

		// Raycast screen project coordinates into scene and calculate intersection
		auto rayInitial = unproject(initialProjScreen, viewMatrix, projectionMatrix, viewport);
		auto rayCurrent = unproject(currentProjScreen, viewMatrix, projectionMatrix, viewport);

		auto worldPosInitial = intersect(rayInitial, plane_);
		auto worldPosCurrent = intersect(rayCurrent, plane_);

		if (worldPosInitial.has_value() && worldPosCurrent.has_value()) {
			// To ensure that the movement vector doesn't have components perpendicular to the reference axis due to rounding erorrs
			// we transform the original movement vector back to the plane coordinates, pick the coordinate along the
			// reference axis, and reconstruct the world movement vector from the reference axis and the coordinate:
			glm::vec3 orthoAxis = glm::normalize(glm::cross(refAxis_, plane_.normal));
			auto dist = (glm::inverse(glm::mat3(refAxis_, orthoAxis, plane_.normal)) * (worldPosCurrent.value() - worldPosInitial.value())).x;

			if (snap) {
				dist = snapScale * round(dist/snapScale);
			}

			auto worldMovement = dist * refAxis_;

			glm::vec3 objectMovement = inverse(parentModelMatrix) * glm::vec4(worldMovement, 0.0);

			auto objectTranslation = dragInitialObjectTranslation_ + objectMovement;
			commandInterface_->set({activeObject_, &user_types::Node::translation_}, toArray(objectTranslation));
		}

	} else {
		auto rayInitial = unproject(initialPos, viewMatrix, projectionMatrix, viewport);
		auto rayCurrent = unproject(currentPos, viewMatrix, projectionMatrix, viewport);

		auto worldPosInitial = intersect(rayInitial, plane_);
		auto worldPosCurrent = intersect(rayCurrent, plane_);

		if (worldPosInitial.has_value() && worldPosCurrent.has_value()) {
			auto worldMovement = worldPosCurrent.value() - worldPosInitial.value();
			if (snap && transformMode_ == TransformMode::Plane) {
				// Snap both coordinates separately:
				// to do this we first need to determine the coordinates from the world movement and the transformation matrix built
				// from the 2 plane axes and the plane normal:
				glm::vec3 coords = (glm::inverse(glm::mat3(refAxis_, refAxis2_, plane_.normal)) * worldMovement);
				glm::vec2 snappedCoords(snapScale * round(coords.x / snapScale), snapScale * round(coords.y / snapScale));
				worldMovement = snappedCoords.x * refAxis_ + snappedCoords.y * refAxis2_;
			}

			glm::vec3 objectMovement = inverse(parentModelMatrix) * glm::vec4(worldMovement, 0.0);

			auto objectTranslation = dragInitialObjectTranslation_ + objectMovement;

			std::array<double, 3> objectTranslationArray({objectTranslation[0], objectTranslation[1], objectTranslation[2]});
			commandInterface_->set({activeObject_, &user_types::Node::translation_}, objectTranslationArray);
		}
	}
}

void TransformationController::scale(float scalingFactor) {
	if (transformMode_ == TransformMode::Axis) {
		glm::vec3 scale = dragInitialObjectScaling_;
		scale[static_cast<int>(axis_)] *= scalingFactor;

		commandInterface_->set({activeObject_, &user_types::Node::scaling_}, toArray(scale));
	} else {
		if (transformMode_ == TransformMode::Plane) {
			glm::vec3 scale = scalingFactor * dragInitialObjectScaling_;
			scale[static_cast<int>(axis_)] = dragInitialObjectScaling_[static_cast<int>(axis_)];

			commandInterface_->set({activeObject_, &user_types::Node::scaling_}, toArray(scale));
		} else if (transformMode_ == TransformMode::Free) {
			glm::vec3 scale = scalingFactor * dragInitialObjectScaling_;

			commandInterface_->set({activeObject_, &user_types::Node::scaling_}, toArray(scale));
		}
	}
}

void TransformationController::dragScale(QPoint initialPos_, QPoint currentPos_, int widgetWidth, int widgetHeight, bool snap) {
	const double snapScale = 0.1;

	QPoint currentPos(currentPos_.x(), widgetHeight - currentPos_.y());
	QPoint initialPos(initialPos_.x(), widgetHeight - initialPos_.y());

	auto viewMatrix = abstractScene_->cameraController().viewMatrix();
	auto projectionMatrix = abstractScene_->cameraController().projectionMatrix();
	glm::ivec4 viewport{0, 0, widgetWidth, widgetHeight};

	if (transformMode_ == TransformMode::Axis) {
		glm::vec2 screenOrigin = glm::project(initialWorldOrigin_, viewMatrix, projectionMatrix, viewport);
		glm::vec2 screenRefAxis = glm::vec2(glm::project(initialWorldOrigin_ + refAxis_, viewMatrix, projectionMatrix, viewport)) - screenOrigin;

		auto initialProjScreen = project(initialPos, screenOrigin, screenRefAxis);
		auto currentProjScreen = project(currentPos, screenOrigin, screenRefAxis);

		auto rayInitial = unproject(initialProjScreen, viewMatrix, projectionMatrix, viewport);
		auto rayCurrent = unproject(currentProjScreen, viewMatrix, projectionMatrix, viewport);

		auto worldPosInitial = intersect(rayInitial, plane_);
		auto worldPosCurrent = intersect(rayCurrent, plane_);

		if (worldPosInitial.has_value() && worldPosCurrent.has_value()) {
			float scalingFactor = glm::length(worldPosCurrent.value() - initialWorldOrigin_) / glm::length(worldPosInitial.value() - initialWorldOrigin_);
			if (snap) {
				scalingFactor = snapScale * round(scalingFactor / snapScale);
			}

			glm::vec3 scale = dragInitialObjectScaling_;
			scale[static_cast<int>(axis_)] *= scalingFactor;

			std::array<double, 3> objectScaling({scale[0], scale[1], scale[2]});
			commandInterface_->set({activeObject_, &user_types::Node::scaling_}, objectScaling);
		}
	} else {
		auto rayInitial = unproject(initialPos, viewMatrix, projectionMatrix, viewport);
		auto rayCurrent = unproject(currentPos, viewMatrix, projectionMatrix, viewport);

		auto worldPosInitial = intersect(rayInitial, plane_);
		auto worldPosCurrent = intersect(rayCurrent, plane_);

		if (transformMode_ == TransformMode::Plane) {
			if (worldPosInitial.has_value() && worldPosCurrent.has_value()) {
				float scalingFactor = glm::length(worldPosCurrent.value() - initialWorldOrigin_) / glm::length(worldPosInitial.value() - initialWorldOrigin_);
				if (snap) {
					scalingFactor = snapScale * round(scalingFactor / snapScale);
				}
				
				glm::vec3 scale = scalingFactor * dragInitialObjectScaling_;
				scale[static_cast<int>(axis_)] = dragInitialObjectScaling_[static_cast<int>(axis_)];

				std::array<double, 3> objectScaling({scale[0], scale[1], scale[2]});
				commandInterface_->set({activeObject_, &user_types::Node::scaling_}, objectScaling);
			}
		} else if (transformMode_ == TransformMode::Free) {
			if (worldPosInitial.has_value() && worldPosCurrent.has_value()) {
				float scalingFactor = glm::length(worldPosCurrent.value() - initialWorldOrigin_) / glm::length(worldPosInitial.value() - initialWorldOrigin_);
				if (snap) {
					scalingFactor = snapScale * round(scalingFactor / snapScale);
				}
				
				glm::vec3 scale = scalingFactor * dragInitialObjectScaling_;

				std::array<double, 3> objectScaling({scale[0], scale[1], scale[2]});
				commandInterface_->set({activeObject_, &user_types::Node::scaling_}, objectScaling);
			}
		}
	}
}

void TransformationController::rotate(float angleDegrees) {
	glm::mat4 parentModelMatrix = abstractScene_->modelMatrix(activeObject_->getParent());
	
	if (transformMode_ == TransformMode::Axis || transformMode_ == TransformMode::Free) {
		glm::mat4 initialRotation = glm::eulerAngleXYZ(glm::radians(dragInitialObjectRotation_[0]), glm::radians(dragInitialObjectRotation_[1]), glm ::radians(dragInitialObjectRotation_[2]));
		glm::vec3 objectRefAxis = glm::normalize(glm::inverse(parentModelMatrix) * glm::vec4(refAxis_, 0.0));
		glm::mat4 deltaRotation = glm::rotate(glm::identity<glm::mat4>(), glm::radians(angleDegrees), objectRefAxis);
		glm::mat4 objectRotation = deltaRotation * initialRotation;
		float rotX, rotY, rotZ;
		glm::extractEulerAngleXYZ(objectRotation, rotX, rotY, rotZ);
		std::array<double, 3> rotationEulerAngles({glm::degrees(rotX), glm::degrees(rotY), glm::degrees(rotZ)});
		commandInterface_->set({activeObject_, &user_types::Node::rotation_}, rotationEulerAngles);
	}
}

void TransformationController::dragRotate(QPoint initialPos_, QPoint currentPos_, int widgetWidth, int widgetHeight, bool snap) {
	const double snapScaleRadian = glm::radians(5.0);
	QPoint currentPos(currentPos_.x(), widgetHeight - currentPos_.y());
	QPoint initialPos(initialPos_.x(), widgetHeight - initialPos_.y());

	glm::mat4 parentModelMatrix = abstractScene_->modelMatrix(activeObject_->getParent());
	auto viewMatrix = abstractScene_->cameraController().viewMatrix();
	auto projectionMatrix = abstractScene_->cameraController().projectionMatrix();
	glm::ivec4 viewport{0, 0, widgetWidth, widgetHeight};

	if (transformMode_ == TransformMode::Axis || transformMode_ == TransformMode::Free) {
		auto rayInitial = unproject(initialPos, viewMatrix, projectionMatrix, viewport);
		auto rayCurrent = unproject(currentPos, viewMatrix, projectionMatrix, viewport);

		auto worldPosInitial = intersect(rayInitial, plane_);
		auto worldPosCurrent = intersect(rayCurrent, plane_);

		if (worldPosInitial.has_value() && worldPosCurrent.has_value()) {
			glm::vec3 startDirection = glm::normalize(worldPosInitial.value() - initialWorldOrigin_);
			glm::vec3 currentDirection = glm::normalize(worldPosCurrent.value() - initialWorldOrigin_);

			float angle = acos(glm::dot(startDirection, currentDirection));
			if (!isRightHanded(startDirection, currentDirection, refAxis_)) {
				angle = -angle;
			}
			if (snap) {
				angle = snapScaleRadian * round(angle / snapScaleRadian);
			}

			glm::mat4 initialRotation = glm::eulerAngleXYZ(glm::radians(dragInitialObjectRotation_[0]), glm::radians(dragInitialObjectRotation_[1]), glm ::radians(dragInitialObjectRotation_[2]));
			glm::vec3 objectRefAxis = glm::normalize(glm::inverse(parentModelMatrix) * glm::vec4(refAxis_, 0.0));
			glm::mat4 deltaRotation = glm::rotate(glm::identity<glm::mat4>(), angle, objectRefAxis);
			glm::mat4 objectRotation = deltaRotation * initialRotation;
			float rotX, rotY, rotZ;
			glm::extractEulerAngleXYZ(objectRotation, rotX, rotY, rotZ);
			std::array<double, 3> rotationEulerAngles({glm::degrees(rotX), glm::degrees(rotY), glm::degrees(rotZ)});
			commandInterface_->set({activeObject_, &user_types::Node::rotation_}, rotationEulerAngles);
		}
	}
}

}  // namespace raco::ramses_widgets