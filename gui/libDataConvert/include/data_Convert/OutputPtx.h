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
}

#endif // OUTPUTPTX_H
