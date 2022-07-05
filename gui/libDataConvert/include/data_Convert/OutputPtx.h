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
#include <stdio.h>
#include <string>
#include <iostream>
#include <fstream>
#include <qdir.h>

namespace HmiScenegraph {
class TNode;
class TMesh;
}


using namespace raco::guiData;

namespace raco::dataConvert {

class OutputPtx : public QObject {
    Q_OBJECT
public:
	bool writeProgram2Ptx(QString filePath);
	void writeNodePtx(NodeData* pNode, HmiScenegraph::TNode* parent);
    void setPtxNode(NodeData* childNode, HmiScenegraph::TNode& hmiNode);
    void setPtxTCamera(NodeData* childNode, HmiScenegraph::TNode& hmiNode);
	void setPtxTMesh(NodeData* node, HmiScenegraph::TMesh& mesh);
	void setMeshBaseNode(NodeData* node, HmiScenegraph::TNode* baseNode);
	void setRootSRT(HmiScenegraph::TNode* hmiNode);

Q_SIGNALS:

private:

};

class OutputPtw{
public:
	std::string ConvertAnimationInfo(HmiWidget::TWidget* widget);
	void ConvertCurveInfo(HmiWidget::TWidget* widget, std::string animation_interal);
	void ConvertBind(HmiWidget::TWidget* widget, raco::guiData::NodeData& node);
	void WriteAsset(std::string filePath);

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

#endif // OUTPUTPTX_H
