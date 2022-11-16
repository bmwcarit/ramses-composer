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
#include "ramses_adaptor/BlitPassAdaptor.h"
#include "user_types/RenderBuffer.h"
#include "user_types/RenderBufferMS.h"

using namespace raco::user_types;

class BlitPassAdaptorTest : public RamsesBaseFixture<> {};

TEST_F(BlitPassAdaptorTest, defaultConstruction) {
	auto blitPass = context.createObject(BlitPass::typeDescription.typeName, "BlitPass");

	dispatch();

	auto blitPasses{select<ramses::BlitPass>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_BlitPass)};
	ASSERT_EQ(blitPasses.size(), 0);
}

TEST_F(BlitPassAdaptorTest, validBlitPass_BlitPassObj_sourceSingleSample_targetSingleSample) {
	auto blitPass = context.createObject(BlitPass::typeDescription.typeName, "BlitPass");
	auto sourceRenderBuffer = context.createObject(RenderBuffer::typeDescription.typeName, "RenderBuffer");
	auto targetRenderBuffer = context.createObject(RenderBuffer::typeDescription.typeName, "RenderBuffer");
	dispatch();

	context.set({blitPass, &raco::user_types::BlitPass::sourceRenderBuffer_}, sourceRenderBuffer);
	context.set({blitPass, &raco::user_types::BlitPass::targetRenderBuffer_}, targetRenderBuffer);
	dispatch();

	auto blitPasses{select<ramses::BlitPass>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_BlitPass)};
	ASSERT_EQ(blitPasses.size(), 1);
}

TEST_F(BlitPassAdaptorTest, validBlitPass_BlitPassObj_sourceMultiSample_targetMultiSample) {
	auto blitPass = context.createObject(BlitPass::typeDescription.typeName, "BlitPass");
	auto sourceRenderBuffer = context.createObject(RenderBufferMS::typeDescription.typeName, "RenderBufferMS");
	auto targetRenderBuffer = context.createObject(RenderBufferMS::typeDescription.typeName, "RenderBufferMS");
	dispatch();

	context.set({blitPass, &raco::user_types::BlitPass::sourceRenderBufferMS_}, sourceRenderBuffer);
	context.set({blitPass, &raco::user_types::BlitPass::targetRenderBufferMS_}, targetRenderBuffer);
	dispatch();

	auto blitPasses{select<ramses::BlitPass>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_BlitPass)};
	ASSERT_EQ(blitPasses.size(), 1);
}

TEST_F(BlitPassAdaptorTest, validBlitPass_BlitPassObj_sourceSingleSample_targetMultiSample) {
	auto blitPass = context.createObject(BlitPass::typeDescription.typeName, "BlitPass");
	auto sourceRenderBuffer = context.createObject(RenderBuffer::typeDescription.typeName, "RenderBuffer");
	auto targetRenderBuffer = context.createObject(RenderBufferMS::typeDescription.typeName, "RenderBufferMS");
	dispatch();

	context.set({blitPass, &raco::user_types::BlitPass::sourceRenderBuffer_}, sourceRenderBuffer);
	context.set({blitPass, &raco::user_types::BlitPass::targetRenderBufferMS_}, targetRenderBuffer);
	dispatch();

	auto blitPasses{select<ramses::BlitPass>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_BlitPass)};
	ASSERT_EQ(blitPasses.size(), 1);
}

TEST_F(BlitPassAdaptorTest, validBlitPass_BlitPassObj_sourceMultiSample_targetSingleSample) {
	auto blitPass = context.createObject(BlitPass::typeDescription.typeName, "BlitPass");
	auto sourceRenderBuffer = context.createObject(RenderBufferMS::typeDescription.typeName, "RenderBufferMS");
	auto targetRenderBuffer = context.createObject(RenderBuffer::typeDescription.typeName, "RenderBuffer");
	dispatch();

	context.set({blitPass, &raco::user_types::BlitPass::sourceRenderBufferMS_}, sourceRenderBuffer);
	context.set({blitPass, &raco::user_types::BlitPass::targetRenderBuffer_}, targetRenderBuffer);
	dispatch();

	auto blitPasses{select<ramses::BlitPass>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_BlitPass)};
	ASSERT_EQ(blitPasses.size(), 1);
}

TEST_F(BlitPassAdaptorTest, validBlitPass_BlitPassObj_all_refs_filled) {
	auto blitPass = context.createObject(BlitPass::typeDescription.typeName, "BlitPass");
	auto sourceRenderBuffer = context.createObject(RenderBuffer::typeDescription.typeName, "RenderBuffer");
	auto targetRenderBuffer = context.createObject(RenderBuffer::typeDescription.typeName, "RenderBuffer");
	auto sourceRenderBufferMS = context.createObject(RenderBufferMS::typeDescription.typeName, "RenderBufferMS");
	auto targetRenderBufferMS = context.createObject(RenderBufferMS::typeDescription.typeName, "RenderBufferMS");
	dispatch();

	context.set({blitPass, &raco::user_types::BlitPass::sourceRenderBuffer_}, sourceRenderBuffer);
	context.set({blitPass, &raco::user_types::BlitPass::targetRenderBuffer_}, targetRenderBuffer);
	context.set({blitPass, &raco::user_types::BlitPass::sourceRenderBufferMS_}, sourceRenderBufferMS);
	context.set({blitPass, &raco::user_types::BlitPass::targetRenderBufferMS_}, targetRenderBufferMS);
	dispatch();

	auto blitPasses{select<ramses::BlitPass>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_BlitPass)};
	ASSERT_EQ(blitPasses.size(), 1);
}

TEST_F(BlitPassAdaptorTest, validBlitPass_BlitPassObj_invalidate) {
	auto blitPass = context.createObject(BlitPass::typeDescription.typeName, "BlitPass");
	auto sourceRenderBuffer = context.createObject(RenderBuffer::typeDescription.typeName, "RenderBuffer");
	auto targetRenderBuffer = context.createObject(RenderBuffer::typeDescription.typeName, "RenderBuffer");
	dispatch();

	context.set({blitPass, &raco::user_types::BlitPass::sourceRenderBuffer_}, sourceRenderBuffer);
	context.set({blitPass, &raco::user_types::BlitPass::targetRenderBuffer_}, targetRenderBuffer);
	dispatch();

	context.set({blitPass, &raco::user_types::BlitPass::width_}, 600);
	dispatch();

	auto blitPasses{select<ramses::BlitPass>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_BlitPass)};
	ASSERT_EQ(blitPasses.size(), 1);
	ASSERT_TRUE(context.errors().hasError(blitPass));

	context.set({blitPass, &raco::user_types::BlitPass::width_}, 256);
	dispatch();

	blitPasses = select<ramses::BlitPass>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_BlitPass);
	ASSERT_EQ(blitPasses.size(), 1);
	ASSERT_FALSE(context.errors().hasError(blitPass));
}

TEST_F(BlitPassAdaptorTest, validBlitPass_nameChange) {
	auto blitPass = context.createObject(BlitPass::typeDescription.typeName, "MyBlitPass");
	auto sourceRenderBuffer = context.createObject(RenderBuffer::typeDescription.typeName, "RenderBuffer");
	auto targetRenderBuffer = context.createObject(RenderBuffer::typeDescription.typeName, "RenderBuffer");
	dispatch();

	context.set({blitPass, &raco::user_types::BlitPass::sourceRenderBuffer_}, sourceRenderBuffer);
	context.set({blitPass, &raco::user_types::BlitPass::targetRenderBuffer_}, targetRenderBuffer);
	dispatch();

	auto blitPasses{select<ramses::BlitPass>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_BlitPass)};
	ASSERT_EQ(blitPasses.size(), 1);
	ASSERT_STREQ(blitPasses.front()->getName(), "MyBlitPass");

	context.set({blitPass, &raco::user_types::BlitPass::objectName_}, std::string("BlitPassRenamed"));
	dispatch();

	blitPasses = select<ramses::BlitPass>(*sceneContext.scene(), ramses::ERamsesObjectType::ERamsesObjectType_BlitPass);
	ASSERT_EQ(blitPasses.size(), 1);
	ASSERT_STREQ(blitPasses.front()->getName(), "BlitPassRenamed");

}