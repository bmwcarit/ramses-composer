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

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace raco::user_types {
class BaseCamera;
}

namespace raco::components {
class Subscription;
}

namespace ramses {
class Property;
class CameraBinding;
}

namespace ramses {
class Camera;
}

namespace raco::core {
class Errors;
}

namespace raco::ramses_adaptor {

class SceneAdaptor;
class ObjectAdaptor;

class BaseCameraAdaptorHelpers {
public:
	static void sync(std::shared_ptr<user_types::BaseCamera> editorObject, ramses::Camera* ramsesCamera, ramses::CameraBinding* cameraBinding, core::Errors* errors);
	static ramses::Property* getProperty(ramses::CameraBinding* cameraBinding, const std::vector<std::string_view>& propertyNamesVector);
};

};  // namespace raco::ramses_adaptor
