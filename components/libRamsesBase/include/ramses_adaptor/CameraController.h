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

#include "ramses_adaptor/utilities.h"

#include <QObject>

namespace raco::ramses_adaptor {

class CameraController : public QObject {
	Q_OBJECT
public:
	static constexpr float rotationSensitivity = 0.005;
	static constexpr float panSensitivity = 0.001;
	static constexpr float zoomMoveSensitivity = 0.01;
	static constexpr float zoomWheelSensitivity = 0.001;
	static constexpr float pitchLimit = 87.0;

	CameraController(ramses_base::RamsesPerspectiveCamera camera);

	void reset();

	/**
	 * @brief Focus the camera view frustum on the given bounding box by translating the camera position
	 * and adjusting the position-lookat distance but keeping the current direction unchanged.
	 * @param boundingBox Bounding box to focus on.
	 */
	void focus(const BoundingBox& boundingBox);

	void rescaleCameraToViewport(uint32_t width, uint32_t height);

	void orbitCamera(QPoint deltaMousePos);

	void panCamera(QPoint deltaMousePos);

	void zoomCameraMove(QPoint deltaMousePos);
	void zoomCameraWheel(QPoint deltaWheelAngle);

	float cameraDistance();

	glm::mat4 projectionMatrix();
	glm::mat4 viewMatrix();
	glm::mat4 modelMatrix();

Q_SIGNALS:
	/**
	 * @brief Signal emitted when the distance of the camera position to the lookat point changes.
	 * @param cameraDistance New distance of the camera position from the lookat point.
	 */
	void distanceChanged(float cameraDistance);

private:
	void update(bool distChanged = false);
	glm::vec3 getEulerAngles();
	void zoomCamera(float logFactor);

	ramses_base::RamsesPerspectiveCamera camera_;

	glm::vec3 position_;
	glm::vec3 lookAt_;
	glm::vec3 up_;

	float angle_;
	float aspectRatio_;
	float near_;
	float far_;
};

}  // namespace raco::ramses_adaptor