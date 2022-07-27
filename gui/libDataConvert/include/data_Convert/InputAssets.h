#ifndef INPUT_ASSETS_H
#define INPUT_ASSETS_H

#include "PropertyData/PropertyData.h"
#include "NodeData/nodeManager.h"
#include "PropertyData/PropertyType.h"
#include "AnimationData/animationData.h"
#include "CurveData/CurveManager.h"
#include "MaterialData/materialManager.h"
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


using namespace raco::guiData;

namespace raco::dataConvert {
class InputPtx {
public:
	bool parseOneNode(const HmiScenegraph::TNode& tNode, NodeData& nodeData);
	

private:

};

}

#endif // INPUT_ASSETS_H
