#ifndef ASSETS_LOGIC_H
#define ASSETS_LOGIC_H

#include "PropertyData/PropertyData.h"
#include "NodeData/nodeManager.h"
#include "PropertyData/PropertyType.h"
#include "AnimationData/animationData.h"
#include "CurveData/CurveManager.h"
#include "MaterialData/materialManager.h"
#include "signal/SignalProxy.h"
#include "MeshData/MeshDataManager.h"
#include "data_Convert/InputAssets.h"

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
class AssetsLogic : public QObject {
    Q_OBJECT
public:
	bool readProgram2Ptx(std::string filePathStr);

	NodeData* getRoot() {
		return &root_;
	}
	const std::vector<MaterialData>& getMaterialArr() {
		return materialArr;
	}

	
Q_SIGNALS:

private:
	bool isInputError_{false};
	InputPtx inputPtx_;
	NodeData root_;
	std::vector<MaterialData> materialArr;
};

}

#endif // ASSETS_LOGIC_H
