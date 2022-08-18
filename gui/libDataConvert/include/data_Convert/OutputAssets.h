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
};

class OutputPtw{
public:
	void WriteAsset(std::string filePath);
};
}

#endif // OUTPUT_ASSETS_H
