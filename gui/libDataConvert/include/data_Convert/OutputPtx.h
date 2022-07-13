#ifndef OUTPUTPTX_H
#define OUTPUTPTX_H

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

using namespace raco::guiData;

namespace raco::dataConvert {
class OutputPtx : public QObject {
    Q_OBJECT
public:
	bool writeProgram2Ptx(std::string filePath, QString relativePath);
	void writeNodePtx(NodeData* pNode, HmiScenegraph::TNode* parent);
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
};
}

#endif // OUTPUTPTX_H
