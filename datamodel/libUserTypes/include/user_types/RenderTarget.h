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

#include "user_types/BaseObject.h"
#include "user_types/RenderBuffer.h"
#include "user_types/RenderBufferMS.h"

namespace raco::user_types {

class RenderTargetBase : public BaseObject {
public:
	static inline const TypeDescriptor typeDescription = {"RenderTargetBase", true};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	RenderTargetBase(std::string name = std::string(), std::string id = std::string())
		: BaseObject(name, id) {
	}
};

using SRenderTargetBase = std::shared_ptr<RenderTargetBase>;


class RenderTarget : public RenderTargetBase {
public:
	static inline const TypeDescriptor typeDescription = {"RenderTarget", true};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	RenderTarget(RenderTarget const& other)
		: RenderTargetBase(other),
		  buffers_(other.buffers_) {
		fillPropertyDescription();
	}

	RenderTarget(const std::string& name, const std::string& id) : RenderTargetBase(name, id) {
		fillPropertyDescription();
		buffers_->resize(1);
	}

	void fillPropertyDescription() {
		properties_.emplace_back("buffers", &buffers_);
	}


	Property<Array<SRenderBuffer>, DisplayNameAnnotation, ExpectEmptyReference, ResizableArray> buffers_{{}, {"Buffers"}, {}, {}};
};

using SRenderTarget = std::shared_ptr<RenderTarget>;


class RenderTargetMS : public RenderTargetBase {
public:
	static inline const TypeDescriptor typeDescription = {"RenderTargetMS", true};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	RenderTargetMS(RenderTargetMS const& other)
		: RenderTargetBase(other),
		  buffers_(other.buffers_) {
		fillPropertyDescription();
	}

	RenderTargetMS(const std::string& name, const std::string& id) : RenderTargetBase(name, id) {
		fillPropertyDescription();
		buffers_->resize(1);
	}

	void fillPropertyDescription() {
		properties_.emplace_back("buffers", &buffers_);
	}

	Property<Array<SRenderBufferMS>, DisplayNameAnnotation, ExpectEmptyReference, ResizableArray> buffers_{{}, {"Buffers"}, {}, {}};
};

using SRenderTargetMS = std::shared_ptr<RenderTargetMS>;

}  // namespace raco::user_types
#pragma once
