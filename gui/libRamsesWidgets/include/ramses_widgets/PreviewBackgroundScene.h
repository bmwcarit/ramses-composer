#ifndef __PREVIEW_BACKGROUND_SCENE_H__
#define __PREVIEW_BACKGROUND_SCENE_H__

#pragma once

#include "ramses_base/RamsesHandles.h"
#include "ramses_widgets/RendererBackend.h"
#include <QtGlobal>

namespace raco::ramses_widgets {

class PreviewBackgroundScene final {
	Q_DISABLE_COPY(PreviewBackgroundScene);

public:
	explicit PreviewBackgroundScene(ramses::RamsesClient& client, ramses::sceneId_t sceneId);
    ramses::sceneId_t getSceneId() const;
    ramses::dataConsumerId_t generateAxesGridlines(RendererBackend& backend, const QSize& size);
    void setViewport(const QPoint& viewportPosition, const QSize& viewportSize, const QSize& virtualSize);
	void update(bool z_up, float scaleValue);
	void setEnableDisplayGrid(bool enable);
	void madd_v2_v2v2fl(float r[2], const float a[2], const float b[2], float f);
	raco::ramses_base::RamsesScene getScene();
	raco::ramses_base::RamsesPerspectiveCamera getCamera();

private:
	// Scene
	raco::ramses_base::RamsesScene scene_;
	raco::ramses_base::RamsesPerspectiveCamera camera_;
	std::shared_ptr<ramses::RenderGroup> renderGroup_;
	std::shared_ptr<ramses::RenderPass> renderPass_;
	raco::ramses_base::RamsesEffect effect_;
	raco::ramses_base::RamsesAppearance appearance_;
	raco::ramses_base::RamsesArrayResource indexDataBuffer_;
	raco::ramses_base::RamsesArrayResource vertexDataBuffer_;
	raco::ramses_base::RamsesArrayResource uvDataBuffer_;
	raco::ramses_base::RamsesArrayResource colorDataBuffer_;
	raco::ramses_base::RamsesGeometryBinding geometryBinding_;
	raco::ramses_base::RamsesMeshNode meshNode_;


    ramses::dataConsumerId_t backgroundSampleId_;

	bool zUp_;
    float lineMaxValue_;
	float scaleValue_;

	enum {
		SHOW_AXIS_X = (1 << 0),
		SHOW_AXIS_Y = (1 << 1),
		SHOW_AXIS_Z = (1 << 2),
		SHOW_GRID = (1 << 3),
		PLANE_XY = (1 << 4),
		PLANE_XZ = (1 << 5),
		PLANE_YZ = (1 << 6),
		CLIP_ZPOS = (1 << 7),
		CLIP_ZNEG = (1 << 8),
		GRID_BACK = (1 << 9),
		GRID_CAMERA = (1 << 10),
		PLANE_IMAGE = (1 << 11),
		CUSTOM_GRID = (1 << 12),
	};
};

}

#endif // __PREVIEW_BACKGROUND_SCENE_H__