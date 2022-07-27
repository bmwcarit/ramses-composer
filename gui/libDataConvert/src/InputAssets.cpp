#include "data_Convert/InputAssets.h"
#include "data_Convert/ProgramDefine.h"
#include "PropertyData/PropertyType.h"



namespace raco::dataConvert {

bool InputPtx::parseOneNode(const HmiScenegraph::TNode& tNode, NodeData& nodeData) {
	nodeData.setName(tNode.name());

	TVector3f tScale = tNode.scale();
	Vec3 scaleData{tScale.x(), tScale.y(), tScale.z()};
	nodeData.insertSystemData("scale", scaleData);

	TVector3f tRota = tNode.rotation();
	Vec3 rotaData{tRota.x(), tRota.y(), tRota.z()};
	nodeData.insertSystemData("rotation", rotaData);

	TVector3f tTran = tNode.translation();
	Vec3 tranData{tTran.x(), tTran.y(), tTran.z()};
	nodeData.insertSystemData("translation", tranData);

	
	return true;
}


}  // namespace raco::dataConvert
