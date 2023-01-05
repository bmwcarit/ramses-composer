
#include "data_Convert/OutputAssets.h"
#include "data_Convert/ProgramDefine.h"
#include "PropertyData/PropertyType.h"
#include "style/Icons.h"

#include <set>
#include <stack>
#include <QMessageBox>
#include <cmath>

namespace raco::dataConvert {
using namespace raco::style;

// A curve is bound by multiple animations.
//std::map<std::string, std::set<std::string>> curveNameAnimation_;
//        curveName ,   animation1, animation2, animation3...
std::map<std::string, std::map<std::string, std::vector<AnimationsSingleCurve>>> curveNameAnimations_;
	//        curveName  ,          [Animation,      [nodeID,   defaultData]]
// A property of a node is bound by different curves of different animations.
std::map<std::string, std::vector<std::map<std::string, std::vector<std::map<std::string, CurvesSingleProp>>>>> pNodePropCurveNames_;
//        pNodeID ,	          <   propertyName,  [ < Animation, CurvesSingleProp > ]	>

std::map<std::string, std::set<int>> NodeIDUColorNums_;
//			  NodeID , ucolors

std::set<int> uColorNums_;

std::set<std::string> HasOpacityMaterials_;

float NodeScaleSize_;

std::string SceneChildName_;

void DEBUG(QString FILE, QString FUNCTION, int LINE, QString msg) {
	QMessageBox msgBox;
	msgBox.setWindowTitle("Debug message box");
	QPushButton* okButton = msgBox.addButton("OK", QMessageBox::ActionRole);
	msgBox.setIcon(QMessageBox::Icon::Warning);

	QString info;
	info += FILE + QString(" ") + QString::number(LINE) + QString(" ") + FUNCTION;
	info += ": " + msg;
	//msgBox.setWindowTitle("OutputAssets.cpp Line：emplace error " + info);
	msgBox.setText(info);
	msgBox.exec();
}

void OutputPtx::isNotAddedAttribute(std::string name) {
	if (name == "a_Position" || name == "a_TextureCoordinate" || name == "a_TextureCoordinate1") {
		return;
	}

	QMessageBox msgBox;
	msgBox.setWindowTitle("Attribute Not added message box");
	QPushButton* okButton = msgBox.addButton("OK", QMessageBox::ActionRole);
	msgBox.setIcon(QMessageBox::Icon::Warning);

	QString info;
	// The currently used attribute has not been added yet!
	info = QString::fromStdString(name) + "\" has not been added yet!";
	info = "Warning: The currently used attribute \"" + info;
	msgBox.setText(info);
	msgBox.exec();

	if (msgBox.clickedButton() == (QAbstractButton*)(okButton)) {
		isPtxOutputError_ = true;
	}
}

std::string delUniformNamePrefix(std::string nodeName) {
	int index = nodeName.rfind("uniforms.");
	if (-1 != index) {
		nodeName = nodeName.substr(9, nodeName.length());
	}
	return nodeName;
}

void OutputPtx::setMeshBaseNode(NodeData* node, HmiScenegraph::TNode* baseNode) {
	std::string nodeName = node->getName();
	std::string baseNodeName = nodeName + "Shape";
	baseNode->set_name(baseNodeName);

	if (node->hasSystemData("scale")) {
		TVector3f* scale = new TVector3f();
		Vec3 scal = std::any_cast<Vec3>(node->getSystemData("scale"));
		scale->set_x(scal.x);
		scale->set_y(scal.y);
		scale->set_z(scal.z);
		baseNode->set_allocated_scale(scale);
	}
	if (node->hasSystemData("rotation")) {
		TVector3f* rotation = new TVector3f();
		Vec3 rota = std::any_cast<Vec3>(node->getSystemData("rotation"));
		rotation->set_x(rota.x);
		rotation->set_y(rota.y);
		rotation->set_z(rota.z);
		baseNode->set_allocated_rotation(rotation);
	}
	if (node->hasSystemData("translation")) {
		TVector3f* translation = new TVector3f();
		Vec3 tran = std::any_cast<Vec3>(node->getSystemData("translation"));
		translation->set_x(tran.x);
		translation->set_y(tran.y);
		translation->set_z(tran.z);
		baseNode->set_allocated_translation(translation);
	}
}

bool uniformCompare(Uniform data, Uniform myUni) {
	bool result = false;
	UniformType dataType = data.getType();
	switch (dataType) {
		case raco::guiData::Null:
			// Do not have
			break;
		case raco::guiData::Bool: {
			uint32_t detemp = std::any_cast<bool>(data.getValue());
			uint32_t mytemp = std::any_cast<bool>(myUni.getValue());
			if (detemp == mytemp) {
				result = true;
			}
			break;
		}
		case raco::guiData::Int: {
			int detemp = std::any_cast<int>(data.getValue());
			int mytemp = std::any_cast<int>(myUni.getValue());
			if (detemp == mytemp) {
				result = true;
			}

			break;
		}
		case raco::guiData::Double: {
			float temp = std::any_cast<double>(data.getValue());
			float mytemp = std::any_cast<double>(myUni.getValue());
			if (temp == mytemp) {
				result = true;
			}
			break;
		}
		case raco::guiData::String:
			// Do not have
			break;
		case raco::guiData::Ref:
			// Do not have
			break;
		case raco::guiData::Table:
			// Do not have
			break;
		case raco::guiData::Vec2f: {
			Vec2 temp = std::any_cast<Vec2>(data.getValue());
			Vec2 mytemp = std::any_cast<Vec2>(myUni.getValue());
			if (temp.x == mytemp.x && temp.y == mytemp.y) {
				result = true;
			}
			break;
		}
		case raco::guiData::Vec3f: {
			Vec3 temp = std::any_cast<Vec3>(data.getValue());
			Vec3 mytemp = std::any_cast<Vec3>(myUni.getValue());
			if (temp.x == mytemp.x && temp.y == mytemp.y && temp.z == mytemp.z) {
				result = true;
			}
			break;
		}
		case raco::guiData::Vec4f: {
			Vec4 temp = std::any_cast<Vec4>(data.getValue());
			Vec4 mytemp = std::any_cast<Vec4>(myUni.getValue());
			if (temp.x == mytemp.x && temp.y == mytemp.y && temp.z == mytemp.z && temp.w == mytemp.w) {
				result = true;
			}
			break;
		}
		case raco::guiData::Vec2i: {
			Vec2int temp = std::any_cast<Vec2int>(data.getValue());
			Vec2int mytemp = std::any_cast<Vec2int>(myUni.getValue());
			if (temp.x == mytemp.x && temp.y == mytemp.y) {
				result = true;
			}
			break;
		}
		case raco::guiData::Vec3i: {
			Vec3int temp = std::any_cast<Vec3int>(data.getValue());
			Vec3int mytemp = std::any_cast<Vec3int>(myUni.getValue());
			if (temp.x == mytemp.x && temp.y == mytemp.y && temp.z == mytemp.z) {
				result = true;
			}
			break;
		}
		case raco::guiData::Vec4i: {
			Vec4int temp = std::any_cast<Vec4int>(data.getValue());
			Vec4int mytemp = std::any_cast<Vec4int>(myUni.getValue());
			if (temp.x == mytemp.x && temp.y == mytemp.y && temp.z == mytemp.z && temp.w == mytemp.w) {
				result = true;
			}
			break;
		}
		case raco::guiData::Struct:
			// Do not have
			break;
		default:
			break;
	}
	return result;
}

bool Vec4Equal(Vec4 left, Vec4 right) {
	if (std::abs(left.x - right.x) < 0.00001 && std::abs(left.y - right.y) < 0.00001
		&& std::abs(left.z - right.z) < 0.00001 &&  std::abs(left.w - right.w) < 0.00001) {
		return true;
	}
	return false;
}

bool isCorrectUColor(int index, std::any value) {
	Vec4 u_colorX = std::any_cast<Vec4>(value);
	Vec4 u_color1 = {1, 0.960784316, 0.909803927, 1};
	Vec4 u_color2 = {1, 0.850980401, 0.65882355, 1};
	Vec4 u_color3 = {0.75686276, 0.631372571, 0.443137258, 1};
	Vec4 u_color4 = {0.541176498, 0.356862754, 0.376470596, 1};
	Vec4 u_color5 = {0.16862746, 0.22352943, 0.32549021, 1};
	Vec4 u_color6 = {0.0, 0.0, 0.0, 1};
	switch (index) {
		case 1:
			return Vec4Equal(u_colorX, u_color1);
		case 2:
			return Vec4Equal(u_colorX, u_color2);
		case 3:
			return Vec4Equal(u_colorX, u_color3);
		case 4:
			return Vec4Equal(u_colorX, u_color4);
		case 5:
			return Vec4Equal(u_colorX, u_color5);
		case 6:
			return Vec4Equal(u_colorX, u_color6);
		default:
			DEBUG(__FILE__, __FUNCTION__, __LINE__, "u_color's index is error!");
	}
	return false;
}

void updateNodeIDUColors(std::string NodeID, std::vector<Uniform> uniforms) {
	for (auto& un : uniforms) {
		int index = un.getName().rfind("u_color");
		if (-1 != index) {
			std::string str = un.getName().substr(7, un.getName().length());
			int n = std::stoi(str);
			if (!isCorrectUColor(n, un.getValue())) {
				std::string info = "u_color" + str + "'s value is error!";
				DEBUG(__FILE__, __FUNCTION__, __LINE__, "u_color's value is error!");
			}
			uColorNums_.emplace(n);
			auto it = NodeIDUColorNums_.find(NodeID);
			if (it != NodeIDUColorNums_.end()) {
				it->second.emplace(n);
			} else {
				std::set<int> uColorNums;
				uColorNums.emplace(n);
				NodeIDUColorNums_.emplace(NodeID, uColorNums);
			}
		}
	}
}

bool OutputPtx::isEqualUniform(std::vector<Uniform> publicUniforms, Uniform privateUniform) {
	for (auto un : publicUniforms) {
		if (un.getName() == privateUniform.getName()) {
			return uniformCompare(un, privateUniform);
		}
	}
	DEBUG(__FILE__, __FUNCTION__, __LINE__, "private Uniform is out of range from public Uniforms!");
	return false;
}

void OutputPtx::setPtxTMesh(NodeData* node, HmiScenegraph::TMesh& tMesh) {
	// set baseNode data
	HmiScenegraph::TNode* baseNode = new HmiScenegraph::TNode();
	setMeshBaseNode(node, baseNode);
	tMesh.set_allocated_basenode(baseNode);

	MaterialData materialData;
	NodeMaterial nodeMaterial;
	MeshData meshData;

	if(raco::guiData::MeshDataManager::GetInstance().getMeshData(node->objectID(), meshData)){
		//setMeshUniform(node, meshData);
		// set meshresource
		tMesh.set_meshresource(meshData.getMeshUri());
		// usedAttributes
		if (raco::guiData::MaterialManager::GetInstance().getMaterialData(node->objectID(), materialData)) {
			for (auto& attr : materialData.getUsedAttributes()) {
				isNotAddedAttribute(attr.name);
				HmiScenegraph::TMesh_TAttributeParamteter tempAttr;
				tempAttr.set_name(attr.name);
				HmiScenegraph::TMesh_TAttributeParamteter* itAttr = tMesh.add_attributeparameter();
				*itAttr = tempAttr;
			}
			// if node has material data，so it has nodeMaterial
			raco::guiData::MaterialManager::GetInstance().getNodeMaterial(node->objectID(), nodeMaterial);
			// set material reference name
			tMesh.set_materialreference(materialData.getObjectName());

			// update map:: NodeIDUColors
			updateNodeIDUColors(node->objectID(), materialData.getUniforms());

			// uniforms for mesh
			if (nodeMaterial.isPrivate()) {
				for (auto& uniform : nodeMaterial.getUniforms()) {
					if (!isEqualUniform(materialData.getUniforms(), uniform)) {
						HmiScenegraph::TUniform tUniform;
						uniformTypeValue(uniform, tUniform);
						HmiScenegraph::TUniform* itMesh = tMesh.add_uniform();
						*itMesh = tUniform;
					}
				}
			}
		}
	}
}

void OutputPtx::setPtxTCamera(NodeData* childNode, HmiScenegraph::TNode& hmiNode) {
    // camera
	Q_UNUSED(childNode);
	HmiScenegraph::TCamera* camera = new HmiScenegraph::TCamera();
	camera->set_horizontalfov(0.7);
	camera->set_aspectratio(1.0);
	camera->set_nearplane(0.01);
	camera->set_farplane(100.0); 
	camera->set_projectiontype(HmiScenegraph::TECameraProjectionType::TECameraProjectionType_FOV);
	hmiNode.set_allocated_camera(camera);
}

// update ptx node
void OutputPtx::setPtxNode(NodeData* childNode, HmiScenegraph::TNode& hmiNode) {
    std::string nodeName = childNode->getName();
	hmiNode.set_name(nodeName);
	MeshData meshData;
	bool isMeshNode = raco::guiData::MeshDataManager::GetInstance().getMeshData(childNode->objectID(), meshData);

	if (!isMeshNode) {
		if (childNode->hasSystemData("scale")) {
			TVector3f* scale = new TVector3f();
			Vec3 scal = std::any_cast<Vec3>(childNode->getSystemData("scale"));
			scale->set_x(scal.x);
			scale->set_y(scal.y);
			scale->set_z(scal.z);
			if (nodeName == SceneChildName_) {
				NodeScaleSize_ = scal.x;
			}
			hmiNode.set_allocated_scale(scale);
		}
		if (childNode->hasSystemData("rotation")) {
			TVector3f* rotation = new TVector3f();
			Vec3 rota = std::any_cast<Vec3>(childNode->getSystemData("rotation"));
			rotation->set_x(rota.x);
			rotation->set_y(rota.y);
			rotation->set_z(rota.z);
			hmiNode.set_allocated_rotation(rotation);
		}
		if (childNode->hasSystemData("translation")) {
			TVector3f* translation = new TVector3f();
			Vec3 tran = std::any_cast<Vec3>(childNode->getSystemData("translation"));
			translation->set_x(tran.x);
			translation->set_y(tran.y);
			translation->set_z(tran.z);
			hmiNode.set_allocated_translation(translation);
		}
	}else {// meshNode use default data
		TVector3f* scale = new TVector3f();
		scale->set_x(1.0);
		scale->set_y(1.0);
		scale->set_z(1.0);
		hmiNode.set_allocated_scale(scale);

		TVector3f* rotation = new TVector3f();
		rotation->set_x(0.0);
		rotation->set_y(0.0);
		rotation->set_z(0.0);
		hmiNode.set_allocated_rotation(rotation);

		TVector3f* translation = new TVector3f();
		translation->set_x(0.0);
		translation->set_y(0.0);
		translation->set_z(0.0);
		hmiNode.set_allocated_translation(translation);
	}

    // renderorder and childSortOrderRank
    hmiNode.set_renderorder(0);
	hmiNode.set_childsortorderrank(0);

	//MaterialData materialData;
	//if (raco::guiData::MaterialManager::GetInstance().getMaterialData(childNode->objectID(), materialData)) {
	//	setMaterialTextureByNodeUniforms(childNode, materialData);
	//	raco::guiData::MaterialManager::GetInstance().deleteMateialData(childNode->objectID());
	//	raco::guiData::MaterialManager::GetInstance().addMaterialData(childNode->objectID(), materialData);
	//}

    // mesh
	if (isMeshNode) {
		HmiScenegraph::TMesh mesh;
		setPtxTMesh(childNode, mesh);
		HmiScenegraph::TMesh* it = hmiNode.add_mesh();
		*it = mesh;
	}
}

void OutputPtx::setRootSRT(HmiScenegraph::TNode* hmiNode) {
	// scale
	TVector3f* scale = new TVector3f();
	scale->set_x(1.0);
	scale->set_y(1.0);
	scale->set_z(1.0);
	hmiNode->set_allocated_scale(scale);
	// rotation
	TVector3f* rotation = new TVector3f();
	rotation->set_x(0.0);
	rotation->set_y(0.0);
	rotation->set_z(0.0);
	hmiNode->set_allocated_rotation(rotation);
	// translation
	TVector3f* translation = new TVector3f();
	translation->set_x(0.0);
	translation->set_y(0.0);
	translation->set_z(0.0);
	hmiNode->set_allocated_translation(translation);
}


bool isUniformAndIndex(std::string propName, int& index) {
	std::string subLast = propName.substr(propName.length() - 2, propName.length());
	index = -1;
	if (subLast == ".x") {
		index = 0;
	} else if (subLast == ".y") {
		index = 1;
	} else if (subLast == ".z") {
		index = 2;
	} else if (subLast == ".w") {
		index = 3;
	}
	int r = propName.rfind("uniforms.");
	if (-1 == r) {
		return false;
	} else {
		return true;
	}
}


float getUniformValueByType(Uniform data, int index) {
	UniformType type = data.getType();
	switch (type) {
		case raco::guiData::Vec2f: {
			Vec2 temp = std::any_cast<Vec2>(data.getValue());
			switch (index) {
				case 0:
					return temp.x;
				case 1:
					return temp.y;
				default:
					return 0;
			}
		}
		case raco::guiData::Vec3f: {
			Vec3 temp = std::any_cast<Vec3>(data.getValue());
			switch (index) {
				case 0:
					return temp.x;
				case 1:
					return temp.y;
				case 2:
					return temp.z;
				default:
					return 0;
			}
		}
		case raco::guiData::Vec4f: {
			Vec4 temp = std::any_cast<Vec4>(data.getValue());
			switch (index) {
				case 0:
					return temp.x;
				case 1:
					return temp.y;
				case 2:
					return temp.z;
				case 3:
					return temp.w;
				default:
					return 0;
			}
		}
		case raco::guiData::Vec2i: {
			Vec2int temp = std::any_cast<Vec2int>(data.getValue());
			switch (index) {
				case 0:
					return temp.x;
				case 1:
					return temp.y;
				default:
					return 0;
			}
		}
		case raco::guiData::Vec3i: {
			Vec3int temp = std::any_cast<Vec3int>(data.getValue());
			switch (index) {
				case 0:
					return temp.x;
				case 1:
					return temp.y;
				case 2:
					return temp.z;
				default:
					return 0;
			}
		}
		case raco::guiData::Vec4i: {
			Vec4int temp = std::any_cast<Vec4int>(data.getValue());
			switch (index) {
				case 0:
					return temp.x;
				case 1:
					return temp.y;
				case 2:
					return temp.z;
				case 3:
					return temp.w;
				default:
					return 0;
			}
		}
	}
	return 0;
}

float setCurvesPropData(NodeData* pNode, std::string propName) {
	int index;
	bool isUniform = isUniformAndIndex(propName, index);
	if (index != -1 && !isUniform) {
		propName = propName.substr(0, propName.length()-2);
		auto systemProp = pNode->systemDataMapNewRef();
		auto it = systemProp.find(propName);
		if (it != systemProp.end()) {
			Vec3 valueVec = std::any_cast<Vec3>(pNode->getSystemData(propName));
			if (index == 0) {
				return valueVec.x;
			} else if (index == 1) {
				return valueVec.y;
			} else if (index == 2) {
				return valueVec.z;
			} else {
				return 0;
			}
		}
	} 
	if (isUniform) {
		NodeMaterial nodeMaterial;
		raco::guiData::MaterialManager::GetInstance().getNodeMaterial(pNode->objectID(), nodeMaterial);
		propName = delUniformNamePrefix(propName);
		if (index == -1) {
			for (auto un : nodeMaterial.getUniforms()) {
				if (un.getName() == propName) {
					return std::any_cast<double>(un.getValue());
				}
			}
		} else {
			for (auto un : nodeMaterial.getUniforms()) {
				if (un.getName() == propName.substr(0, propName.length() - 2)) {
					return getUniformValueByType(un, index);
				}
			}
		}
	} 
	return 0;
}

void searchRepeatPropInNode(NodeData* pNode, std::string name) {
	auto bindyMap = pNode->NodeExtendRef().curveBindingRef().bindingMap();
	
	std::vector<std::map<std::string, CurvesSingleProp>> curvesName;
	for (auto animation : bindyMap) {
		for (auto prop : animation.second) {
			std::string propName = prop.first;
			if (propName == name) {
				std::map<std::string, CurvesSingleProp> aniProp;
				CurvesSingleProp curveSingleProp;
				curveSingleProp.curveName = prop.second;
				//curveSingleProp.defaultData = setCurvesPropData(pNode, name);
				aniProp.emplace(animation.first, curveSingleProp);
				curvesName.push_back(aniProp);
			}
		}
	}

	if (curvesName.size() > 1) {
		std::vector<std::map<std::string, std::vector<std::map<std::string, CurvesSingleProp>>>> propCurves;
		std::map<std::string, std::vector<std::map<std::string, CurvesSingleProp>>> propCurve;
		double defultData = setCurvesPropData(pNode, name);
		for (int i = 0; i < curvesName.size(); ++i) {
			curvesName[i].begin()->second.defaultData = defultData;
		}
		auto ret = propCurve.emplace(name, curvesName);
		bool a = ret.second;
		propCurves.push_back(propCurve);
		std::string ID = pNode->objectID();
		auto nodeIt = pNodePropCurveNames_.find(ID);
		if (nodeIt != pNodePropCurveNames_.end()) {
			nodeIt->second.push_back(propCurve);
		}
		else {
			if (!pNodePropCurveNames_.emplace(ID, propCurves).second) {
				DEBUG(__FILE__, __FUNCTION__, __LINE__, "pNodePropCurveNames_ emplace failed!");
			}
		}
	}
}

void updatePNodePropCurveMap(NodeData* pNode) {
	std::set<std::string> peopertys;
	const std::map<std::string, std::map<std::string, std::string>>& bindyMap = pNode->NodeExtendRef().curveBindingRef().bindingMap();
	for (auto animation : bindyMap) {
		for (auto prop : animation.second) {
			std::string propName = prop.first;
			auto it = peopertys.find(propName);
			if (it == peopertys.end()) {
				peopertys.insert(propName);
				searchRepeatPropInNode(pNode, propName);
			}

		}
	}
}

void updateCurveAnimationMap(NodeData* pNode) {
	const std::map<std::string, std::map<std::string, std::string>>& bindyMap = pNode->NodeExtendRef().curveBindingRef().bindingMap();
	for (auto animation : bindyMap) {
		for (auto curve : animation.second) {
			auto it = curveNameAnimations_.find(curve.second);
			std::map<std::string, std::vector<AnimationsSingleCurve>> animations;
			std::vector<AnimationsSingleCurve> datas;
			AnimationsSingleCurve data;
			if (it == curveNameAnimations_.end()) {
				data.pNode = pNode;
				data.property = curve.first;
				datas.push_back(data);
				if (!animations.emplace(animation.first, datas).second) {
					DEBUG(__FILE__, __FUNCTION__, __LINE__, "animations emplace failed!");
				}
				if (!curveNameAnimations_.emplace(curve.second, animations).second) {
					DEBUG(__FILE__, __FUNCTION__, __LINE__, "curveNameAnimations_ emplace failed!");
				}
			} else {
				data.pNode = pNode;
				data.property = curve.first;
				// If the same curve is bound to the same node of different animations, it can only be added once. Step 4
				/*for (auto ani : it->second) {
					for (auto nodeTemp : ani.second) {
						if (nodeTemp.pNode->objectID() == pNode->objectID()) {
							return;
						}
					}
				}*/
				auto iter = it->second.find(animation.first);
				if (iter != it->second.end()
					&& (iter->second.at(iter->second.size() - 1)).pNode->objectID() != pNode->objectID()) {  // if animation is found
					iter->second.push_back(data);
				} else { // no animation found
					std::vector<AnimationsSingleCurve> datas;
					datas.push_back(data);
					if (!it->second.emplace(animation.first, datas).second) {
						DEBUG(__FILE__, __FUNCTION__, __LINE__, "it->second emplace failed!");
					}
				}
			}
		}
	}
}

void cullingData() {
	for (auto it = curveNameAnimations_.begin(); it != curveNameAnimations_.end();) {
		if (it->second.size() < 2) {
			curveNameAnimations_.erase(it++);
		} else {
			it++;
		}
	}
}

void OutputPtx::writeNodePtx(NodeData* pNode, HmiScenegraph::TNode* parent) {
	if (!pNode){
		return;
	}
	HmiScenegraph::TNode hmiNode;
	setPtxNode(pNode, hmiNode);
	updateCurveAnimationMap(pNode);
	updatePNodePropCurveMap(pNode);
	HmiScenegraph::TNode* it = parent->add_child();
	*it = hmiNode;
	parent = const_cast<HmiScenegraph::TNode*>(&(parent->child(parent->child_size() - 1)));
	
	for (auto it = pNode->childMapRef().begin(); it != pNode->childMapRef().end(); ++it) {
		writeNodePtx(&(it->second), parent);
	}
}
TECompareFunction OutputPtx::matchCompareFunction(DepthCompare depthCmp) {
	TECompareFunction result = TECompareFunction::TECompareFunction_Never;
	switch (depthCmp) {
		case raco::guiData::DC_Disabled:
			result = TECompareFunction::TECompareFunction_Never;
			break;
		case raco::guiData::DC_GreaterThan:
			result = TECompareFunction::TECompareFunction_Greater;
			break;
		case raco::guiData::DC_GreaterOrEqualTo:
			result = TECompareFunction::TECompareFunction_Equal;
			break;
		case raco::guiData::DC_LessThan:
			result = TECompareFunction::TECompareFunction_Less;
			break;
		case raco::guiData::DC_LessThanOrEqualTo:
			result = TECompareFunction::TECompareFunction_LessEqual;
			break;
		case raco::guiData::DC_Equal:
			result = TECompareFunction::TECompareFunction_Equal;
			break;
		case raco::guiData::DC_NotEqual:
			result = TECompareFunction::TECompareFunction_NotEqual;
			break;
		case raco::guiData::DC_True:
			result = TECompareFunction::TECompareFunction_Always;
			break;
		case raco::guiData::DC_False:
			result = TECompareFunction::TECompareFunction_Never;
			break;
		default:
			break;
	}
	return result;
}

TEBlendFactor OutputPtx::matchBlendFactor(BlendFactor blendFactor) {
	TEBlendFactor result = TEBlendFactor::TEBlendFactor_Zero;
	switch (blendFactor) {
		case raco::guiData::Zero:
			result = TEBlendFactor::TEBlendFactor_Zero;
			break;
		case raco::guiData::One:
			result = TEBlendFactor::TEBlendFactor_One;
			break;
		case raco::guiData::SrcAlpha:
			result = TEBlendFactor::TEBlendFactor_SourceAlpha;
			break;
		case raco::guiData::OneMinusSrcAlpha:
			result = TEBlendFactor::TEBlendFactor_InverseSourceAlpha;
			break;
		case raco::guiData::DstAlpha:
			result = TEBlendFactor::TEBlendFactor_DestinationAlpha;
			break;
		case raco::guiData::OneMinusDstAlpha:
			result = TEBlendFactor::TEBlendFactor_InverseDestinationAlpha;
			break;
		case raco::guiData::SrcColor:
			result = TEBlendFactor::TEBlendFactor_SourceColor;
			break;
		case raco::guiData::OneMinusSrcColor:
			result = TEBlendFactor::TEBlendFactor_InverseSourceColor;
			break;
		case raco::guiData::DstColor:
			result = TEBlendFactor::TEBlendFactor_DestinationColor;
			break;
		case raco::guiData::OneMinusDstColor:
			result = TEBlendFactor::TEBlendFactor_InverseDestinationColor;
			break;
		case raco::guiData::ConstColor:
			result = TEBlendFactor::TEBlendFactor_ConstantColor;
			break;
		case raco::guiData::OneMinusConstColor:
			result = TEBlendFactor::TEBlendFactor_InverseConstantColor;
			break;
		case raco::guiData::ConstAlpha:
			result = TEBlendFactor::TEBlendFactor_ConstantAlpha;
			break;
		case raco::guiData::OneMinusConstAlpha:
			result = TEBlendFactor::TEBlendFactor_InverseConstantAlpha;
			break;
		case raco::guiData::AlphaSaturated:
			result = TEBlendFactor::TEBlendFactor_SourceAlphaSaturate;
			break;
		default:
			break;
	}
	return result;
}
TEBlendOperation OutputPtx::matchBlendOperation(BlendOperation blendOpera) {
	TEBlendOperation result = TEBlendOperation::TEBlendOperation_None;
	switch (blendOpera) {
		case raco::guiData::BO_None:
			result = TEBlendOperation::TEBlendOperation_None;
			break;
		case raco::guiData::BO_Add:
			result = TEBlendOperation::TEBlendOperation_Add;
			break;
		case raco::guiData::BO_Subtract:
			result = TEBlendOperation::TEBlendOperation_Subtract;
			break;
		case raco::guiData::BO_ReverseSub:
			result = TEBlendOperation::TEBlendOperation_ReverseSubtract;
			break;
		case raco::guiData::BO_Min:
			result = TEBlendOperation::TEBlendOperation_None;
			break;
		case raco::guiData::BO_Max:
			result = TEBlendOperation::TEBlendOperation_None;
			break;
		default:
			break;
	}
	return result;
}

TEFace OutputPtx::matchFaceCulling(Culling cull) {
	TEFace result = TEFace::TEFace_None;
	switch (cull) {
		case raco::guiData::CU_Front:
			result = TEFace::TEFace_Front;
			break;
		case raco::guiData::CU_Back:
			result = TEFace::TEFace_Back;
			break;
		case raco::guiData::CU_FrontAndBack:
			result = TEFace::TEFace_FrontAndBack;
			break;
		case raco::guiData::CU_None:
			result = TEFace::TEFace_None;
			break;
		default:
			break;
	}
	return result;
}

TEWinding OutputPtx::matchWinding(WindingType wind) {
	TEWinding result = TEWinding::TEWinding_ClockWise;
	switch (wind) {
		case raco::guiData::M_TEWinding_ClockWise:
			result = TEWinding::TEWinding_ClockWise;
			break;
		case raco::guiData::M_TEWinding_CounterClockWise:
			result = TEWinding::TEWinding_CounterClockWise;
			break;
		default:
			break;
	}
	return result;
}

void OutputPtx::setMaterialDefaultRenderMode(RenderMode& renderMode, HmiScenegraph::TRenderMode* rRenderMode) {
	// winding and culling
	rRenderMode->set_winding(TEWinding::TEWinding_CounterClockWise);
	rRenderMode->set_culling(TEFace::TEFace_None);
	// tblending
	HmiScenegraph::TBlendMode* tblending = new HmiScenegraph::TBlendMode();
	Blending blending = renderMode.getBlending();
	// operation
	tblending->set_blendoperationcolor(TEBlendOperation::TEBlendOperation_Add);
	tblending->set_blendoperationalpha(TEBlendOperation::TEBlendOperation_Add);
	// factor
	tblending->set_sourcealphafactor(TEBlendFactor::TEBlendFactor_One);
	tblending->set_sourcecolorfactor(TEBlendFactor::TEBlendFactor_SourceAlpha);
	tblending->set_destinationalphafactor(TEBlendFactor::TEBlendFactor_InverseSourceAlpha);
	tblending->set_destinationcolorfactor(TEBlendFactor::TEBlendFactor_InverseSourceAlpha);

	rRenderMode->set_allocated_blending(tblending);
	rRenderMode->set_depthcompare(TECompareFunction::TECompareFunction_Always);

	rRenderMode->set_depthwrite(false);
	// ColorWrite
	HmiScenegraph::TRenderMode_TColorWrite* tColorWrite = new HmiScenegraph::TRenderMode_TColorWrite();
	ColorWrite colorWrite = renderMode.getColorWrite();
	tColorWrite->set_alpha(true);
	tColorWrite->set_blue(true);
	tColorWrite->set_green(true);
	tColorWrite->set_red(true);
	rRenderMode->set_allocated_colorwrite(tColorWrite);
}

void OutputPtx::setMaterialRenderMode(RenderMode& renderMode, HmiScenegraph::TRenderMode* rRenderMode) {
	// winding and culling
	rRenderMode->set_winding(matchWinding(renderMode.getWindingType()));
	rRenderMode->set_culling(matchFaceCulling(renderMode.getCulling()));
	// tblending
	HmiScenegraph::TBlendMode* tblending = new HmiScenegraph::TBlendMode();
	Blending blending = renderMode.getBlending();
	// operation
	tblending->set_blendoperationcolor(matchBlendOperation(blending.getBlendOperationColor()));
	tblending->set_blendoperationalpha(matchBlendOperation(blending.getBlendOperationAlpha()));
	// factor
	tblending->set_sourcealphafactor(matchBlendFactor(blending.getSrcAlphaFactor()));
	tblending->set_sourcecolorfactor(matchBlendFactor(blending.getSrcColorFactor()));
	tblending->set_destinationalphafactor(matchBlendFactor(blending.getDesAlphaFactor()));
	tblending->set_destinationcolorfactor(matchBlendFactor(blending.getDesColorFactor()));

	rRenderMode->set_allocated_blending(tblending);
	rRenderMode->set_depthcompare(matchCompareFunction(renderMode.getDepthCompare()));

	rRenderMode->set_depthwrite(renderMode.getDepthWrite());
	// ColorWrite
	HmiScenegraph::TRenderMode_TColorWrite* tColorWrite = new HmiScenegraph::TRenderMode_TColorWrite();
	ColorWrite colorWrite = renderMode.getColorWrite();
	// If the colorwrite property value is false, the content will not be displayed in Hmi.
	tColorWrite->set_alpha(true);
	tColorWrite->set_blue(true);
	tColorWrite->set_green(true);
	tColorWrite->set_red(true);
	rRenderMode->set_allocated_colorwrite(tColorWrite);
}

void OutputPtx::uniformTypeValue(Uniform data, HmiScenegraph::TUniform& tUniform) {
	tUniform.set_name(data.getName());
	TNumericValue* tNumericValue = new TNumericValue();
	UniformType dataType = data.getType();
	switch (dataType) {
		case raco::guiData::Int: {
			int temp = std::any_cast<int>(data.getValue());
			tNumericValue->set_int_(temp);
			tUniform.set_allocated_value(tNumericValue);
			tUniform.set_type(TENumericType::TENumericType_integer);
			break;
		}
		case raco::guiData::Double: {
			float temp = std::any_cast<double>(data.getValue());
			tNumericValue->set_float_(temp);
			tUniform.set_allocated_value(tNumericValue);
			tUniform.set_type(TENumericType::TENumericType_float);
			break;
		}
		case raco::guiData::Vec2f: {
			Vec2 temp = std::any_cast<Vec2>(data.getValue());
			TVector2f* tVec2f = new TVector2f();
			tVec2f->set_x(temp.x);
			tVec2f->set_y(temp.y);
			tNumericValue->set_allocated_vec2f(tVec2f);
			tUniform.set_allocated_value(tNumericValue);
			tUniform.set_type(TENumericType::TENumericType_floatVector2);
			break;
		}
		case raco::guiData::Vec3f: {
			Vec3 temp = std::any_cast<Vec3>(data.getValue());
			TVector3f* tVec3f = new TVector3f();
			tVec3f->set_x(temp.x);
			tVec3f->set_y(temp.y);
			tVec3f->set_z(temp.z);
			tNumericValue->set_allocated_vec3f(tVec3f);
			tUniform.set_allocated_value(tNumericValue);
			tUniform.set_type(TENumericType::TENumericType_floatVector3);
			break;
		}
		case raco::guiData::Vec4f: {
			Vec4 temp = std::any_cast<Vec4>(data.getValue());
			TVector4f* tVec4f = new TVector4f();
			tVec4f->set_x(temp.x);
			tVec4f->set_y(temp.y);
			tVec4f->set_z(temp.z);
			tVec4f->set_w(temp.w);
			tNumericValue->set_allocated_vec4f(tVec4f);
			tUniform.set_allocated_value(tNumericValue);
			tUniform.set_type(TENumericType::TENumericType_floatVector4);
			break;
		}
		case raco::guiData::Vec2i: {
			Vec2int temp = std::any_cast<Vec2int>(data.getValue());
			TVector2i* tVec2i = new TVector2i();
			tVec2i->set_x(temp.x);
			tVec2i->set_y(temp.y);
			tNumericValue->set_allocated_vec2i(tVec2i);
			tUniform.set_allocated_value(tNumericValue);
			tUniform.set_type(TENumericType::TENumericType_intVector2);
			break;
		}
		case raco::guiData::Vec3i: {
			Vec3int temp = std::any_cast<Vec3int>(data.getValue());
			TVector3i* tVec3i = new TVector3i();
			tVec3i->set_x(temp.x);
			tVec3i->set_y(temp.y);
			tVec3i->set_z(temp.z);
			tNumericValue->set_allocated_vec3i(tVec3i);
			tUniform.set_allocated_value(tNumericValue);
			tUniform.set_type(TENumericType::TENumericType_intVector3);
			break;
		}
		case raco::guiData::Vec4i: {
			Vec4int temp = std::any_cast<Vec4int>(data.getValue());
			TVector4i* tVec4i = new TVector4i();
			tVec4i->set_x(temp.x);
			tVec4i->set_y(temp.y);
			tVec4i->set_z(temp.z);
			tVec4i->set_w(temp.w);
			tNumericValue->set_allocated_vec4i(tVec4i);
			tUniform.set_allocated_value(tNumericValue);
			tUniform.set_type(TENumericType::TENumericType_intVector4);
			break;
		}
		default:
			DEBUG(__FILE__, __FUNCTION__, __LINE__, "Uniform type is not match with TUniform!");
			break;
	}
}

TETextureFilter OutputPtx::matchFilter(Filter filter) {
	TETextureFilter result = TETextureFilter::TETextureFilter_Linear;
	switch (filter) {
		case raco::guiData::Nearest:
			result = TETextureFilter::TETextureFilter_Nearest;
			break;
		case raco::guiData::Linear:
			result = TETextureFilter::TETextureFilter_Linear;
			break;
		case raco::guiData::NearestMipMapNearest:
			result = TETextureFilter::TETextureFilter_NearestMipMapNearest;
			break;
		case raco::guiData::NearestMipMapLinear:
			result = TETextureFilter::TETextureFilter_NearestMipMapLinear;
			break;
		case raco::guiData::LinearMipMapNearest:
			result = TETextureFilter::TETextureFilter_LinearMipMapNearest;
			break;
		case raco::guiData::LinearMipMapLinear:
			result = TETextureFilter::TETextureFilter_LinearMipMapLinear;
			break;
		default:
			break;
	}

	return result;
}

TETextureWrapMode OutputPtx::matchWrapMode(WrapMode mode) {
	TETextureWrapMode result = TETextureWrapMode::TETextureWrapMode_ClampToEdge;
	switch (mode) {
		case raco::guiData::Clamp:
			result = TETextureWrapMode::TETextureWrapMode_ClampToEdge;
			break;
		case raco::guiData::Repeat:
			result = TETextureWrapMode::TETextureWrapMode_Repeat;
			break;
		case raco::guiData::Mirror:
			result = TETextureWrapMode::TETextureWrapMode_MirroredRepeat;
			break;
		default:
			break;
	}
	return result;
}

bool OutputPtx::mkdir(QString path) {
	QString dest = path;
	QDir dir;
	if (!dir.exists(dest)) {
		dir.mkpath(dest);
		return true;
	} else {
		return false;
	}
}

bool OutputPtx::isStored(std::string name, std::set<std::string>& nameArr) {
	auto it = nameArr.find(name);
	if (it == nameArr.end()) {
		nameArr.emplace(name);
		return false;
	}
	return true;
}

std::string OutputPtx::getShaderPtxNameByShaderName(std::string name) {
	std::map<std::string, Shader> shaderMap = raco::guiData::MaterialManager::GetInstance().getShaderDataMap();
	auto it = shaderMap.find(name);
	if (it == shaderMap.end()) {
		return std::string("");
	} else {
		return it->second.getPtxShaderName();
	}
}

void OutputPtx::messageBoxError(std::string materialName, int type) {
	if (isPtxOutputError_) {
		return;
	}

	QMessageBox customMsgBox;
	customMsgBox.setWindowTitle("Warning message box");
	QPushButton* okButton = customMsgBox.addButton("OK", QMessageBox::ActionRole);
	//QPushButton* cancelButton = customMsgBox.addButton(QMessageBox::Cancel);
	customMsgBox.setIcon(QMessageBox::Icon::Warning);
	QString text;
	if (1 == type) {
		text = QString::fromStdString(materialName) + "\" has an empty texture !";
		text = "Warning: Material \"" + text;
	} else if (2 == type) {
		text = QString::fromStdString(materialName) + "\" generated by mesh node has the same name. !";
		text = "Warning: The private material \"" + text;
	}
	customMsgBox.setText(text);
	customMsgBox.exec();

	if (customMsgBox.clickedButton() == (QAbstractButton *)(okButton)) {
		isPtxOutputError_ = true;
	}
}

bool updateHasOpacityMaterial(std::vector<Uniform> uniforms, std::string materialName) {
	for (auto& uniform : uniforms) {
		if (uniform.getName() == PTW_FRAG_SYS_OPACITY) {
			auto it = HasOpacityMaterials_.find(materialName);
			if (it == HasOpacityMaterials_.end()) {
				HasOpacityMaterials_.emplace(materialName);
			}
			return true;
		}
	}
	return false;
}

void OutputPtx::writeMaterial2MaterialLib(HmiScenegraph::TMaterialLib* materialLibrary) {
	std::map<std::string, MaterialData> materialMap = raco::guiData::MaterialManager::GetInstance().getMaterialDataMap();
	std::set<std::string> setNameArr;
	for (auto& material : materialMap) {
		MaterialData data = material.second;
		HmiScenegraph::TMaterial tMaterial;

		// whether it has been stored?
		if (isStored(data.getObjectName(), setNameArr)) {
			continue;
		}
		// name
		tMaterial.set_name(data.getObjectName());

		// RenderMode
		HmiScenegraph::TRenderMode* rRenderMode = new HmiScenegraph::TRenderMode();
		RenderMode renderMode = data.getRenderMode();
		// setRenderMode
		setMaterialRenderMode(renderMode, rRenderMode);
		tMaterial.set_allocated_rendermode(rRenderMode);

		// shaderReference
		std::string shaderPtxName = getShaderPtxNameByShaderName(data.getShaderRef());
		tMaterial.set_shaderreference(shaderPtxName);

		for (auto& textureData : data.getTextures()) {
			HmiScenegraph::TTexture tTextture;
			if (textureData.getName() == "empty") {
				messageBoxError(data.getObjectName(), 1);
			}
			tTextture.set_name(textureData.getName());
			tTextture.set_bitmapreference(textureData.getBitmapRef());
			tTextture.set_minfilter(matchFilter(textureData.getMinFilter()));
			tTextture.set_magfilter(matchFilter(textureData.getMagFilter()));
			tTextture.set_anisotropicsamples(textureData.getAnisotropicSamples());
			tTextture.set_wrapmodeu(matchWrapMode(textureData.getWrapModeU()));
			tTextture.set_wrapmodev(matchWrapMode(textureData.getWrapModeV()));
			tTextture.set_uniformname(textureData.getUniformName());
			HmiScenegraph::TTexture* textureIt = tMaterial.add_texture();
			*textureIt = tTextture;
		}
		// uniforms
		for (auto& uniform : data.getUniforms()) {
			HmiScenegraph::TUniform tUniform;

			uniformTypeValue(uniform, tUniform);
			HmiScenegraph::TUniform* tUniformIt = tMaterial.add_uniform();
			*tUniformIt = tUniform;
		}

		updateHasOpacityMaterial(data.getUniforms(), data.getObjectName());

		HmiScenegraph::TMaterial* materialIt = materialLibrary->add_material();
		*materialIt = tMaterial;
	}
}

void OutputPtx::writeShaders2MaterialLib(QString& filePath, QString& oldPath, HmiScenegraph::TMaterialLib* materialLibrary) {
	mkdir(filePath + "/shaders");
	std::map<std::string, Shader> shaderMap = raco::guiData::MaterialManager::GetInstance().getShaderDataMap();
	std::set<std::string> shaderNameArr;
	for (auto& shader : shaderMap) {
		// whether it has been stored?
		if (isStored(shader.second.getPtxShaderName(), shaderNameArr)) {
			continue;
		}

		HmiScenegraph::TShader tShader;
		tShader.set_name(shader.second.getPtxShaderName());

		QString shaderPath = oldPath + "/" + QString::fromStdString(shader.second.getVertexShader());
		QFileInfo fileinfo(shaderPath);
		QString shadersPathName = "shaders/" + fileinfo.fileName();
		qDebug() << shadersPathName;
		QString desPath = filePath + "/" + shadersPathName;
		qDebug() << desPath;
		if (!QFile::copy(shaderPath, desPath)) {
			qDebug() << " copy [" << fileinfo.fileName() << " ] failed!";
		}
		tShader.set_vertexshader(shadersPathName.toStdString());
		shaderPath = oldPath + "/" + QString::fromStdString(shader.second.getFragmentShader());
		fileinfo = QFileInfo(shaderPath);
		shadersPathName = "shaders/" + fileinfo.fileName();
		desPath = filePath + "/" + shadersPathName;
		if (!QFile::copy(shaderPath, desPath)) {
			qDebug() << " copy [" << fileinfo.fileName() << " ] failed!";
		}
		tShader.set_fragmentshader(shadersPathName.toStdString());
		HmiScenegraph::TShader* tShaderIt = materialLibrary->add_shader();
		*tShaderIt = tShader;
	}
}

void OutputPtx::writeBitmap2MaterialLib(QString& filePath, QString& oldPath, HmiScenegraph::TMaterialLib* materialLibrary) {
	mkdir(filePath + "./bitmaps");
	std::map<std::string, Bitmap> bitMaps = raco::guiData::MaterialManager::GetInstance().getBitmapDataMap();
	std::set<std::string> bitmapNameArr;
	for (auto& bitData : bitMaps) {
		// whether it has been stored?
		if (isStored(bitData.second.getName(), bitmapNameArr)) {
			continue;
		}
		HmiScenegraph::TBitmap tBitMap;
		if (bitData.second.getName() != "" && bitData.second.getResource() != "") {
			tBitMap.set_name(bitData.second.getName());
			QString bitmapPath = oldPath + "/" + QString::fromStdString(bitData.second.getResource());
			QFileInfo fileinfo(bitmapPath);
			QString bitmapPathName = "bitmaps/" + fileinfo.fileName();
			QString desPath = filePath + "/" + bitmapPathName;
			if (!QFile::copy(bitmapPath, desPath)) {
				qDebug() << " copy [" << fileinfo.fileName() << " ] failed!";
			}

			tBitMap.set_resource(bitmapPathName.toStdString());
			tBitMap.set_generatemipmaps(bitData.second.getGenerateMipmaps());
			HmiScenegraph::TBitmap* tBitMapIt = materialLibrary->add_bitmap();
			*tBitMapIt = tBitMap;
		}
	}
}

void OutputPtx::writeMaterialLib2Ptx(QString& filePath, QString& oldPath, HmiScenegraph::TMaterialLib* materialLibrary) {
	// add materials
	writeMaterial2MaterialLib(materialLibrary);

	// add shaders
	writeShaders2MaterialLib(filePath, oldPath, materialLibrary);

	// add bitmaps
	writeBitmap2MaterialLib(filePath, oldPath, materialLibrary);
}

bool OutputPtx::writeProgram2Ptx(std::string filePathStr, QString oldPath) {
	filePathStr = filePathStr.substr(0, filePathStr.find(".rca"));
	QString filePath = QString::fromStdString(filePathStr);
	mkdir(filePath);

	QFile file(filePath + "/scene.ptx");
	if (!file.open(QIODevice::ReadWrite)) {
		return false;
	}
	file.resize(0);
	nodeWithMaterial_.clear();
	curveNameAnimations_.clear();
	pNodePropCurveNames_.clear();
	HasOpacityMaterials_.clear();
	NodeIDUColorNums_.clear();
	uColorNums_.clear();
	SceneChildName_.clear();
	// root
	NodeData* rootNode = &(raco::guiData::NodeDataManager::GetInstance().root());
	HmiScenegraph::TScene scene;

    HmiScenegraph::TNode* tRoot = new HmiScenegraph::TNode();
	tRoot->set_name(PTX_SCENE_NAME.toStdString());
	setRootSRT(tRoot);
	int rootChildIndex = 0;
	NodeScaleSize_ = 0;
	int nodeNum = 0;
    for (auto& child : rootNode->childMapRef()) {
		NodeData* childNode = &(child.second);
		if (-1 != childNode->getName().find("PerspectiveCamera")) {
			continue;
		}
		nodeNum++;
		SceneChildName_ = childNode->getName();
		writeNodePtx(childNode, tRoot);
		if (nodeNum > 1) {
			// Scene Child more than one.
			DEBUG(__FILE__, __FUNCTION__, __LINE__, "Scene Child more than one!");
		}
	}
    scene.set_allocated_root(tRoot);

	// materiallibrary
	HmiScenegraph::TMaterialLib* materialLibrary = new HmiScenegraph::TMaterialLib();
	writeMaterialLib2Ptx(filePath, oldPath, materialLibrary);
	scene.set_allocated_materiallibrary(materialLibrary);

    std::string output;
	google::protobuf::TextFormat::PrintToString(scene, &output);

    QByteArray byteArray = QByteArray::fromStdString(output);
	file.write(byteArray);
	file.close();

	if (isPtxOutputError_) {
		QFile::remove(filePath + "/scene.ptx");
		isPtxOutputError_ = false;
		return false;
	}
	return true;
}

// Only when multiple animations bind one curve
void OutputPtw::switchMultAnimsOneCurve(HmiWidget::TWidget* widget) {
	for (auto curve : curveNameAnimations_) {
		if (curve.second.size() > 1) {
			PTWSwitch switchData;
			switchData.outPutKey = curve.first + "_interal_switch";
			switchData.dataType1 = TEDataType_Identifier;
			switchData.dataType2 = TEDataType_Float;
			
			TDataBinding Operand;
			Operand.set_allocated_key(assetsFun_.Key(PTW_USED_ANIMATION_NAME));
			Operand.set_allocated_provider(assetsFun_.ProviderSrc(TEProviderSource_IntModelValue));
			switchData.Operands.push_back(Operand);

			for (auto anName : curve.second) {
				TDataBinding Operand1;
				assetsFun_.OperandProVarIdentAndType(Operand1, anName.first, TEIdentifierType_ParameterValue);
				switchData.Operands.push_back(Operand1);

				TDataBinding Operand2;
				assetsFun_.OperandKeySrc(Operand2, anName.first + PTW_SUF_CURVE_INETERAL, TEProviderSource_IntModelValue);
				switchData.Operands.push_back(Operand2);
			}

			TDataBinding OperandDefault;
			assetsFun_.OperandKeySrc(OperandDefault, curve.second.begin()->first + PTW_SUF_CURVE_INETERAL, TEProviderSource_IntModelValue);
			switchData.Operands.push_back(OperandDefault);

			HmiWidget::TInternalModelParameter* internalModelswitch = widget->add_internalmodelvalue();
			assetsFun_.SwitchAnimation(internalModelswitch, switchData);
		}
	}
}

void OutputPtw::ConvertAnimationInfo(HmiWidget::TWidget* widget) {
	auto animationList = raco::guiData::animationDataManager::GetInstance().getAnitnList();
	if (0 == animationList.size()) {
		messageBoxError("", 5);
	}

	for (auto animation : animationList) {
		std::string animation_interal = animation.first + PTW_SUF_CURVE_INETERAL;
		auto animations = animationDataManager::GetInstance().getAnitnList();

		std::vector<TDataBinding> Operands;
		HmiWidget::TInternalModelParameter* internalModelMul = widget->add_internalmodelvalue();
		TDataBinding Operand1;
		Operand1.set_allocated_key(assetsFun_.Key(animation.first + PTW_SUF_ANIMAT_DOMAIN));
		Operand1.set_allocated_provider(assetsFun_.ProviderSrc(TEProviderSource_IntModelValue));
		Operands.push_back(Operand1);
		TDataBinding Operand2;
		Operand2.set_allocated_provider(assetsFun_.ProviderNumeric(float(animation.second.GetEndTime() - animation.second.GetStartTime())));
		Operands.push_back(Operand2);
		TDataBinding Operand3;
		Operand3.set_allocated_key(assetsFun_.Key(animation.first));
		Operand3.set_allocated_provider(assetsFun_.ProviderSrc(TEProviderSource_ExtModelValue));
		Operands.push_back(Operand3);
		assetsFun_.operatorOperands(internalModelMul, animation_interal, TEDataType_Float, Operands, TEOperatorType_Mul);

		addAnimationDomain(widget, animation.first);
	}
	switchMultAnimsOneCurve(widget);
}

// animation domain switch
void OutputPtw::addAnimationDomain(HmiWidget::TWidget* widget, std::string animationName) {
	PTWSwitch switchData;
	switchData.outPutKey = animationName + PTW_SUF_ANIMAT_DOMAIN;
	switchData.dataType1 = TEDataType_Identifier;
	switchData.dataType2 = TEDataType_Float;

	TDataBinding Operand;
	Operand.set_allocated_key(assetsFun_.Key(PTW_USED_ANIMATION_NAME));
	Operand.set_allocated_provider(assetsFun_.ProviderSrc(TEProviderSource_IntModelValue));
	switchData.Operands.push_back(Operand);

	TDataBinding Operand1;
	assetsFun_.OperandProVarIdentAndType(Operand1, animationName, TEIdentifierType_ParameterValue);
	switchData.Operands.push_back(Operand1);

	TDataBinding Operand2;
	assetsFun_.OperandKeySrc(Operand2, "domain", TEProviderSource_IntModelValue);
	switchData.Operands.push_back(Operand2);
	// default Numeric
	TDataBinding OperandDefault;
	assetsFun_.OperandNumeric(OperandDefault, 0.0);
	switchData.Operands.push_back(OperandDefault);

	HmiWidget::TInternalModelParameter* internalModelswitch = widget->add_internalmodelvalue();
	assetsFun_.SwitchAnimation(internalModelswitch, switchData);
}

void OutputPtw::triggerByInternalModel(HmiWidget::TWidget* widget) {
	internalDurationValue(widget);

	assetsFun_.addCompositeAnimation(widget);
	assetsFun_.addTrigger(widget);
	assetsFun_.addPlayDomain(widget);
}

void OutputPtw::triggerByExternalModel(HmiWidget::TWidget* widget) {
	externalAnimation(widget);
	auto domain = widget->add_internalmodelvalue();
	domain->set_allocated_key(assetsFun_.Key("domain"));
	// todo: Ex_Triggle_Animation * 1.0 -> domain
	auto binding = assetsFun_.BindingValueStrNumericOperatorType(PTW_EX_TRIGGLE_ANIMATION, TEProviderSource_ExtModelValue, 3.5, TEOperatorType_Mul);

	domain->set_allocated_binding(binding);
}

void OutputPtw::messageBoxError(std::string curveName,int errorNum) {
	if (isPtwOutputError_) {
		return;
	}
	QMessageBox customMsgBox;
	customMsgBox.setWindowTitle("Warning message box");
	QPushButton* okButton = customMsgBox.addButton("OK", QMessageBox::ActionRole);
	customMsgBox.setIcon(QMessageBox::Icon::Warning);
	QString text;
	if (errorNum == 1) {
		text = QString::fromStdString(curveName) + "\" components is less than 3 !";
		text = "Warning: The number of Rotation Curve \"" + text;
	} else if (errorNum == 2) {
		text = QString::fromStdString(curveName) + "\" do not match !";
		text = "Warning: The length in \"" + text;
	} else if (errorNum == 3) {
		text = QString::fromStdString(curveName) + "\" do not match !";
		text = "Warning: The keyframe points in \"" + text;
	} else if (errorNum == 4) {
		text = QString::fromStdString(curveName) + "\" is neither linear nor hermite/beiser !";
		text = "Warning: The type of curve  \"" + text;
	} else if (errorNum == 5) {
		text = "No animation information !";
	}
	customMsgBox.setText(text);
	customMsgBox.exec();

	if (customMsgBox.clickedButton() == (QAbstractButton*)(okButton)) {
		isPtwOutputError_ = true;
	}
}

bool getAnimationInteral(std::string curveName, std::string& animationInteral) {
	auto it = curveNameAnimations_.find(curveName);
	if (it != curveNameAnimations_.end()) {
		auto animations = it->second;
		if (animations.size() > 1) {
			animationInteral = curveName + "_interal_switch";
		} else {
			// bugs
			animationInteral = animations.begin()->first + PTW_SUF_CURVE_INETERAL;
		}
		return true;
	} else {
		return false;
	}
}
// if Curve only has one point, add Point.
void OutputPtw::modifyOnePointCurve(Point* point, TCurveDefinition* curveDefinition, std::string curveName) {
	Point* pointTemp = new Point(*point);

	int key = pointTemp->getKeyFrame();
	if (key == 0) {
		pointTemp->setKeyFrame(key+1);
	} else {
		pointTemp->setKeyFrame(key-1);
	}
	addPoint2Curve(pointTemp, curveDefinition, curveName);
}

void OutputPtw::addPoint2Curve(Point* pointData, TCurveDefinition* curveDefinition, std::string curveName) {
	auto point = curveDefinition->add_point();
	TMultidimensionalPoint* pot = new TMultidimensionalPoint;
	TNumericValue* value = new TNumericValue;
	value->set_float_(std::any_cast<double>(pointData->getDataValue()));
	pot->set_domain(pointData->getKeyFrame());
	pot->set_allocated_value(value);
	point->set_allocated_point(pot);

	if (pointData->getInterPolationType() == raco::guiData::LINER) {
		TCurvePointInterpolation* incommingInterpolation = new TCurvePointInterpolation;
		incommingInterpolation->set_interpolation(TCurvePointInterpolationType_Linear);
		point->set_allocated_incomminginterpolation(incommingInterpolation);
		TCurvePointInterpolation* outgoingInterpolation = new TCurvePointInterpolation;
		outgoingInterpolation->set_interpolation(TCurvePointInterpolationType_Linear);
		point->set_allocated_outgoinginterpolation(outgoingInterpolation);
	} else if (pointData->getInterPolationType() == raco::guiData::BESIER_SPLINE) {	 // HERMIT_SPLINE
		TCurvePointInterpolation* incommingInterpolation = new TCurvePointInterpolation;
		incommingInterpolation->set_interpolation(TCurvePointInterpolationType_Hermite);
		TMultidimensionalPoint* lefttangentVector = new TMultidimensionalPoint;
		lefttangentVector->set_domain(pointData->getLeftKeyFrame());
		TNumericValue* leftValue = new TNumericValue;
		leftValue->set_float_(std::any_cast<double>(pointData->getLeftData()));
		lefttangentVector->set_allocated_value(leftValue);
		incommingInterpolation->set_allocated_tangentvector(lefttangentVector);
		point->set_allocated_incomminginterpolation(incommingInterpolation);

		TCurvePointInterpolation* outgoingInterpolation = new TCurvePointInterpolation;
		outgoingInterpolation->set_interpolation(TCurvePointInterpolationType_Hermite);
		TMultidimensionalPoint* RighttangentVector = new TMultidimensionalPoint;
		RighttangentVector->set_domain(pointData->getRightKeyFrame());
		TNumericValue* RightValue = new TNumericValue;
		//double right = std::any_cast<double>(pointData->getRightTagent());
		RightValue->set_float_(std::any_cast<double>(pointData->getRightData()));
		RighttangentVector->set_allocated_value(RightValue);
		outgoingInterpolation->set_allocated_tangentvector(RighttangentVector);
		point->set_allocated_outgoinginterpolation(outgoingInterpolation);
	} else {
		messageBoxError(curveName, 4);
	}
}

void OutputPtw::ConvertCurveInfo(HmiWidget::TWidget* widget, std::string animation_interal) {
	for (auto curveData : raco::guiData::CurveManager::GetInstance().getCurveList()) {
		std::string animation_interal;
		if (!getAnimationInteral(curveData->getCurveName(), animation_interal)) {
			continue;
		}
		auto curve = widget->add_curve();
		TIdentifier* curveIdentifier = new TIdentifier;
		curveIdentifier->set_valuestring(curveData->getCurveName());
		curve->set_allocated_curveidentifier(curveIdentifier);
		TDataBinding* samplePosition = new TDataBinding;
		TIdentifier* key = new TIdentifier;
		key->set_valuestring(animation_interal);
		samplePosition->set_allocated_key(key);
		TDataProvider* provider = new TDataProvider;
		provider->set_source(TEProviderSource_IntModelValue);
		samplePosition->set_allocated_provider(provider);
		curve->set_allocated_sampleposition(samplePosition);
		TCurveDefinition* curveDefinition = new TCurveDefinition;
		curveDefinition->set_curvevaluetype(TENumericType_float);
		for (auto pointData : curveData->getPointList()) {
			addPoint2Curve(pointData, curveDefinition, curveData->getCurveName());
		}
		if (curveData->getPointList().size() == 1) {
			modifyOnePointCurve(curveData->getPointList().front(), curveDefinition, curveData->getCurveName());
		}
		curve->set_allocated_curvedefinition(curveDefinition);
	}
}


bool hasUColorUniform(std::string id) {
	auto it = NodeIDUColorNums_.find(id);
	if (it != NodeIDUColorNums_.end()) {
		return true;
	}
	return false;
}

void OutputPtw::ConvertBind(HmiWidget::TWidget* widget, raco::guiData::NodeData& node) {
	if (0 != node.getBindingySize() || hasUColorUniform(node.objectID())) {
		HmiWidget::TNodeParam* nodeParam = widget->add_nodeparam();
		TIdentifier* identifier = new TIdentifier;
		NodeMaterial nodeMaterial;
		if (raco::guiData::MaterialManager::GetInstance().getNodeMaterial(node.objectID(), nodeMaterial)) {
			identifier->set_valuestring(node.getName() + "Shape");
		} else {
			identifier->set_valuestring(node.getName());
		}
		if (0 == node.getMaterialName().compare("")) {
			identifier->set_valuestring(node.getName());
		} else {
			identifier->set_valuestring(node.getName() + "Shape");
		}
		nodeParam->set_allocated_identifier(identifier);
		TDataBinding* paramnode = new TDataBinding;
		TDataProvider* provider = new TDataProvider;
		TVariant* variant = new TVariant;
		if (raco::guiData::MaterialManager::GetInstance().getNodeMaterial(node.objectID(), nodeMaterial)) {
			variant->set_asciistring(node.getName() + "Shape");
		} else {
			variant->set_asciistring(node.getName());
		}

		provider->set_allocated_variant(variant);
		paramnode->set_allocated_provider(provider);
		nodeParam->set_allocated_node(paramnode);
		auto animationList = node.NodeExtendRef().curveBindingRef().bindingMap();
		for (auto cuvebindList : animationList) {
			for (auto curveProP : cuvebindList.second) {
				if (curveProP.first.find("translation") == 0) {
					if (nodeParam->has_transform()) {
						auto transform = nodeParam->mutable_transform();
						if (transform->has_translation()) {
							ModifyTranslation(curveProP, transform, node);
						} else {
							CreateTranslation(curveProP, transform, node);
						}
					} else {
						HmiWidget::TNodeTransform* transform = new HmiWidget::TNodeTransform;
						CreateTranslation(curveProP, transform, node);
						nodeParam->set_allocated_transform(transform);
					}
				} else if (curveProP.first.find("rotation") == 0) {
					if (nodeParam->has_transform()) {
						auto transform = nodeParam->mutable_transform();
						if (transform->has_rotation()) {
							ModifyRotation(curveProP, transform);
						} else {
							CreateRotation(curveProP, transform, node);
						}
					} else {
						HmiWidget::TNodeTransform* transform = new HmiWidget::TNodeTransform;
						CreateRotation(curveProP, transform, node);
						nodeParam->set_allocated_transform(transform);
					}
				} else if (curveProP.first.find("scale") == 0) {
					if (nodeParam->has_transform()) {
						auto transform = nodeParam->mutable_transform();
						if (transform->has_scale()) {
							ModifyScale(curveProP, transform);
						} else {
							CreateScale(curveProP, transform, node);
						}
					} else {
						HmiWidget::TNodeTransform* transform = new HmiWidget::TNodeTransform;
						CreateScale(curveProP, transform, node);
						nodeParam->set_allocated_transform(transform);
					}
				} else { // find uniform curve
					AddUniform(widget, curveProP, nodeParam, &node);
				}
			}
		}
		// add u_color uniforms
		AddUColorUniforms(nodeParam, &node);
		for (auto cuvebindList : animationList) {
			for (auto curveProP : cuvebindList.second) {
				std::vector<std::map<std::string, CurvesSingleProp>> curves;
				bool hasMultiCurveSingleProp = hasMultiCurveOneProp(curveProP.first, &node, curves);
				if (!hasMultiCurveSingleProp) {
					continue;
				}
				if (curveProP.first.find("scale") == 0 || curveProP.first.find("translation") == 0 || curveProP.first.find("rotation") == 0) {
					auto transform = nodeParam->mutable_transform();
					modifyMultiCurveTransform(widget, transform, curveProP.first, curves);
				}
			}
		}

		for (auto cuvebindList : animationList) {
			for (auto curveProP : cuvebindList.second) {
				std::string animaitionName;
				AnimationsSingleCurve aniSingleCurve;
				bool hasMultiAnimationSingleCurve = hasMultiAnimationOneCurve(curveProP.second, &node, aniSingleCurve, animaitionName);
				if (!hasMultiAnimationSingleCurve) {
					continue;
				}
				if (curveProP.first.find("scale") == 0 || curveProP.first.find("translation") == 0 || curveProP.first.find("rotation") == 0) {
					auto transform = nodeParam->mutable_transform();

					modifyMultiAnimaTransform(widget, transform, curveProP.first, curveProP.second, aniSingleCurve, animaitionName);
				}
			}
		}
	}

	if (node.getChildCount() != 0) {
		for (auto childNode : node.childMapRef()) {
			ConvertBind(widget, childNode.second);
		}
	} else {
		return;
	}
}

void OutputPtw::proExVarMapping(HmiWidget::TWidget* widget) {
// todo:dot background
}

void OutputPtw::WriteAsset(std::string filePath) {
	filePath = filePath.substr(0, filePath.find(".rca"));
	nodeIDUniformsName_.clear();
	addTrigger_ = true; // todo:

	HmiWidget::TWidgetCollection widgetCollection;
	HmiWidget::TWidget* widget = widgetCollection.add_widget();
	WriteBasicInfo(widget);
	switchAnimations(widget);
	externalScaleData(widget);
	if (addTrigger_) {
		triggerByInternalModel(widget);
	}
	else {
		triggerByExternalModel(widget);
	}
	ConvertAnimationInfo(widget);
	std::string animation_interal = "";
	ConvertCurveInfo(widget, animation_interal);
	ConvertBind(widget, NodeDataManager::GetInstance().root());
	externalOpacityData(widget);
	externalColorData(widget);
	proExVarMapping(widget);

	std::string output;
	google::protobuf::TextFormat::PrintToString(widgetCollection, &output);

	QDir* folder = new QDir;
	if (!folder->exists(QString::fromStdString(filePath))) {
		bool ok = folder->mkpath(QString::fromStdString(filePath));
	}
	delete folder;
	std::ofstream outfile;
	outfile.open(filePath + "/widget.ptw", std::ios_base::out | std::ios_base::trunc);
	outfile << output << std::endl;
	outfile.close();

	if (isPtwOutputError_) {
		QFile::remove(QString::fromStdString(filePath) + "/widget.ptw");
		isPtwOutputError_ = false;
	}
	addTrigger_ = false;
}

void OutputPtw::WriteBasicInfo(HmiWidget::TWidget* widget) {
	// type
	TIdentifier* type = new TIdentifier;
	type->set_valuestring("eWidgetType_Generate");
	widget->set_allocated_type(type);
	// prototype
	TIdentifier* prototype = new TIdentifier;
	prototype->set_valuestring("eWidgetType_Model");
	widget->set_allocated_prototype(prototype);
	// WidgetNameHint
	HmiWidget::TExternalModelParameter* externalModel = widget->add_externalmodelvalue();
	assetsFun_.externalKeyVariant(externalModel, "WidgetNameHint", assetsFun_.VariantAsciiString("WIDGET_SCENE"));
	// eParam_ModelResourceId
	externalModel = widget->add_externalmodelvalue();
	assetsFun_.externalKeyVariant(externalModel, "eParam_ModelResourceId", assetsFun_.VariantResourceId("scene.ptx"));
	// eParam_ModelRootId
	externalModel = widget->add_externalmodelvalue();
	assetsFun_.externalKeyVariant(externalModel, "eParam_ModelRootId", assetsFun_.VariantResourceId(""));
}
// Multi Curves Binding Single Prop: Switch different curves for different animations --> curve 
void OutputPtw::multiCurveBindingSinglePropSwitch(HmiWidget::TWidget* widget, std::string propName, std::vector<std::map<std::string, CurvesSingleProp>> curves) {
	PTWSwitch switchData;
	switchData.outPutKey = PTW_PRE_CURVES_ONE_PROP + (curves.at(0).begin())->second.curveName;
	switchData.dataType1 = TEDataType_Identifier;
	switchData.dataType2 = TEDataType_Float;

	TDataBinding Operand;
	Operand.set_allocated_key(assetsFun_.Key(PTW_USED_ANIMATION_NAME));
	Operand.set_allocated_provider(assetsFun_.ProviderSrc(TEProviderSource_IntModelValue));
	switchData.Operands.push_back(Operand);

	for (auto anName : curves) {
		TDataBinding Operand1;
		assetsFun_.OperandProVarIdentAndType(Operand1, anName.begin()->first, TEIdentifierType_ParameterValue);
		switchData.Operands.push_back(Operand1);

		TDataBinding Operand2;
		assetsFun_.OperandKeyCurveRef(Operand2, anName.begin()->second.curveName);
		switchData.Operands.push_back(Operand2);
	}
	// default Numeric
	TDataBinding OperandDefault;
	assetsFun_.OperandNumeric(OperandDefault, curves.begin()->begin()->second.defaultData);
	switchData.Operands.push_back(OperandDefault);

	HmiWidget::TInternalModelParameter* internalModelswitch = widget->add_internalmodelvalue();
	assetsFun_.SwitchAnimation(internalModelswitch, switchData);
}

void modifyMultiCurveTranslation(HmiWidget::TNodeTransform* transform, std::string propName, std::string curveName) {
	auto translation = transform->mutable_translation();
	auto provider = translation->mutable_provider();
	auto operation = provider->mutable_operation();

	if (propName.compare("translation.x") == 0) {
		TDataBinding* operand = operation->mutable_operand(0);
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* key = new TIdentifier;
		key->set_valuestring(curveName);
		TDataProvider* provider = new TDataProvider;
		provider->set_source(TEProviderSource_IntModelValue);
		operand->set_allocated_key(key);
		operand->set_allocated_provider(provider);
	} else if (propName.compare("translation.y") == 0) {
		TDataBinding* operand = operation->mutable_operand(1);
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* key = new TIdentifier;
		key->set_valuestring(curveName);
		TDataProvider* provider = new TDataProvider;
		provider->set_source(TEProviderSource_IntModelValue);
		operand->set_allocated_key(key);
		operand->set_allocated_provider(provider);
	} else if (propName.compare("translation.z") == 0) {
		TDataBinding* operand = operation->mutable_operand(2);
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* key = new TIdentifier;
		key->set_valuestring(curveName);
		TDataProvider* provider = new TDataProvider;
		provider->set_source(TEProviderSource_IntModelValue);
		operand->set_allocated_key(key);
		operand->set_allocated_provider(provider);
	}
}

void modifyMultiCurveRotation(HmiWidget::TNodeTransform* transform, std::string propName, std::string curveName) {
	auto rotation = transform->mutable_rotation();
	auto provider = rotation->mutable_provider();
	auto operation = provider->mutable_operation();

	if (propName.compare("rotation.x") == 0) {
		TDataBinding* operand = operation->mutable_operand(0);
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* key = new TIdentifier;
		key->set_valuestring(curveName);
		TDataProvider* provider = new TDataProvider;
		provider->set_source(TEProviderSource_IntModelValue);
		operand->set_allocated_key(key);
		operand->set_allocated_provider(provider);
	} else if (propName.compare("rotation.y") == 0) {
		TDataBinding* operand = operation->mutable_operand(1);
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* key = new TIdentifier;
		key->set_valuestring(curveName);
		TDataProvider* provider = new TDataProvider;
		provider->set_source(TEProviderSource_IntModelValue);
		operand->set_allocated_key(key);
		operand->set_allocated_provider(provider);
	} else if (propName.compare("rotation.z") == 0) {
		TDataBinding* operand = operation->mutable_operand(2);
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* key = new TIdentifier;
		key->set_valuestring(curveName);
		TDataProvider* provider = new TDataProvider;
		provider->set_source(TEProviderSource_IntModelValue);
		operand->set_allocated_key(key);
		operand->set_allocated_provider(provider);
	}
}

void modifyMultiCurveScale(HmiWidget::TNodeTransform* transform, std::string propName, std::string curveName) {
	auto scale = transform->mutable_scale();
	auto provider = scale->mutable_provider();
	auto operation = provider->mutable_operation();

	if (propName.compare("scale.x") == 0) {
		TDataBinding* operand = operation->mutable_operand(0);
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* key = new TIdentifier;
		key->set_valuestring(curveName);
		TDataProvider* provider = new TDataProvider;
		provider->set_source(TEProviderSource_IntModelValue);
		operand->set_allocated_key(key);
		operand->set_allocated_provider(provider);
	} else if (propName.compare("scale.y") == 0) {
		TDataBinding* operand = operation->mutable_operand(1);
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* key = new TIdentifier;
		key->set_valuestring(curveName);
		TDataProvider* provider = new TDataProvider;
		provider->set_source(TEProviderSource_IntModelValue);
		operand->set_allocated_key(key);
		operand->set_allocated_provider(provider);
	} else if (propName.compare("scale.z") == 0) {
		TDataBinding* operand = operation->mutable_operand(2);
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* key = new TIdentifier;
		key->set_valuestring(curveName);
		TDataProvider* provider = new TDataProvider;
		provider->set_source(TEProviderSource_IntModelValue);
		operand->set_allocated_key(key);
		operand->set_allocated_provider(provider);
	}
}

void OutputPtw::modifyMultiCurveTransform(HmiWidget::TWidget* widget, HmiWidget::TNodeTransform* transform, std::string propName, std::vector<std::map<std::string, CurvesSingleProp>> curves) {
	std::string curveName = std::string(PTW_PRE_CURVES_ONE_PROP) + (curves.at(0).begin())->second.curveName;
	if (propName.find("translation") == 0) {
		modifyMultiCurveTranslation(transform, propName, curveName);
	} else if (propName.find("rotation") == 0) {
		modifyMultiCurveRotation(transform, propName, curveName);
	} else if (propName.find("scale") == 0) {
		modifyMultiCurveScale(transform, propName, curveName);
	}
	multiCurveBindingSinglePropSwitch(widget, propName, curves);
}

// same curve is used by different animation different node. test/05/1
void OutputPtw::addAnimationCurveSwitch(HmiWidget::TWidget* widget, std::string animationName, std::string curveName, AnimationsSingleCurve aniSingleCurve) {
	PTWSwitch switchData;
	switchData.outPutKey = animationName + "-" + curveName;
	switchData.dataType1 = TEDataType_Identifier;
	switchData.dataType2 = TEDataType_Float;

	TDataBinding Operand;
	Operand.set_allocated_key(assetsFun_.Key(PTW_USED_ANIMATION_NAME));
	Operand.set_allocated_provider(assetsFun_.ProviderSrc(TEProviderSource_IntModelValue));
	switchData.Operands.push_back(Operand);

	TDataBinding Operand1;
	assetsFun_.OperandProVarIdentAndType(Operand1, animationName, TEIdentifierType_ParameterValue);
	switchData.Operands.push_back(Operand1);

	TDataBinding Operand2;
	assetsFun_.OperandCurveRef(Operand2, curveName);
	switchData.Operands.push_back(Operand2);
	// default Numeric
	TDataBinding OperandDefault;
	assetsFun_.OperandNumeric(OperandDefault, aniSingleCurve.defaultData);
	switchData.Operands.push_back(OperandDefault);

	HmiWidget::TInternalModelParameter* internalModelswitch = widget->add_internalmodelvalue();
	assetsFun_.SwitchAnimation(internalModelswitch, switchData);
}
// todo
void OutputPtw::modifyMultiAnimaTransform(HmiWidget::TWidget* widget, HmiWidget::TNodeTransform* transform, std::string propName, std::string curve, AnimationsSingleCurve aniSingleCurve, std::string animationName) {
	std::string curveName = animationName + "-" + curve;
	if (propName.find("translation") == 0) {
		modifyMultiCurveTranslation(transform, propName, curveName);
	} else if (propName.find("rotation") == 0) {
		modifyMultiCurveRotation(transform, propName, curveName);
	} else if (propName.find("scale") == 0) {
		modifyMultiCurveScale(transform, propName, curveName);
	}
	addAnimationCurveSwitch(widget, animationName, curve, aniSingleCurve);
}

// std::map<std::string, std::vector<std::map<std::string, std::vector<std::map<std::string, std::string>>>>> pNodePropCurveNames_;
//        pNodeID ,	          <   propertyName,  [ < Animation, curveName > ]	>
bool OutputPtw::hasMultiCurveOneProp(std::string prop, NodeData* pNode, std::vector<std::map<std::string, CurvesSingleProp>>& curves) {
	curves.clear();
	std::map<std::string, std::vector<std::map<std::string, std::vector<std::map<std::string, CurvesSingleProp>>>>> test = pNodePropCurveNames_;
	auto it = pNodePropCurveNames_.find(pNode->objectID());
	if (it != pNodePropCurveNames_.end()) {
		std::vector<std::map<std::string, std::vector<std::map<std::string, CurvesSingleProp>>>>& propCurves = it->second;
		for (auto& propCurve : propCurves) {
			auto itProp = propCurve.find(prop);
			if (itProp != propCurve.end()) {
				curves = itProp->second;
				propCurve.erase(itProp->first);
				return true;
			}
		}
	}
	return false;
}

// std::map<std::string, std::map<std::string, std::vector<AnimationsSingleCurve>>> curveNameAnimations_;
//        curveName  ,          [Animation,      [nodeID,   defaultData]]
bool OutputPtw::hasMultiAnimationOneCurve(std::string curveName, NodeData* pNode, AnimationsSingleCurve& aniSingleCurv, std::string& animationName) {
	for (auto it = curveNameAnimations_.begin(); it != curveNameAnimations_.end(); it++) {
		if (it->second.size() > 1 && it->first == curveName) {
			for (auto& animation : it->second) {
				for (auto& aniSingleCurve : animation.second) {
					if (pNode->objectID() == aniSingleCurve.pNode->objectID()) {
						aniSingleCurve.defaultData = setCurvesPropData(aniSingleCurve.pNode, aniSingleCurve.property);
						aniSingleCurv = aniSingleCurve;
						animationName = animation.first;
						return true;
					}
				}
			}
		}
	}
	return false;
}

void OutputPtw::ModifyTranslation(std::pair<std::string, std::string> curveProP, HmiWidget::TNodeTransform* transform, NodeData& node) {
	auto translation = transform->mutable_translation();
	auto provider = translation->mutable_provider();
	auto operation = provider->mutable_operation();

	if (curveProP.first.compare("translation.x") == 0) {
		TDataBinding* operand = operation->mutable_operand(0);
		// has multi curves binding
		if (operand->has_key() && operand->has_provider()) {
			return;
		}
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);
	} else if (curveProP.first.compare("translation.y") == 0) {
		TDataBinding* operand = operation->mutable_operand(1);
		// has multi curves binding
		if (operand->has_key() && operand->has_provider()) {
			return;
		}
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);
	} else if (curveProP.first.compare("translation.z") == 0) {
		TDataBinding* operand = operation->mutable_operand(2);
		// has multi curves binding
		if (operand->has_key() && operand->has_provider()) {
			return;
		}
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);
	}

}

void OutputPtw::CreateTranslation(std::pair<std::string, std::string> curveProP, HmiWidget::TNodeTransform* transform , raco::guiData::NodeData& node) {
	TDataBinding* translation = new TDataBinding;
	TDataProvider* provider = new TDataProvider;
	TOperation* operation = new TOperation;
	operation->set_operator_(TEOperatorType_MuxVec3);
	operation->add_datatype(TEDataType_Float);
	operation->add_datatype(TEDataType_Float);
	operation->add_datatype(TEDataType_Float);
	auto operand1 = operation->add_operand();
	auto operand2 = operation->add_operand();
	auto operand3 = operation->add_operand();
	if (curveProP.first.compare("translation.x") == 0) {
		TDataProvider* provide = new TDataProvider;
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);
		operand1->set_allocated_provider(provide);
	} else {
		TDataProvider* provide = new TDataProvider;
		TVariant* variant = new TVariant;
		TNumericValue* nuneric = new TNumericValue;
		nuneric->set_float_(std::any_cast<Vec3>(node.getSystemData("translation")).x);
		variant->set_allocated_numeric(nuneric);
		provide->set_allocated_variant(variant);
		operand1->set_allocated_provider(provide);
	}

	if (curveProP.first.compare("translation.y") == 0) {
		TDataProvider* provide = new TDataProvider;
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);
		operand2->set_allocated_provider(provide);
	} else {
		TDataProvider* provide = new TDataProvider;
		TVariant* variant = new TVariant;
		TNumericValue* nuneric = new TNumericValue;
		nuneric->set_float_(std::any_cast<Vec3>(node.getSystemData("translation")).y);
		variant->set_allocated_numeric(nuneric);
		provide->set_allocated_variant(variant);
		operand2->set_allocated_provider(provide);
	}

	if (curveProP.first.compare("translation.z") == 0) {
		TDataProvider* provide = new TDataProvider;
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);
		operand3->set_allocated_provider(provide);
	} else {
		TDataProvider* provide = new TDataProvider;
		TVariant* variant = new TVariant;
		TNumericValue* nuneric = new TNumericValue;
		nuneric->set_float_(std::any_cast<Vec3>(node.getSystemData("translation")).z);
		variant->set_allocated_numeric(nuneric);
		provide->set_allocated_variant(variant);
		operand3->set_allocated_provider(provide);
	}

	provider->set_allocated_operation(operation);
	translation->set_allocated_provider(provider);
	transform->set_allocated_translation(translation);
}
void OutputPtw::ModifyScale(std::pair<std::string, std::string> curveProP, HmiWidget::TNodeTransform* transform) {
	auto scale = transform->mutable_scale();
	auto provider = scale->mutable_provider();
	auto operation = provider->mutable_operation();
	if (curveProP.first.compare("scale.x") == 0) {
		TDataBinding* operand = operation->mutable_operand(0);
		// has multi curves binding
		if (operand->has_key() && operand->has_provider()) {
			return;
		}
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);
	} else if (curveProP.first.compare("scale.y") == 0) {
		TDataBinding* operand = operation->mutable_operand(1);
		// has multi curves binding
		if (operand->has_key() && operand->has_provider()) {
			return;
		}
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);

	} else if (curveProP.first.compare("scale.z") == 0) {
		TDataBinding* operand = operation->mutable_operand(2);
		// has multi curves binding
		if (operand->has_key() && operand->has_provider()) {
			return;
		}
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);
	}
}
void OutputPtw::CreateScale(std::pair<std::string, std::string> curveProP, HmiWidget::TNodeTransform* transform, raco::guiData::NodeData node) {
	TDataBinding* scale = new TDataBinding;
	TDataProvider* provider = new TDataProvider;
	TOperation* operation = new TOperation;
	operation->set_operator_(TEOperatorType_MuxVec3);
	operation->add_datatype(TEDataType_Float);
	operation->add_datatype(TEDataType_Float);
	operation->add_datatype(TEDataType_Float);
	auto operand1 = operation->add_operand();
	auto operand2 = operation->add_operand();
	auto operand3 = operation->add_operand();
	if (curveProP.first.compare("scale.x") == 0) {
		TDataProvider* provide = new TDataProvider;
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);
		operand1->set_allocated_provider(provide);
	} else {
		TDataProvider* provide = new TDataProvider;
		TVariant* variant = new TVariant;
		TNumericValue* nuneric = new TNumericValue;
		nuneric->set_float_(std::any_cast<Vec3>(node.getSystemData("scale")).x);
		variant->set_allocated_numeric(nuneric);
		provide->set_allocated_variant(variant);
		operand1->set_allocated_provider(provide);
	}

	if (curveProP.first.compare("scale.y") == 0) {
		TDataProvider* provide = new TDataProvider;
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);
		operand2->set_allocated_provider(provide);
	} else {
		TDataProvider* provide = new TDataProvider;
		TVariant* variant = new TVariant;
		TNumericValue* nuneric = new TNumericValue;
		nuneric->set_float_(std::any_cast<Vec3>(node.getSystemData("scale")).y);
		variant->set_allocated_numeric(nuneric);
		provide->set_allocated_variant(variant);
		operand2->set_allocated_provider(provide);
	}

	if (curveProP.first.compare("scale.z") == 0) {
		TDataProvider* provide = new TDataProvider;
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);
		operand3->set_allocated_provider(provide);
	} else {
		TDataProvider* provide = new TDataProvider;
		TVariant* variant = new TVariant;
		TNumericValue* nuneric = new TNumericValue;
		nuneric->set_float_(std::any_cast<Vec3>(node.getSystemData("scale")).z);
		variant->set_allocated_numeric(nuneric);
		provide->set_allocated_variant(variant);
		operand3->set_allocated_provider(provide);
	}

	provider->set_allocated_operation(operation);
	scale->set_allocated_provider(provider);
	transform->set_allocated_scale(scale);
}

void OutputPtw::ModifyRotation(std::pair<std::string, std::string> curveProP, HmiWidget::TNodeTransform* transform) {
	auto rotation = transform->mutable_rotation();
	auto provider = rotation->mutable_provider();
	auto operation = provider->mutable_operation();
	if (curveProP.first.compare("rotation.x") == 0) {
		TDataBinding* operand = operation->mutable_operand(0);
		// has multi curves binding
		if (operand->has_key() && operand->has_provider()) {
			return;
		}
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);
	} else if (curveProP.first.compare("rotation.y") == 0) {
		TDataBinding* operand = operation->mutable_operand(1);
		// has multi curves binding
		if (operand->has_key() && operand->has_provider()) {
			return;
		}
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);

	} else if (curveProP.first.compare("rotation.z") == 0) {
		TDataBinding* operand = operation->mutable_operand(2);
		// has multi curves binding
		if (operand->has_key() && operand->has_provider()) {
			return;
		}
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);
	}
}

void OutputPtw::CreateRotation(std::pair<std::string, std::string> curveProP, HmiWidget::TNodeTransform* transform, raco::guiData::NodeData node) {
	TDataBinding* rotation = new TDataBinding;
	TDataProvider* provider = new TDataProvider;
	TOperation* operation = new TOperation;
	operation->set_operator_(TEOperatorType_MuxVec3);
	operation->add_datatype(TEDataType_Float);
	operation->add_datatype(TEDataType_Float);
	operation->add_datatype(TEDataType_Float);
	auto operand1 = operation->add_operand();
	auto operand2 = operation->add_operand();
	auto operand3 = operation->add_operand();
	if (curveProP.first.compare("rotation.x") == 0) {
		TDataProvider* provide = new TDataProvider;
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);
		operand1->set_allocated_provider(provide);
	} else {
		TDataProvider* provide = new TDataProvider;
		TVariant* variant = new TVariant;
		TNumericValue* nuneric = new TNumericValue;
		nuneric->set_float_(std::any_cast<Vec3>(node.getSystemData("rotation")).x);
		variant->set_allocated_numeric(nuneric);
		provide->set_allocated_variant(variant);
		operand1->set_allocated_provider(provide);
	}

	if (curveProP.first.compare("rotation.y") == 0) {
		TDataProvider* provide = new TDataProvider;
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);
		operand2->set_allocated_provider(provide);
	} else {
		TDataProvider* provide = new TDataProvider;
		TVariant* variant = new TVariant;
		TNumericValue* nuneric = new TNumericValue;
		nuneric->set_float_(std::any_cast<Vec3>(node.getSystemData("rotation")).y);
		variant->set_allocated_numeric(nuneric);
		provide->set_allocated_variant(variant);
		operand2->set_allocated_provider(provide);
	}

	if (curveProP.first.compare("rotation.z") == 0) {
		TDataProvider* provide = new TDataProvider;
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);
		operand3->set_allocated_provider(provide);
	} else {
		TDataProvider* provide = new TDataProvider;
		TVariant* variant = new TVariant;
		TNumericValue* nuneric = new TNumericValue;
		nuneric->set_float_(std::any_cast<Vec3>(node.getSystemData("rotation")).z);
		variant->set_allocated_numeric(nuneric);
		provide->set_allocated_variant(variant);
		operand3->set_allocated_provider(provide);
	}

	provider->set_allocated_operation(operation);
	rotation->set_allocated_provider(provider);
	transform->set_allocated_rotation(rotation);
}

size_t getArrIndex(std::string name) {
	std::string suffix = name.substr(name.length() - 2, 2);
	if (suffix == ".x") {
		return 0;
	} else if (suffix == ".y") {
		return 1;
	} else if (suffix == ".z") {
		return 2;
	} else if (suffix == ".w") {
		return 3;
	}
	return -1;
}

void OutputPtw::addOperandCurveRef2Operation(TOperation* operation, std::string curveName, std::string multiCurveName) {
	auto operand = operation->add_operand();
	if (multiCurveName == "") {
		TDataProvider* provider = new TDataProvider;
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveName);
		provider->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);
		operand->set_allocated_provider(provider);
	} else {
		TDataProvider* provider = new TDataProvider;
		TIdentifier* key = new TIdentifier;
		key->set_valuestring(multiCurveName);
		provider->set_source(TEProviderSource_IntModelValue);
		operand->set_allocated_key(key);
		operand->set_allocated_provider(provider);
	}
}


void OutputPtw::setUniformOperationByType(raco::guiData::Uniform& vecUniform, TOperation* operation, std::string* curveNameArr, std::string multiCurveOrAnimationName, bool isMultiAnimationSingleCurve) {
	auto usedUniformType = vecUniform.getType();
	switch (usedUniformType) {
		case raco::guiData::Vec2f:
			operation->set_operator_(TEOperatorType_MuxVec2);
			for (int i = 0; i < 2; ++i) {
				operation->add_datatype(TEDataType_Float);
				if (curveNameArr[i] == "") {
					float data = getUniformValueByType(vecUniform, i);
					addOperandOne2Operation(operation, data);
				} else {
					if (isMultiAnimationSingleCurve) {
						addOperandCurveRef2Operation(operation, curveNameArr[i], multiCurveOrAnimationName + "-" + curveNameArr[i]);
					}
					else {
						addOperandCurveRef2Operation(operation, curveNameArr[i], multiCurveOrAnimationName);
					}
				}
			}
			break;
		case raco::guiData::Vec3f:
			operation->set_operator_(TEOperatorType_MuxVec3);
			for (int i = 0; i < 3; ++i) {
				operation->add_datatype(TEDataType_Float);
				if (curveNameArr[i] == "") {
					float data = getUniformValueByType(vecUniform, i);
					addOperandOne2Operation(operation, data);
				} else {
					if (isMultiAnimationSingleCurve) {
						addOperandCurveRef2Operation(operation, curveNameArr[i], multiCurveOrAnimationName + "-" + curveNameArr[i]);
					} else {
						addOperandCurveRef2Operation(operation, curveNameArr[i], multiCurveOrAnimationName);
					}
				}
			}
			break;
		case raco::guiData::Vec4f:
			operation->set_operator_(TEOperatorType_MuxVec4);
			for (int i = 0; i < 4; ++i) {
				operation->add_datatype(TEDataType_Float);
				if (curveNameArr[i] == "") {
					float data = getUniformValueByType(vecUniform, i);
					addOperandOne2Operation(operation, data);
				} else {
					if (isMultiAnimationSingleCurve) {
						addOperandCurveRef2Operation(operation, curveNameArr[i], multiCurveOrAnimationName + "-" + curveNameArr[i]);
					} else {
						addOperandCurveRef2Operation(operation, curveNameArr[i], multiCurveOrAnimationName);
					}
				}
			}
			break;
		case raco::guiData::Vec2i:
			operation->set_operator_(TEOperatorType_MuxVec2);
			for (int i = 0; i < 2; ++i) {
				operation->add_datatype(TEDataType_Int);
				if (curveNameArr[i] == "") {
					float data = getUniformValueByType(vecUniform, i);
					addOperandOne2Operation(operation, data);
				} else {
					if (isMultiAnimationSingleCurve) {
						addOperandCurveRef2Operation(operation, curveNameArr[i], multiCurveOrAnimationName + "-" + curveNameArr[i]);
					} else {
						addOperandCurveRef2Operation(operation, curveNameArr[i], multiCurveOrAnimationName);
					}
				}
			}
			break;
		case raco::guiData::Vec3i:
			operation->set_operator_(TEOperatorType_MuxVec3);
			for (int i = 0; i < 3; ++i) {
				operation->add_datatype(TEDataType_Int);
				if (curveNameArr[i] == "") {
					float data = getUniformValueByType(vecUniform, i);
					addOperandOne2Operation(operation, data);
				} else {
					if (isMultiAnimationSingleCurve) {
						addOperandCurveRef2Operation(operation, curveNameArr[i], multiCurveOrAnimationName + "-" + curveNameArr[i]);
					} else {
						addOperandCurveRef2Operation(operation, curveNameArr[i], multiCurveOrAnimationName);
					}
				}
			}
			break;
		case raco::guiData::Vec4i:
			operation->set_operator_(TEOperatorType_MuxVec4);
			for (int i = 0; i < 4; ++i) {
				operation->add_datatype(TEDataType_Int);
				if (curveNameArr[i] == "") {
					float data = getUniformValueByType(vecUniform, i);
					addOperandOne2Operation(operation, data);
				} else {
					if (isMultiAnimationSingleCurve) {
						addOperandCurveRef2Operation(operation, curveNameArr[i], multiCurveOrAnimationName + "-" + curveNameArr[i]);
					} else {
						addOperandCurveRef2Operation(operation, curveNameArr[i], multiCurveOrAnimationName);
					}
				}
			}
			break;
		default:
			break;
	}
}

bool OutputPtw::isAddedUniform(std::string name, raco::guiData::NodeData* node) {
	auto re = nodeIDUniformsName_.find(node->objectID());
	if (re != nodeIDUniformsName_.end()) {
		for (auto& unName : re->second) {
			if (unName == name) {
				return true;
			}
		}
		re->second.push_back(name);
		return false;
	}
	std::vector<std::string> names;
	names.push_back(name);
	nodeIDUniformsName_.emplace(node->objectID(), names);
	return false;
}

bool findFromUniform(std::string property, std::string name) {
	QStringList propArr = QString::fromStdString(property).split(".");
	if (propArr[propArr.size() - 2].toStdString() == name) {
		return true;
	}
	return false;
}

void OutputPtw::addVecValue2Uniform(HmiWidget::TWidget* widget, std::pair<std::string, std::string> curveProP, HmiWidget::TNodeParam* nodeParam, raco::guiData::NodeData* node) {
	// set uniform name
	std::string uniformName = curveProP.first.substr(9, curveProP.first.length() - 11);

	if (isAddedUniform(uniformName, node)) {
		return;
	}

	// get uniforms
	bool isInUniforms = false;
	NodeMaterial nodeMaterial;
	raco::guiData::MaterialManager::GetInstance().getNodeMaterial(node->objectID(), nodeMaterial);
	// get weights
	raco::guiData::Uniform vecUniform;
	vecUniform.setType(UniformType::Null);
	std::vector<Uniform> uniforms = nodeMaterial.getUniforms();
	for (auto& un : uniforms) {
		if (un.getName() == uniformName) {
			vecUniform = un;
			isInUniforms = true;
			break;
		}
	}
	if (!isInUniforms) {
		return;
	}

	auto uniform = nodeParam->add_uniform();

	TDataBinding* name = new TDataBinding;
	TDataProvider* namePrivder = new TDataProvider;
	TVariant* variant = new TVariant;
	variant->set_asciistring(delUniformNamePrefix(uniformName));
	namePrivder->set_allocated_variant(variant);
	name->set_allocated_provider(namePrivder);
	uniform->set_allocated_name(name);

	// set uniform value
	TDataProvider* valProvder = new TDataProvider;
	TOperation* operation = new TOperation;
	TDataBinding* value = new TDataBinding;

	// get which weight used
	std::string curveNameArr[4] = {""};
	std::map<std::string, std::map<std::string, std::string>>& map = node->NodeExtendRef().curveBindingRef().bindingMap();
	for (auto& an : map) {
		for (auto& prop : an.second) {
			int index = -1;
			if (findFromUniform(prop.first, uniformName) && -1 != (index = getArrIndex(prop.first))) {
				curveNameArr[index] = prop.second;
			}
		}
	}
	// step2
	std::vector<std::map<std::string, CurvesSingleProp>> curves;
	bool hasMultiCurveSingleProp = hasMultiCurveOneProp(curveProP.first, node, curves);
	// step3
	std::string animaitionName;
	AnimationsSingleCurve aniSingleCurve;
	bool hasMultiAnimationSingleCurve = hasMultiAnimationOneCurve(curveProP.second, node, aniSingleCurve, animaitionName);

	if (!hasMultiCurveSingleProp && !hasMultiAnimationSingleCurve) {
		// set operation
		setUniformOperationByType(vecUniform, operation, curveNameArr);
	} else if (hasMultiCurveSingleProp && !hasMultiAnimationSingleCurve) {
		std::string multiCurveName = PTW_PRE_CURVES_ONE_PROP + (curves.at(0).begin())->second.curveName;
		setUniformOperationByType(vecUniform, operation, curveNameArr, multiCurveName);
		multiCurveBindingSinglePropSwitch(widget, curveProP.first, curves);
	} else if (!hasMultiCurveSingleProp && hasMultiAnimationSingleCurve) {
		std::string curveName = animaitionName + "-" + curveProP.second;
		bool isMultiAnimationSingleCurve = true;
		setUniformOperationByType(vecUniform, operation, curveNameArr, animaitionName, true);
		for (int i = 0; i < 4; ++i) {
			if (curveNameArr[i] != "") {
				addAnimationCurveSwitch(widget, animaitionName, curveNameArr[i], aniSingleCurve);
			}
		}
	}
	else { // Step4
		DEBUG(__FILE__, __FUNCTION__, __LINE__, "The Step4 of multi-animation needs to be improved.");
	}

	// add to value
	valProvder->set_allocated_operation(operation);
	value->set_allocated_provider(valProvder);
	uniform->set_allocated_value(value);
}

bool OutputPtw::isVecUniformValue(std::string name) {
	std::string suffix = name.substr(name.length() - 2, 2);
	if (suffix == ".x" || suffix == ".y" || suffix == ".z" || suffix == ".w") {
		return true;
	}
	return false;
}

void OutputPtw::addOperandOne2Operation(TOperation* operation, float data) {
	auto operand = operation->add_operand();
	TDataProvider* provider = new TDataProvider;
	TVariant* variant = new TVariant;
	TNumericValue* numeric = new TNumericValue;
	numeric->set_float_(data);
	variant->set_allocated_numeric(numeric);
	provider->set_allocated_variant(variant);

	operand->set_allocated_provider(provider);
}

void OutputPtw::AddUColorUniforms(HmiWidget::TNodeParam* nodeParam, NodeData* node) {
	auto it = NodeIDUColorNums_.find(node->objectID());
	if (it != NodeIDUColorNums_.end()) {
		for (auto num : it->second) {
			HmiWidget::TUniform unform;
			externalColorUniform(unform, num);
			auto uniformIt = nodeParam->add_uniform();
			*uniformIt = unform;
		}
	}

}

void OutputPtw::AddUniform(HmiWidget::TWidget* widget,std::pair<std::string, std::string> curveProP, HmiWidget::TNodeParam* nodeParam, raco::guiData::NodeData* node) {
	auto uniforms = nodeParam->uniform();
	for (auto un : uniforms) {
		std::string nameBinding = un.name().provider().variant().asciistring();
		if (nameBinding == delUniformNamePrefix(curveProP.first)) {
			return;
		}
	}
	if (!isVecUniformValue(curveProP.first)) {
		auto uniform = nodeParam->add_uniform();
		// set uniform name
		TDataBinding* name = new TDataBinding;
		TDataProvider* namePrivder = new TDataProvider;
		TVariant* variant = new TVariant;
		variant->set_asciistring(delUniformNamePrefix(curveProP.first));
		namePrivder->set_allocated_variant(variant);
		name->set_allocated_provider(namePrivder);
		uniform->set_allocated_name(name);

		std::vector<std::map<std::string, CurvesSingleProp>> curves;
		bool hasMultiCurveSingleProp = hasMultiCurveOneProp(curveProP.first, node, curves);

		std::string animaitionName;
		AnimationsSingleCurve aniSingleCurve;
		bool hasMultiAnimationSingleCurve = hasMultiAnimationOneCurve(curveProP.second, node, aniSingleCurve, animaitionName);

		if (!hasMultiCurveSingleProp && !hasMultiAnimationSingleCurve) {
			// set uniform value
			TDataBinding* value = new TDataBinding;
			TDataProvider* privder = new TDataProvider;
			TIdentifier* curveReference = new TIdentifier;
			curveReference->set_valuestring(curveProP.second);
			privder->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);
			value->set_allocated_provider(privder);
			uniform->set_allocated_value(value);
		} else if (hasMultiCurveSingleProp && !hasMultiAnimationSingleCurve) {
			TDataBinding* value = new TDataBinding;
			TIdentifier* key = new TIdentifier;
			TDataProvider* provider = new TDataProvider;
			key->set_valuestring(PTW_PRE_CURVES_ONE_PROP + (curves.at(0).begin())->second.curveName);
			provider->set_source(TEProviderSource_IntModelValue);
			value->set_allocated_key(key);
			value->set_allocated_provider(provider);
			uniform->set_allocated_value(value);

			multiCurveBindingSinglePropSwitch(widget, curveProP.first, curves);
		} else if (!hasMultiCurveSingleProp && hasMultiAnimationSingleCurve) {
			std::string curveName = animaitionName + "-" + curveProP.second;
			TDataBinding* value = new TDataBinding;
			TIdentifier* key = new TIdentifier;
			TDataProvider* provider = new TDataProvider;
			key->set_valuestring(curveName);
			provider->set_source(TEProviderSource_IntModelValue);
			value->set_allocated_key(key);
			value->set_allocated_provider(provider);
			uniform->set_allocated_value(value);
			addAnimationCurveSwitch(widget, animaitionName, curveProP.second, aniSingleCurve);
		} else {
			DEBUG(__FILE__, __FUNCTION__, __LINE__, "The Step4 of multi-animation needs to be improved.");
			qDebug() << "Step4";
		}
	} else {
		addVecValue2Uniform(widget, curveProP, nodeParam, node);
	}
}

// add external Animaiton name
void OutputPtw::animationSwitchPreData(HmiWidget::TWidget* widget) {
	auto animations = animationDataManager::GetInstance().getAnitnList();
	auto activeAnimation = animationDataManager::GetInstance().GetActiveAnimation();
	int n = 1;
	for (auto an : animations) {
		// externalAnimationName
		HmiWidget::TExternalModelParameter* externalModelValue = widget->add_externalmodelvalue();
		externalModelValue->set_allocated_key(assetsFun_.Key(an.first));
		if (an.first == activeAnimation) {
			externalModelValue->set_allocated_variant(assetsFun_.VariantNumeric(1.0));
		} else {
			externalModelValue->set_allocated_variant(assetsFun_.VariantNumeric(0.0));
		}
		{// Greater0
			HmiWidget::TInternalModelParameter* internalModelCompare = widget->add_internalmodelvalue();
			TDataBinding Operand1;
			Operand1.set_allocated_key(assetsFun_.Key(an.first));
			Operand1.set_allocated_provider(assetsFun_.ProviderSrc(TEProviderSource_ExtModelValue));
			TDataBinding Operand2;
			Operand2.set_allocated_provider(assetsFun_.ProviderNumeric(0));
			assetsFun_.CompareOperation(internalModelCompare, an.first + "Greater0", Operand1, TEDataType_Float, Operand2, TEDataType_Float, TEOperatorType_Greater);
		}
		{  // IfThenElse
			HmiWidget::TInternalModelParameter* internalModelIfThenElse = widget->add_internalmodelvalue();
			TDataBinding Operand1;
			Operand1.set_allocated_key(assetsFun_.Key(an.first + "Greater0"));
			Operand1.set_allocated_provider(assetsFun_.ProviderSrc(TEProviderSource_IntModelValue));
			TDataBinding Operand2;
			Operand2.set_allocated_provider(assetsFun_.ProviderNumericInt(n));
			TDataBinding Operand3;
			Operand3.set_allocated_provider(assetsFun_.ProviderNumericInt(0));
			assetsFun_.IfThenElse(internalModelIfThenElse, an.first + "Value", Operand1, TEDataType_Bool, Operand2, TEDataType_Int, Operand3, TEDataType_Int);
		}
		n++;
	}
}

// sum animationValue
void OutputPtw::sumAnimationValue(HmiWidget::TWidget* widget) {
	auto animations = animationDataManager::GetInstance().getAnitnList();
	std::vector<TDataBinding> Operands;
	HmiWidget::TInternalModelParameter* internalModelAdd = widget->add_internalmodelvalue();
	for (auto an : animations) {
		TDataBinding Operand;
		Operand.set_allocated_key(assetsFun_.Key(an.first + "Value"));
		Operand.set_allocated_provider(assetsFun_.ProviderSrc(TEProviderSource_IntModelValue));
		Operands.push_back(Operand);
	}
	if (Operands.size() == 1) {
		TDataBinding Operand;
		Operand.set_allocated_provider(assetsFun_.ProviderNumericInt(0));
		Operands.push_back(Operand);
	}else if (Operands.size() > 7) {
		DEBUG(__FILE__, __FUNCTION__, __LINE__, "animations number > 7!");
	}
	assetsFun_.operatorOperands(internalModelAdd, "AnimationSumValue", TEDataType_Int, Operands, TEOperatorType_Add);
}

// switch animation
void OutputPtw::animationSwitch(HmiWidget::TWidget* widget) {
	PTWSwitch data;
	data.outPutKey = "UsedAnimationName";
	data.dataType1 = TEDataType_Int;
	data.dataType2 = TEDataType_AsciiString;

	TDataBinding Operand;
	Operand.set_allocated_key(assetsFun_.Key("AnimationSumValue"));
	Operand.set_allocated_provider(assetsFun_.ProviderSrc(TEProviderSource_IntModelValue));
	data.Operands.push_back(Operand);

	auto animations = animationDataManager::GetInstance().getAnitnList();
	int n = 1;
	for (auto an : animations) {
		TDataBinding Operand1;
		Operand1.set_allocated_provider(assetsFun_.ProviderNumericInt(n));
		data.Operands.push_back(Operand1);

		TDataBinding Operand2;
		Operand2.set_allocated_provider(assetsFun_.ProviderAsciiString(an.first));
		data.Operands.push_back(Operand2);
		n++;
	}
	TDataBinding OperandDefault;
	OperandDefault.set_allocated_provider(assetsFun_.ProviderAsciiString(animations.begin()->first));
	data.Operands.push_back(OperandDefault);

	HmiWidget::TInternalModelParameter* internalModelswitch = widget->add_internalmodelvalue();
	assetsFun_.SwitchAnimation(internalModelswitch, data);
}

void OutputPtw::switchAnimations(HmiWidget::TWidget* widget) {
	// each animation
	animationSwitchPreData(widget);
	// sum value
	sumAnimationValue(widget);
	// switch animation
	animationSwitch(widget);
}

// add exteral scale
void OutputPtw::externalScale(HmiWidget::TWidget* widget) {
	HmiWidget::TExternalModelParameter* externalModelValue = widget->add_externalmodelvalue();
	assetsFun_.externalKeyVariant(externalModelValue, PTW_EX_SCALE_NAME, assetsFun_.VariantNumeric(300.0));
}

void OutputPtw::externalScaleData(HmiWidget::TWidget* widget) {
	if (!NodeScaleSize_) {
		DEBUG(__FILE__, __FUNCTION__, __LINE__, "NodeScaleSize_ is 0!");
	}

	//  add exteral scale
	externalScale(widget);

	// add scale Div 300
	auto internalModelValue = widget->add_internalmodelvalue();
	internalModelValue->set_allocated_key(assetsFun_.Key(PTW_SCALE_DIVIDE_VALUE));
	internalModelValue->set_allocated_binding(assetsFun_.BindingValueStrNumericOperatorType(PTW_EX_SCALE_NAME, TEProviderSource_ExtModelValue, 300, TEOperatorType_Div));

	// add scale Mul NodeScaleSize_
	internalModelValue = widget->add_internalmodelvalue();
	internalModelValue->set_allocated_key(assetsFun_.Key(PTW_SCALE_DIV_MUL_VALUE));
	internalModelValue->set_allocated_binding(assetsFun_.BindingValueStrNumericOperatorType(PTW_SCALE_DIVIDE_VALUE, TEProviderSource_IntModelValue, NodeScaleSize_, TEOperatorType_Mul));

	// add nodeParam of Node scale
	HmiWidget::TNodeParam* nodeParam = widget->add_nodeparam();
	assetsFun_.NodeParamAddIdentifier(nodeParam, SceneChildName_);
	assetsFun_.NodeParamAddNode(nodeParam, SceneChildName_);
	auto transform = nodeParam->mutable_transform();
	TDataBinding operandX;
	assetsFun_.OperandKeySrc(operandX, PTW_SCALE_DIV_MUL_VALUE, TEProviderSource_IntModelValue);
	TDataBinding operandY;
	assetsFun_.OperandKeySrc(operandY, PTW_SCALE_DIV_MUL_VALUE, TEProviderSource_IntModelValue);
	TDataBinding operandZ;
	assetsFun_.OperandKeySrc(operandZ, PTW_SCALE_DIV_MUL_VALUE, TEProviderSource_IntModelValue);
	assetsFun_.TransformCreateScale(transform, operandX, operandY, operandZ);
}

// add exteral Opacity
void OutputPtw::externalOpacity(HmiWidget::TWidget* widget) {
	HmiWidget::TExternalModelParameter* externalModelValue = widget->add_externalmodelvalue();
	externalModelValue->set_allocated_key(assetsFun_.Key(PTW_EX_OPACITY_NAME));
	externalModelValue->set_allocated_variant(assetsFun_.VariantNumeric(1.0));
}

void OutputPtw::createResourceParam(HmiWidget::TWidget* widget, std::string materialName) {
	// identifier
	HmiWidget::TResourceParam* resParam = widget->add_resourceparam();
	assetsFun_.ResourceParamAddIdentifier(resParam, materialName + "_Opacity");	 // test
	// resource
	assetsFun_.ResourceParamAddResource(resParam, materialName);
	// appearance
	HmiWidget::TAppearanceParam* appearance = new HmiWidget::TAppearanceParam;
	assetsFun_.AddUniform2Appearance(appearance, PTW_FRAG_SYS_OPACITY, PTW_EX_OPACITY_NAME, TEProviderSource_ExtModelValue);
	resParam->set_allocated_appearance(appearance);
}

void OutputPtw::externalOpacityData(HmiWidget::TWidget* widget) {
	if (HasOpacityMaterials_.empty()) {
		return ;
	}
	// external Opacity
	externalOpacity(widget);

	// resource Param
	for (auto materialName : HasOpacityMaterials_) {
		createResourceParam(widget, materialName);
	}
}

void OutputPtw::externalAnimation(HmiWidget::TWidget* widget) {
	HmiWidget::TExternalModelParameter* externalModelValue = widget->add_externalmodelvalue();
	externalModelValue->set_allocated_key(assetsFun_.Key(PTW_EX_TRIGGLE_ANIMATION));
	externalModelValue->set_allocated_variant(assetsFun_.VariantNumeric(1.0));
}

void OutputPtw::externalColorData(HmiWidget::TWidget* widget) {
	if (uColorNums_.empty()) {
		return ;
	}
	for (int i : uColorNums_) {
		// CONTENT..._Ci
		auto externalModelValueCi = widget->add_externalmodelvalue();
		assetsFun_.ColorIPAIconExternal(externalModelValueCi, PTW_EX_CON_IPA_ICON_C + std::to_string(i));
	}

	for (int i : uColorNums_) {
		// HUD_CONTENT..._Ci
		auto HUD_ExternalModelValueCi = widget->add_externalmodelvalue();
		assetsFun_.ColorIPAIconExternal(HUD_ExternalModelValueCi, PTW_EX_HUD_CON_IPA_ICON_C + std::to_string(i));
	}

	// HUD
	auto externalModelValue = widget->add_externalmodelvalue();
	externalModelValue->set_allocated_key(assetsFun_.Key(PTW_EX_HUD_NAME));
	externalModelValue->set_allocated_variant(assetsFun_.VariantNumeric(0));

	// CONTENT..._Ci_V4
	for (int i : uColorNums_) {
		auto internalModelValueCiV4 = widget->add_internalmodelvalue();
		assetsFun_.ColorIPAIconInternal(internalModelValueCiV4, PTW_EX_CON_IPA_ICON_C + std::to_string(i) + PTW_IN_V4, PTW_EX_CON_IPA_ICON_C + std::to_string(i));
	}
	//	HUD_CONTENT..._Ci_V4
	for (int i : uColorNums_) {
		auto HUB_InternalModelValueCiV4 = widget->add_internalmodelvalue();
		assetsFun_.ColorIPAIconInternal(HUB_InternalModelValueCiV4, PTW_EX_HUD_NAME + "_" + PTW_EX_CON_IPA_ICON_C + std::to_string(i) + PTW_IN_V4, PTW_EX_HUD_NAME + "_" + PTW_EX_CON_IPA_ICON_C + std::to_string(i));
	}

	// ColorMode
	for (int i : uColorNums_) {
		auto InternalColorModelV4 = widget->add_internalmodelvalue();
		assetsFun_.ColorModeMixInternal(InternalColorModelV4, PTW_IN_COLOR_MODE + std::to_string(i), PTW_EX_CON_IPA_ICON_C + std::to_string(i) + PTW_IN_V4, PTW_EX_HUD_CON_IPA_ICON_C + std::to_string(i) + PTW_IN_V4, PTW_EX_HUD_NAME);
	}
}

void OutputPtw::externalColorUniform(HmiWidget::TUniform& tUniform, int index) {
	assetsFun_.CreateHmiWidgetUniform(&tUniform, PTW_IN_U_COLOR + std::to_string(index), PTW_IN_COLOR_MODE + std::to_string(index) ,TEProviderSource_IntModelValue);
}

// DurationValue
void OutputPtw::internalDurationValue(HmiWidget::TWidget* widget) {
	PTWSwitch data;
	data.outPutKey = "DurationValue";
	data.dataType1 = TEDataType_Identifier;
	data.dataType2 = TEDataType_UInt;

	TDataBinding Operand;
	Operand.set_allocated_key(assetsFun_.Key("UsedAnimationName"));
	Operand.set_allocated_provider(assetsFun_.ProviderSrc(TEProviderSource_IntModelValue));
	data.Operands.push_back(Operand);

	auto animations = animationDataManager::GetInstance().getAnitnList();
	for (auto an : animations) {
		unsigned int durationTime = an.second.GetUpdateInterval() * (an.second.GetEndTime() - an.second.GetStartTime());
		TDataBinding Operand1;
		assetsFun_.OperandProVarIdentAndType(Operand1, an.first, TEIdentifierType_ParameterValue);
		data.Operands.push_back(Operand1);

		TDataBinding Operand2;
		Operand2.set_allocated_provider(assetsFun_.ProviderNumericUInt(durationTime));
		data.Operands.push_back(Operand2);
	}
	TDataBinding OperandDefault;
	OperandDefault.set_allocated_provider(assetsFun_.ProviderNumericUInt(8000));
	data.Operands.push_back(OperandDefault);

	HmiWidget::TInternalModelParameter* internalModelswitch = widget->add_internalmodelvalue();
	assetsFun_.SwitchAnimation(internalModelswitch, data);
}

}  // namespace raco::dataConvert
