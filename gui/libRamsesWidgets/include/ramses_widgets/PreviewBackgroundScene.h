#ifndef __PREVIEW_BACKGROUND_SCENE_H__
#define __PREVIEW_BACKGROUND_SCENE_H__

#pragma once

#include "ramses_base/RamsesHandles.h"
#include "ramses_widgets/RendererBackend.h"
#include "ramses_adaptor/SceneBackend.h"
#include <QtGlobal>

namespace raco::ramses_widgets {

class PreviewBackgroundScene final {
	Q_DISABLE_COPY(PreviewBackgroundScene);

public:
	explicit PreviewBackgroundScene(ramses::RamsesClient& client, raco::ramses_adaptor::SceneBackend* sceneBackend, ramses::sceneId_t sceneId);
    ramses::sceneId_t getSceneId() const;
	void setEnableDisplayGrid(bool enable);
	void sceneUpdate(bool z_up, float scaleValue);

private:
	void madd_v2_v2v2fl(float r[2], const float a[2], const float b[2], float f);
	void update(bool z_up, float scaleValue);

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
	raco::ramses_adaptor::SceneBackend* sceneBackend_;
	ramses::PerspectiveCamera* globalCamera_;

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

	struct CameraParam_t {
		float translation[3];
		float rotation[3];
		float scaling[3];
		int32_t viewport[4];
		float frustum[4];

		bool operator!=(const CameraParam_t& other) {
			return (this->translation[0] != other.translation[0]
				|| this->translation[1] != other.translation[1]
				|| this->translation[2] != other.translation[2]
				|| this->rotation[0] != other.rotation[0]
				|| this->rotation[1] != other.rotation[1]
				|| this->rotation[2] != other.rotation[2]
				|| this->scaling[0] != other.scaling[0]
				|| this->scaling[1] != other.scaling[1]
				|| this->scaling[2] != other.scaling[2]
				|| this->viewport[0] != other.viewport[0]
				|| this->viewport[1] != other.viewport[1]
				|| this->viewport[2] != other.viewport[2]
				|| this->viewport[3] != other.viewport[3]
				|| this->frustum[0] != other.frustum[0]
				|| this->frustum[1] != other.frustum[1]
				|| this->frustum[2] != other.frustum[2]
				|| this->frustum[3] != other.frustum[3]);
		};
	};

	CameraParam_t currentCameraParam_;
};

}

#endif // __PREVIEW_BACKGROUND_SCENE_H__