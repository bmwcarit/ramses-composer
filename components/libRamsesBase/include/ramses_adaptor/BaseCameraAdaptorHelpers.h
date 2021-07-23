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

namespace rlogic {
class Property;
class RamsesCameraBinding;
}

namespace ramses {
class Camera;
}

namespace raco::ramses_adaptor {

class SceneAdaptor;
class ObjectAdaptor;

class BaseCameraAdaptorHelpers {
public:
	static void sync(std::shared_ptr<user_types::BaseCamera> editorObject, ramses::Camera* ramsesCamera, rlogic::RamsesCameraBinding* cameraBinding);
	static const rlogic::Property* getProperty(rlogic::RamsesCameraBinding* cameraBinding, const std::vector<std::string>& propertyNamesVector);
};

};  // namespace raco::ramses_adaptor
