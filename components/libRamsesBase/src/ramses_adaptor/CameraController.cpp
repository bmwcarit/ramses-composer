/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "ramses_adaptor/CameraController.h"

#include <glm/gtx/vector_angle.hpp>

#include <QPoint>

namespace raco::ramses_adaptor {

using namespace raco::ramses_base;

CameraController::CameraController(ramses_base::RamsesPerspectiveCamera camera)
	: camera_(camera),
	  angle_(35.0),
	  aspectRatio_(1.0) {
	reset();
}

void CameraController::update(bool distChanged) {
	if (distChanged) {
		far_ = 10 * cameraDistance();
		near_ = far_ / 100.0;
	}

	(*camera_)->setTranslation(position_);
	auto rotation = getEulerAngles();
	(*camera_)->setRotation(glm::degrees(rotation), ramses::ERotationType::Euler_XYZ);
	(*camera_)->setFrustum(angle_, aspectRatio_, near_, far_);

	if (distChanged) {
		Q_EMIT distanceChanged(cameraDistance());
	}
}

void CameraController::reset() {
	up_ = {0.0, 1.0, 0.0};
	lookAt_ = {0.0, 0.0, 0.0};
	position_ = {2.0, 2.0, 10.0};
	update(true);
}

void CameraController::focus(const BoundingBox& boundingBox) {
	auto front = glm::normalize(lookAt_ - position_);
	float bboxSize = boundingBox.size();
	float newDist = bboxSize / tan(glm::radians(angle_) / 2.0);

	lookAt_ = boundingBox.center();
	position_ = lookAt_ - newDist * front;

	update(true);
}

void CameraController::rescaleCameraToViewport(uint32_t width, uint32_t height) {
	(*camera_)->setViewport(0, 0, width, height);
	aspectRatio_ = static_cast<float>(width) / static_cast<float>(height);
	(*camera_)->setFrustum(angle_, aspectRatio_, near_, far_);
}

glm::vec3 CameraController::getEulerAngles() {
	auto front = glm::normalize(lookAt_ - position_);
	auto right = glm::normalize(glm::cross(front, up_));

	auto yaw = atan2(-right.z, right.x);
	auto pitch = glm::pi<float>() / 2.0 - glm::angle(up_, front);

	return {pitch, yaw, 0.0};
}

void CameraController::orbitCamera(QPoint deltaMousePos) {
	auto rotation = getEulerAngles();
	auto dist = glm::length(lookAt_ - position_);

	auto deltaYaw = -deltaMousePos.x() * rotationSensitivity;
	auto deltaPitch = -deltaMousePos.y() * rotationSensitivity;
	auto newPitch = glm::clamp(rotation.x + deltaPitch, -glm::radians(pitchLimit), glm::radians(pitchLimit));
	rotation = glm::vec3(newPitch, rotation.y + deltaYaw, 0.0);

	auto newFront = glm::rotateY(glm::rotateX(glm::vec3(0.0, 0.0, -1.0), rotation.x), rotation.y);
	position_ = lookAt_ - dist * newFront;

	(*camera_)->setTranslation(position_);
	(*camera_)->setRotation(glm::degrees(rotation), ramses::ERotationType::Euler_XYZ);
}

void CameraController::panCamera(QPoint deltaMousePos) {
	auto dist = glm::length(lookAt_ - position_);
	auto front = glm::normalize(lookAt_ - position_);
	auto right = glm::normalize(glm::cross(front, up_));
	auto top = glm::cross(right, front);

	auto translation = panSensitivity * dist * (-static_cast<float>(deltaMousePos.x()) * right + static_cast<float>(deltaMousePos.y()) * top);

	position_ += translation;
	lookAt_ += translation;

	(*camera_)->setTranslation(position_);
}

void CameraController::zoomCamera(float logFactor) {
	auto front = glm::normalize(lookAt_ - position_);
	auto dist = glm::length(lookAt_ - position_);

	float newDist = std::min(std::max<float>(dist * exp(logFactor), 1e-3f), 1e8f);

	position_ = lookAt_ - newDist * front;

	update(true);
}

void CameraController::zoomCameraMove(QPoint deltaMousePos) {
	zoomCamera(-zoomMoveSensitivity * deltaMousePos.x());
}

void CameraController::zoomCameraWheel(QPoint deltaWheelAngle) {
	float delta = deltaWheelAngle.y() != 0 ? deltaWheelAngle.y() : deltaWheelAngle.x();
	zoomCamera(-zoomWheelSensitivity * delta);
}

float CameraController::cameraDistance() {
	return glm::length(lookAt_ - position_);
}

glm::mat4 CameraController::projectionMatrix() {
	glm::mat4 matrix;
	bool status = (*camera_)->getProjectionMatrix(matrix);
	return matrix;
}

glm::mat4 CameraController::viewMatrix() {
	glm::mat4 matrix;
	bool status = (*camera_)->getModelMatrix(matrix);
	return glm::inverse(matrix);
}

glm::mat4 CameraController::modelMatrix() {
	glm::mat4 matrix;
	bool status = (*camera_)->getModelMatrix(matrix);
	return matrix;
}


}  // namespace raco::ramses_adaptor
