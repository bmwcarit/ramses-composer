#ifndef OUTPUT_ASSETS_H
#define OUTPUT_ASSETS_H

#include <QDebug>
#include <QFile>
#include <QDir>
#include <QThread>
#include <mutex>

#include "PropertyData/PropertyData.h"
#include "NodeData/nodeManager.h"
#include "PropertyData/PropertyType.h"
#include "AnimationData/animationData.h"
#include "CurveData/CurveManager.h"
#include "MaterialData/materialManager.h"
#include "signal/SignalProxy.h"
#include "MeshData/MeshDataManager.h"

#include "proto/Numeric.pb.h"
#include "proto/Common.pb.h"
#include "proto/Scenegraph.pb.h"
#include "proto/HmiWidget.pb.h"
#include "proto/HmiBase.pb.h"
#include <google/protobuf/text_format.h>

#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <qdir.h>

using namespace raco::guiData;

namespace raco::dataConvert {

struct NodeWithMaterial {
	std::string nodeTextureName;
	MaterialData material;
};

class OutputPtx : public QObject {
    Q_OBJECT
public:
	bool writeProgram2Ptx(std::string filePath, QString relativePath);
	void writeNodePtx(NodeData* pNode, HmiScenegraph::TNode* parent);
	void setMaterialTextureByNodeUniforms(NodeData* childNode, MaterialData& materialData);
    void setPtxNode(NodeData* childNode, HmiScenegraph::TNode& hmiNode);
    void setPtxTCamera(NodeData* childNode, HmiScenegraph::TNode& hmiNode);
	void setPtxTMesh(NodeData* node, HmiScenegraph::TMesh& mesh);
	void setMeshBaseNode(NodeData* node, HmiScenegraph::TNode* baseNode);
	void setRootSRT(HmiScenegraph::TNode* hmiNode);
	std::string delNodeNameSuffix(std::string nodeName);
	TECompareFunction matchCompareFunction(DepthCompare depthCmp);
	TEBlendFactor matchBlendFactor(BlendFactor blendFactor);
	TEBlendOperation matchBlendOperation(BlendOperation blendOpera);
	TEFace matchFaceCulling(Culling cull);
	TEWinding matchWinding(WindingType wind);
	void setMaterialDefaultRenderMode(RenderMode& renderMode, HmiScenegraph::TRenderMode* rRenderMode);
	void setMaterialRenderMode(RenderMode& renderMode, HmiScenegraph::TRenderMode* rRenderMode);
	void uniformTypeValue(Uniform data, HmiScenegraph::TUniform& tUniform);
	TETextureFilter matchFilter(Filter filter);
	TETextureWrapMode matchWrapMode(WrapMode mode);
	bool mkdir(QString path);
	bool isStored(std::string name, std::set<std::string>& nameArr);
	std::string getShaderPtxNameByShaderName(std::string name);
	void messageBoxError(std::string materialName);
	void writeBitmap2MaterialLib(QString& filePath, QString& oldPath, HmiScenegraph::TMaterialLib* materialLibrary);
	void writeShaders2MaterialLib(QString& filePath, QString& oldPath, HmiScenegraph::TMaterialLib* materialLibrary);
	void writeMaterial2MaterialLib(HmiScenegraph::TMaterialLib* materialLibrary);
	void writeMaterialLib2Ptx(QString& filePath, QString& oldPath, HmiScenegraph::TMaterialLib* materialLibrary);
Q_SIGNALS:

private:
	bool isOutputError_{false};
	std::vector<NodeWithMaterial> nodeWithMaterial_;
};

class OutputPtw{
public:
	std::string ConvertAnimationInfo(HmiWidget::TWidget* widget);
	void ConvertCurveInfo(HmiWidget::TWidget* widget, std::string animation_interal);
	void ConvertBind(HmiWidget::TWidget* widget, raco::guiData::NodeData& node);
	void WriteAsset(std::string filePath);
	void WriteBasicInfo(HmiWidget::TWidget* widget);

private:
	void ModifyTranslation(std::pair<std::string, std::string> curveProP, HmiWidget::TNodeTransform* transform);
	void CreateTranslation(std::pair<std::string, std::string> curveProP, HmiWidget::TNodeTransform* transform, raco::guiData::NodeData node);

	void ModifyScale(std::pair<std::string, std::string> curveProP, HmiWidget::TNodeTransform* transform);
	void CreateScale(std::pair<std::string, std::string> curveProP, HmiWidget::TNodeTransform* transform, raco::guiData::NodeData node);

	void ModifyRotation(std::pair<std::string, std::string> curveProP, HmiWidget::TNodeTransform* transform);
	void CreateRotation(std::pair<std::string, std::string> curveProP, HmiWidget::TNodeTransform* transform, raco::guiData::NodeData node);

	void AddUniform(std::pair<std::string, std::string> curveProP, HmiWidget::TNodeParam* nodeParam);
};
}

#endif // OUTPUT_ASSETS_H
