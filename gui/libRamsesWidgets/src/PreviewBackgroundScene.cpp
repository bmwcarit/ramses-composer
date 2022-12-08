#include "ramses_widgets/PreviewBackgroundScene.h"
#include "ramses_widgets/SceneStateEventHandler.h"
#include "ramses_widgets/grid_vert.h"
#include "ramses_widgets/grid_frag.h"

namespace raco::ramses_widgets {

using namespace raco::ramses_base;

PreviewBackgroundScene::PreviewBackgroundScene(
	ramses::RamsesClient& client,
    raco::ramses_adaptor::SceneBackend* sceneBackend, 
	ramses::sceneId_t sceneId)
	: scene_{ ramsesScene(sceneId, &client)}, 
    sceneBackend_(sceneBackend),
	camera_{ ramsesPerspectiveCamera(scene_.get()) }, 
	renderGroup_{std::shared_ptr<ramses::RenderGroup>(scene_.get()->createRenderGroup(), raco::ramses_base::createRamsesObjectDeleter<ramses::RenderGroup>(scene_.get()))},
	renderPass_{scene_->createRenderPass(), raco::ramses_base::createRamsesObjectDeleter<ramses::RenderPass>(scene_.get())} {

    lineMaxValue_ = 500.0f;
    scaleValue_ = 1.0f;
    zUp_ = true;
    globalCamera_ = nullptr;

    (*camera_)->setViewport(0, 0, 1440u, 720u);
    (*camera_)->setFrustum(35.f, 1440.f / 720.f, 0.1f, 1000.f);
    (*camera_)->setTranslation(0.0f, 0.0f, 10.0f);
    (*camera_)->setRotation(0.0, 0.0, 0.0, ramses::ERotationConvention::XYZ);

    currentCameraParam_.translation[0] = 0.0f;
    currentCameraParam_.translation[1] = 0.0f;
    currentCameraParam_.translation[2] = 10.0f;
    currentCameraParam_.rotation[0] = 0.0f;
    currentCameraParam_.rotation[1] = 0.0f;
    currentCameraParam_.rotation[2] = 0.0f;
    currentCameraParam_.viewport[0] = 0;
    currentCameraParam_.viewport[1] = 0;
    currentCameraParam_.viewport[2] = 1440;
    currentCameraParam_.viewport[3] = 720;
    currentCameraParam_.frustum[0] = 35.f;
    currentCameraParam_.frustum[1] = 1440.f / 720.f;
    currentCameraParam_.frustum[2] = 0.1f;
    currentCameraParam_.frustum[3] = 1000.f;

	renderPass_->setCamera(**camera_);
	renderPass_->addRenderGroup(*renderGroup_.get());
	renderPass_->setClearFlags(ramses::EClearFlags_All);
    renderPass_->setClearColor(0.0, 0.0, 0.0, 0.0);
    renderPass_->setRenderOrder(0);

    std::vector<float> vertex_data;
    uint v_idx = 0;
    for (int i = 0; i < 8; i++) {
      for (int j = 0; j < 8; j++) {
        float pos0[2] = {(float)i / 8.0f, (float)j / 8.0f};
        float pos1[2] = {(float)(i + 1) / 8.0f, (float)j / 8.0f};
        float pos2[2] = {(float)i / 8.0f, (float)(j + 1) / 8.0f};
        float pos3[2] = {(float)(i + 1) / 8.0f, (float)(j + 1) / 8.0f};

        float value[2] = {-1.0f, -1.0f};
		madd_v2_v2v2fl(pos0, value, pos0, 2.0f);
		madd_v2_v2v2fl(pos1, value, pos1, 2.0f);
		madd_v2_v2v2fl(pos2, value, pos2, 2.0f);
		madd_v2_v2v2fl(pos3, value, pos3, 2.0f);

        vertex_data.push_back(pos0[0]);
        vertex_data.push_back(pos0[1]);
        vertex_data.push_back(pos1[0]);
        vertex_data.push_back(pos1[1]);
        vertex_data.push_back(pos2[0]);
        vertex_data.push_back(pos2[1]);
        vertex_data.push_back(pos3[0]);
        vertex_data.push_back(pos3[1]);
      }
    }
	vertexDataBuffer_ = ramsesArrayResource(scene_.get(), ramses::EDataType::Vector2F, vertex_data.size() / 2, vertex_data.data());
    std::vector<uint32_t> index_data;
    for (uint32_t i = 0; i < vertex_data.size() / 2 / 4; i++) {
        index_data.push_back(i * 4);
        index_data.push_back(i * 4 + 1);
        index_data.push_back(i * 4 + 2);
        index_data.push_back(i * 4 + 2);
        index_data.push_back(i * 4 + 1);
        index_data.push_back(i * 4 + 3);
    }
	indexDataBuffer_ = ramsesArrayResource(scene_.get(), ramses::EDataType::UInt32, index_data.size(), index_data.data());

    ramses::EffectDescription effectDesc;
    effectDesc.setVertexShader(grid_vert);
    effectDesc.setFragmentShader(grid_frag);
    effectDesc.setUniformSemantic("ViewProjectionMatrix", ramses::EEffectUniformSemantic::ModelViewProjectionMatrix);
    effectDesc.setUniformSemantic("ProjectionMatrix", ramses::EEffectUniformSemantic::ProjectionMatrix);
    effectDesc.setUniformSemantic("ViewMatrix", ramses::EEffectUniformSemantic::ViewMatrix);

	effect_ = ramsesEffect(scene_.get(), effectDesc);
	appearance_ = ramsesAppearance(scene_.get(), effect_);
    appearance_->get()->setBlendingOperations(ramses::EBlendOperation_Add, ramses::EBlendOperation_Add);
    appearance_->get()->setBlendingFactors(ramses::EBlendFactor_SrcAlpha,
        ramses::EBlendFactor_OneMinusSrcAlpha,
        ramses::EBlendFactor_One,
        ramses::EBlendFactor_One);

 	geometryBinding_ = ramsesGeometryBinding(scene_.get(), effect_);

	(*geometryBinding_)->setIndices(*indexDataBuffer_.get());

    ramses::AttributeInput positionsInput;
    effect_->findAttributeInput("pos", positionsInput);
    (*geometryBinding_)->setInputBuffer(positionsInput, *vertexDataBuffer_.get());

	meshNode_ =  ramsesMeshNode(scene_.get());
	meshNode_->setGeometryBinding(geometryBinding_);
	meshNode_->setAppearance(appearance_);

	renderGroup_->addMeshNode(**meshNode_);

 	(*appearance_)->setDrawMode(ramses::EDrawMode::EDrawMode_Triangles);

    // signal the scene it is in a state that can be rendered
    scene_->flush();
 
    // distribute the scene to RAMSES
    scene_->publish();

}

ramses::sceneId_t PreviewBackgroundScene::getSceneId() const {
	return scene_->getSceneId();
}

void PreviewBackgroundScene::sceneUpdate(bool z_up, float scaleValue) {
    if (globalCamera_ == nullptr) {
        auto scene = const_cast<ramses::Scene *>(sceneBackend_->currentScene());
        auto id = scene->getSceneId();
        ramses::RamsesObject* object = scene->findObjectByName("PerspectiveCamera");
        if (object) {
            if (object->getType() == ramses::ERamsesObjectType_PerspectiveCamera) {
                globalCamera_ = static_cast<ramses::PerspectiveCamera*>(object);
            }
        }
    }

    CameraParam_t globalCameraParam;
    ramses::ERotationConvention type;
    globalCamera_->getTranslation(globalCameraParam.translation[0], globalCameraParam.translation[1], globalCameraParam.translation[2]);
    globalCamera_->getRotation(globalCameraParam.rotation[0], globalCameraParam.rotation[1], globalCameraParam.rotation[2], type);
    globalCameraParam.viewport[0] = globalCamera_->getViewportX();
    globalCameraParam.viewport[1] = globalCamera_->getViewportY();
    globalCameraParam.viewport[2] = globalCamera_->getViewportWidth();
    globalCameraParam.viewport[3] = globalCamera_->getViewportHeight();
    globalCameraParam.frustum[0] = globalCamera_->getVerticalFieldOfView();
    globalCameraParam.frustum[1] = globalCamera_->getAspectRatio();
    globalCameraParam.frustum[2] = globalCamera_->getNearPlane();
    globalCameraParam.frustum[3] = globalCamera_->getFarPlane();
    
    if (currentCameraParam_ != globalCameraParam) {
        (*camera_)->setTranslation(globalCameraParam.translation[0], globalCameraParam.translation[1], globalCameraParam.translation[2]);
        (*camera_)->setRotation(globalCameraParam.rotation[0], globalCameraParam.rotation[1], globalCameraParam.rotation[2], type);
        (*camera_)->setViewport(globalCamera_->getViewportX(), globalCamera_->getViewportY(), globalCamera_->getViewportWidth(), globalCamera_->getViewportHeight());
        (*camera_)->setFrustum(globalCamera_->getVerticalFieldOfView(), globalCamera_->getAspectRatio(), globalCamera_->getNearPlane(), globalCamera_->getFarPlane());
        currentCameraParam_ = globalCameraParam;
        update(z_up, scaleValue);
    }
}

void PreviewBackgroundScene::madd_v2_v2v2fl(float r[2], const float a[2], const float b[2], float f) {
    r[0] = a[0] + b[0] * f;
    r[1] = a[1] + b[1] * f;
}

void PreviewBackgroundScene::update(bool z_up, float scaleValue) {
    zUp_ = z_up;
    scaleValue_ = scaleValue;

    int zpos_flag = CLIP_ZNEG | CLIP_ZPOS;
    float planeAxes_data[3];
    planeAxes_data[0] = 1.0;
    if (zUp_) {
        planeAxes_data[1] = 1.0;
        planeAxes_data[2] = 0.0;
    } else {
        planeAxes_data[1] = 0.0;
        planeAxes_data[2] = 1.0;
    }
    ramses::UniformInput planeAxesInput;
	(*appearance_)->getEffect().findUniformInput("planeAxes", planeAxesInput);
    (*appearance_)->setInputValueVector3f(planeAxesInput, planeAxes_data[0], planeAxes_data[1], planeAxes_data[2]);

    ramses::UniformInput gridDistanceInput;
	(*appearance_)->getEffect().findUniformInput("gridDistance", gridDistanceInput);
    (*appearance_)->setInputValueFloat(gridDistanceInput, 500.0f * scaleValue_);

    float gridSize_data[3];
    gridSize_data[0] = 1000.0f;
    gridSize_data[1] = 1000.0f;
    gridSize_data[2] = 1000.0f;
    ramses::UniformInput gridSizeInput;
	(*appearance_)->getEffect().findUniformInput("gridSize", gridSizeInput);
    (*appearance_)->setInputValueVector3f(gridSizeInput, gridSize_data[0], gridSize_data[1], gridSize_data[2]);

    ramses::UniformInput lineKernelInput;
	(*appearance_)->getEffect().findUniformInput("lineKernel", lineKernelInput);
    (*appearance_)->setInputValueFloat(lineKernelInput, 0.5f);

    ramses::UniformInput gridFlagInput;
	(*appearance_)->getEffect().findUniformInput("gridFlag", gridFlagInput);
    if (zUp_) {
        (*appearance_)->setInputValueInt32(gridFlagInput,  (PLANE_XY | SHOW_AXIS_X | SHOW_AXIS_Y | SHOW_GRID));
    } else {
        (*appearance_)->setInputValueInt32(gridFlagInput,  (PLANE_XZ | SHOW_AXIS_X | SHOW_AXIS_Z | SHOW_GRID));
    }

    ramses::UniformInput zoomFactorInput;
	(*appearance_)->getEffect().findUniformInput("zoomFactor", zoomFactorInput);
    float zoom = scaleValue_;
    if (zoom < 4.0) {
		zoom = 4.0;
	}
    (*appearance_)->setInputValueFloat(zoomFactorInput, zoom);

    scene_->flush();
}

void PreviewBackgroundScene::setEnableDisplayGrid(bool enable) {
    if (renderPass_->isEnabled() != enable) {
        renderPass_->setEnabled(enable);
        scene_->flush();
    }
}

}
