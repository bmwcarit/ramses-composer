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

class RenderTarget : public BaseObject {
public:
	static inline const TypeDescriptor typeDescription = {"RenderTarget", true};
	TypeDescriptor const& getTypeDescription() const override {
		return typeDescription;
	}

	RenderTarget(RenderTarget const& other)
		: BaseObject(other),
		  buffer0_(other.buffer0_),
		  buffer1_(other.buffer1_),
		  buffer2_(other.buffer2_),
		  buffer3_(other.buffer3_),
		  buffer4_(other.buffer4_),
		  buffer5_(other.buffer5_),
		  buffer6_(other.buffer6_),
		  buffer7_(other.buffer7_),
		  bufferMS0_(other.bufferMS0_),
		  bufferMS1_(other.bufferMS1_),
		  bufferMS2_(other.bufferMS2_),
		  bufferMS3_(other.bufferMS3_),
		  bufferMS4_(other.bufferMS4_),
		  bufferMS5_(other.bufferMS5_),
		  bufferMS6_(other.bufferMS6_),
		  bufferMS7_(other.bufferMS7_) {
		fillPropertyDescription();
	}

	RenderTarget(const std::string& name, const std::string& id) : BaseObject(name, id) {
		fillPropertyDescription();
	}

	void fillPropertyDescription() {
		properties_.emplace_back("buffer0", &buffer0_);
		properties_.emplace_back("buffer1", &buffer1_);
		properties_.emplace_back("buffer2", &buffer2_);
		properties_.emplace_back("buffer3", &buffer3_);
		properties_.emplace_back("buffer4", &buffer4_);
		properties_.emplace_back("buffer5", &buffer5_);
		properties_.emplace_back("buffer6", &buffer6_);
		properties_.emplace_back("buffer7", &buffer7_);
		properties_.emplace_back("bufferMS0", &bufferMS0_);
		properties_.emplace_back("bufferMS1", &bufferMS1_);
		properties_.emplace_back("bufferMS2", &bufferMS2_);
		properties_.emplace_back("bufferMS3", &bufferMS3_);
		properties_.emplace_back("bufferMS4", &bufferMS4_);
		properties_.emplace_back("bufferMS5", &bufferMS5_);
		properties_.emplace_back("bufferMS6", &bufferMS6_);
		properties_.emplace_back("bufferMS7", &bufferMS7_);
	}

	// TODO: want variable number of buffers
	//Property<Table, ArraySemanticAnnotation, DisplayNameAnnotation> buffers_{{}, {}, {"Buffers"}};

	Property<SRenderBuffer, DisplayNameAnnotation, ExpectEmptyReference> buffer0_{{}, {"Buffer 0"}, {}};
	Property<SRenderBuffer, DisplayNameAnnotation, ExpectEmptyReference> buffer1_{{}, {"Buffer 1"}, {}};
	Property<SRenderBuffer, DisplayNameAnnotation, ExpectEmptyReference> buffer2_{{}, {"Buffer 2"}, {}};
	Property<SRenderBuffer, DisplayNameAnnotation, ExpectEmptyReference> buffer3_{{}, {"Buffer 3"}, {}};
	Property<SRenderBuffer, DisplayNameAnnotation, ExpectEmptyReference> buffer4_{{}, {"Buffer 4"}, {}};
	Property<SRenderBuffer, DisplayNameAnnotation, ExpectEmptyReference> buffer5_{{}, {"Buffer 5"}, {}};
	Property<SRenderBuffer, DisplayNameAnnotation, ExpectEmptyReference> buffer6_{{}, {"Buffer 6"}, {}};
	Property<SRenderBuffer, DisplayNameAnnotation, ExpectEmptyReference> buffer7_{{}, {"Buffer 7"}, {}};
	Property<SRenderBufferMS, DisplayNameAnnotation, ExpectEmptyReference> bufferMS0_{{}, {"Buffer (Multisampled) 0"}, {}};
	Property<SRenderBufferMS, DisplayNameAnnotation, ExpectEmptyReference> bufferMS1_{{}, {"Buffer (Multisampled) 1"}, {}};
	Property<SRenderBufferMS, DisplayNameAnnotation, ExpectEmptyReference> bufferMS2_{{}, {"Buffer (Multisampled) 2"}, {}};
	Property<SRenderBufferMS, DisplayNameAnnotation, ExpectEmptyReference> bufferMS3_{{}, {"Buffer (Multisampled) 3"}, {}};
	Property<SRenderBufferMS, DisplayNameAnnotation, ExpectEmptyReference> bufferMS4_{{}, {"Buffer (Multisampled) 4"}, {}};
	Property<SRenderBufferMS, DisplayNameAnnotation, ExpectEmptyReference> bufferMS5_{{}, {"Buffer (Multisampled) 5"}, {}};
	Property<SRenderBufferMS, DisplayNameAnnotation, ExpectEmptyReference> bufferMS6_{{}, {"Buffer (Multisampled) 6"}, {}};
	Property<SRenderBufferMS, DisplayNameAnnotation, ExpectEmptyReference> bufferMS7_{{}, {"Buffer (Multisampled) 7"}, {}};
};

using SRenderTarget = std::shared_ptr<RenderTarget>;

}  // namespace raco::user_types
#pragma once
