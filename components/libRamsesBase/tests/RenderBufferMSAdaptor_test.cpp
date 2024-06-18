/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include <gtest/gtest.h>

#include "RamsesBaseFixture.h"
#include "user_types/RenderBufferMS.h"
#include "user_types/RenderTarget.h"

using namespace user_types;

class RenderBufferMSAdaptorTest : public RamsesBaseFixture<> {
public:
	TextFile defaultFile() {
		return makeFile("interface.lua",
			R"___(
function interface(INOUT)
	INOUT.integer = Type:Int32()
end
)___");
	}
};

TEST_F(RenderBufferMSAdaptorTest, target_buffer_size_mismatch) {
	auto interface_file = defaultFile();
	auto lua = create_lua_interface("lua", interface_file);
	auto buffer_depth = create<RenderBufferMS>("buffer_depth");
	auto buffer_color = create<RenderBufferMS>("buffer_color");
	auto target = create_rendertarget_ms("render_target", { buffer_color, buffer_depth });
	dispatch();

	ASSERT_FALSE(errors.hasError({ target }));

	commandInterface.set({ buffer_depth, &RenderBufferMS::width_ }, 100);
	dispatch();

	ASSERT_TRUE(errors.hasError({ target }));

	commandInterface.set({ buffer_color, &RenderBufferMS::width_ }, 100);
	dispatch();

	ASSERT_FALSE(errors.hasError({ target }));
}

TEST_F(RenderBufferMSAdaptorTest, link_width) {
	auto interface_file = defaultFile();
	auto lua = create_lua_interface("lua", interface_file);
	auto buffer_depth = create<RenderBufferMS>("buffer_depth");
	auto buffer_color = create<RenderBufferMS>("buffer_color");
	auto target = create_rendertarget_ms("render_target", {buffer_color, buffer_depth});

	dispatch();
	ASSERT_FALSE(errors.hasError({ target }));

	commandInterface.set({ lua, {"inputs", "integer"} }, 256);
	commandInterface.addLink({ lua, {"inputs", "integer"} }, { buffer_depth, &RenderBufferMS::width_ });
	dispatch();
	dispatch();
	ASSERT_FALSE(errors.hasError({ target }));
	EXPECT_EQ(*buffer_depth->width_, 256);

	commandInterface.set({ lua, {"inputs", "integer"} }, 100);
	dispatch();
	dispatch();
	ASSERT_TRUE(errors.hasError({ target }));
	EXPECT_EQ(*buffer_depth->width_, 100);

	commandInterface.set({ buffer_color, &RenderBufferMS::width_ }, 100);
	dispatch();
	dispatch();
	ASSERT_FALSE(errors.hasError({ target }));
}

TEST_F(RenderBufferMSAdaptorTest, link_height) {
	auto interface_file = defaultFile();
	auto lua = create_lua_interface("lua", interface_file);
	auto buffer_depth = create<RenderBufferMS>("buffer_depth");
	auto buffer_color = create<RenderBufferMS>("buffer_color");
	auto target = create_rendertarget_ms("render_target", {buffer_color, buffer_depth});

	dispatch();
	ASSERT_FALSE(errors.hasError({ target }));

	commandInterface.set({ lua, {"inputs", "integer"} }, 256);
	commandInterface.addLink({ lua, {"inputs", "integer"} }, { buffer_depth, &RenderBufferMS::height_ });
	dispatch();
	dispatch();
	ASSERT_FALSE(errors.hasError({ target }));
	EXPECT_EQ(*buffer_depth->height_, 256);

	commandInterface.set({ lua, {"inputs", "integer"} }, 100);
	dispatch();
	dispatch();
	ASSERT_TRUE(errors.hasError({ target }));
	EXPECT_EQ(*buffer_depth->height_, 100);

	commandInterface.set({ buffer_color, &RenderBufferMS::height_ }, 100);
	dispatch();
	dispatch();
	ASSERT_FALSE(errors.hasError({ target }));
}

TEST_F(RenderBufferMSAdaptorTest, link_sample_count) {
	auto interface_file = defaultFile();
	auto lua = create_lua_interface("lua", interface_file);
	auto buffer_depth = create<RenderBufferMS>("buffer_depth");
	auto buffer_color = create<RenderBufferMS>("buffer_color");
	auto target = create_rendertarget_ms("render_target", {buffer_color, buffer_depth});

	dispatch();
	ASSERT_FALSE(errors.hasError({target}));

	commandInterface.set({lua, {"inputs", "integer"}}, 1);
	commandInterface.addLink({lua, {"inputs", "integer"}}, {buffer_depth, &RenderBufferMS::sampleCount_});
	dispatch();
	dispatch();
	ASSERT_FALSE(errors.hasError({target}));
	EXPECT_EQ(*buffer_depth->sampleCount_, 1);

	commandInterface.set({lua, {"inputs", "integer"}}, 3);
	dispatch();
	dispatch();
	ASSERT_TRUE(errors.hasError({target}));
	EXPECT_EQ(*buffer_depth->sampleCount_, 3);

	commandInterface.set({buffer_color, &RenderBufferMS::sampleCount_}, 3);
	dispatch();
	dispatch();
	ASSERT_FALSE(errors.hasError({target}));
}
