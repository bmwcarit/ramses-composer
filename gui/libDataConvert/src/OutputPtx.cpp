
#include "data_Convert/OutputPtx.h"
#include "data_Convert/ProgramDefine.h"
#include "PropertyData/PropertyType.h"
#include "style/Icons.h"

#include <set>
#include <QMessageBox>


namespace raco::dataConvert {
using namespace raco::style;

std::string OutputPtx::delNodeNameSuffix(std::string nodeName) {
	int index = nodeName.rfind(".objectID");
	if (-1 != index)
		nodeName = nodeName.substr(0, nodeName.length() - 9);
	return nodeName;
}

void OutputPtx::setMeshBaseNode(NodeData* node, HmiScenegraph::TNode* baseNode) {
	std::string nodeName = node->getName();
	int index = nodeName.rfind(".objectID");
	if (-1 != index)
		nodeName = nodeName.substr(0, nodeName.length() - 9);
	std::string baseNodeName = nodeName + "Shape";
	baseNode->set_name(baseNodeName);

	TVector3f* scale = new TVector3f();
	scale->set_x(1.0);
	scale->set_y(1.0);
	scale->set_z(1.0);
	baseNode->set_allocated_scale(scale);

	TVector3f* rotation = new TVector3f();
	rotation->set_x(0.0);
	rotation->set_y(0.0);
	rotation->set_z(0.0);
	baseNode->set_allocated_rotation(rotation);

	TVector3f* translation = new TVector3f();
	translation->set_x(0.0);
	translation->set_y(0.0);
	translation->set_z(0.0);
	baseNode->set_allocated_translation(translation);
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


void OutputPtx::setPtxTMesh(NodeData* node, HmiScenegraph::TMesh& tMesh) {
	// set baseNode data
	HmiScenegraph::TNode* baseNode = new HmiScenegraph::TNode();
	setMeshBaseNode(node, baseNode);
	tMesh.set_allocated_basenode(baseNode);

	MaterialData materialData;
	MeshData meshData;

	if(raco::guiData::MeshDataManager::GetInstance().getMeshData(node->objectID(), meshData)){
		//setMeshUniform(node, meshData);
		// set meshresource
		tMesh.set_meshresource(meshData.getMeshUri());
		// usedAttributes
		if (raco::guiData::MaterialManager::GetInstance().getMaterialData(node->objectID(), materialData)) {
			for (auto& attr : materialData.getUsedAttributes()) {
				HmiScenegraph::TMesh_TAttributeParamteter tempAttr;
				tempAttr.set_name(attr.name);
				HmiScenegraph::TMesh_TAttributeParamteter* itAttr = tMesh.add_attributeparameter();
				*itAttr = tempAttr;
			}
			tMesh.set_materialreference(materialData.getObjectName());

			// TODO: uniforms for mesh
			for (auto& uniform : node->getUniforms()) {
				HmiScenegraph::TUniform tUniform;
				uniformTypeValue(uniform, tUniform);
				HmiScenegraph::TUniform* itMesh = tMesh.add_uniform();
				*itMesh = tUniform;
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

void OutputPtx::setPtxNode(NodeData* childNode, HmiScenegraph::TNode& hmiNode) {
    std::string nodeName = childNode->getName();
	int index = nodeName.rfind(".objectID");

	if (-1 != index)
		nodeName = nodeName.substr(0, nodeName.length() - 9);
	hmiNode.set_name(nodeName);

	// Do not export camera data for now
  //  if (nodeName == "PerspectiveCamera") {
		//setPtxTCamera(childNode, hmiNode);
  //  }

	if (childNode->hasSystemData("scale")) {
		TVector3f* scale = new TVector3f();
		Vec3 scal = std::any_cast<Vec3>(childNode->getSystemData("scale"));
		scale->set_x(scal.x);
		scale->set_y(scal.y);
		scale->set_z(scal.z);
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
    // renderorder and childSortOrderRank
    hmiNode.set_renderorder(0);
	hmiNode.set_childsortorderrank(0);

    // mesh
	HmiScenegraph::TMesh mesh;
	setPtxTMesh(childNode, mesh);
	HmiScenegraph::TMesh* it = hmiNode.add_mesh();
	*it = mesh;
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

void OutputPtx::writeNodePtx(NodeData* pNode, HmiScenegraph::TNode* parent) {
	if (!pNode)
		return;
	HmiScenegraph::TNode hmiNode;
	setPtxNode(pNode, hmiNode);
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
	tColorWrite->set_alpha(colorWrite.getAlpha());
	tColorWrite->set_blue(colorWrite.getBlue());
	tColorWrite->set_green(colorWrite.getGreen());
	tColorWrite->set_red(colorWrite.getRed());
	rRenderMode->set_allocated_colorwrite(tColorWrite);
}

void OutputPtx::uniformTypeValue(Uniform data, HmiScenegraph::TUniform& tUniform) {
	tUniform.set_name(data.getName());
	TNumericValue* tNumericValue = new TNumericValue();
	UniformType dataType = data.getType();
	switch (dataType) {
		case raco::guiData::Null:
			// Do not have
			break;
		case raco::guiData::Bool: {
			uint32_t temp = std::any_cast<bool>(data.getValue());
			tNumericValue->set_uint(temp);
			tUniform.set_allocated_value(tNumericValue);
			tUniform.set_type(TENumericType::TENumericType_unsignedInteger);
			break;
		}
		case raco::guiData::Int: {
			int temp = std::any_cast<int>(data.getValue());
			tNumericValue->set_int_(temp);
			tUniform.set_allocated_value(tNumericValue);
			tUniform.set_type(TENumericType::TENumericType_unsignedInteger);
			break;
		}
		case raco::guiData::Double: {
			float temp = std::any_cast<double>(data.getValue());
			tNumericValue->set_float_(temp);
			tUniform.set_allocated_value(tNumericValue);
			tUniform.set_type(TENumericType::TENumericType_float);
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
		case raco::guiData::Struct:
			// Do not have
			break;
		default:
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

void OutputPtx::messageBoxError(std::string materialName) {
	if (isOutputError_) {
		return;
	}

	QMessageBox customMsgBox;
	customMsgBox.setWindowTitle("Warning message box");
	QPushButton* okButton = customMsgBox.addButton("OK", QMessageBox::ActionRole);
	//QPushButton* cancelButton = customMsgBox.addButton(QMessageBox::Cancel);
	customMsgBox.setIcon(QMessageBox::Icon::Warning);
	QString text = QString::fromStdString(materialName) + "\" has an empty texture !";
	text = "Warning: Material \"" + text;
	customMsgBox.setText(text);
	customMsgBox.exec();

	if (customMsgBox.clickedButton() == (QAbstractButton *)(okButton)) {
		isOutputError_ = true;
	}
}


void OutputPtx::writeMaterial2MaterialLib(HmiScenegraph::TMaterialLib* materialLibrary) {
	std::map<std::string, MaterialData> materialMap = raco::guiData::MaterialManager::GetInstance().getMaterialDataMap();
	std::set<std::string> setNameArr;
	for (auto& material : materialMap) {
		// whether it has been stored?
		if (isStored(material.second.getObjectName(), setNameArr)) {
			continue;
		}
		MaterialData data = material.second;
		HmiScenegraph::TMaterial tMaterial;
		// name
		tMaterial.set_name(data.getObjectName());

		// RenderMode
		HmiScenegraph::TRenderMode* rRenderMode = new HmiScenegraph::TRenderMode();
		RenderMode renderMode = data.getRenderMode();
		setMaterialRenderMode(renderMode, rRenderMode);
		tMaterial.set_allocated_rendermode(rRenderMode);

		// shaderReference
		std::string shaderPtxName = getShaderPtxNameByShaderName(data.getShaderRef());
		tMaterial.set_shaderreference(shaderPtxName);

		for (auto& textureData : data.getTextures()) {
			HmiScenegraph::TTexture tTextture;
			if (textureData.getName() == "empty") {
				messageBoxError(data.getObjectName());
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
		QString desPath = filePath + "/" + shadersPathName;
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
	// root
	NodeData* rootNode = &(raco::guiData::NodeDataManager::GetInstance().root());
	HmiScenegraph::TScene scene;

    HmiScenegraph::TNode* tRoot = new HmiScenegraph::TNode();
	tRoot->set_name(PTX_SCENE_NAME.toStdString());
	setRootSRT(tRoot);

    for (auto& child : rootNode->childMapRef()) {
		NodeData* childNode = &(child.second);
        writeNodePtx(childNode, tRoot);
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

	if (isOutputError_) {
		QFile::remove(filePath + "/scene.ptx");
		isOutputError_ = false;
		return false;
	}

	return true;
}

std::string OutputPtw::ConvertAnimationInfo(HmiWidget::TWidget* widget) {
	std::string animation_interal;
	auto animationList = raco::guiData::animationDataManager::GetInstance().getAnitnList();
	for (auto animation : animationList) {
		auto externalModelValue = widget->add_externalmodelvalue();
		TIdentifier* key_ext = new TIdentifier;

		key_ext->set_valuestring(animation.first + "_extenal");
		externalModelValue->set_allocated_key(key_ext);
		TVariant* variant = new TVariant;
		TNumericValue* Numeric = new TNumericValue();
		Numeric->set_float_(0.0);
		variant->set_allocated_numeric(Numeric);
		externalModelValue->set_allocated_variant(variant);
		auto internalModelValue = widget->add_internalmodelvalue();
		TIdentifier* key_int = new TIdentifier;
		animation_interal = animation.first + "_interal";
		key_int->set_valuestring(animation_interal);
		internalModelValue->set_allocated_key(key_int);
		TDataBinding* binding = new TDataBinding;
		TDataProvider* provider = new TDataProvider;
		TOperation* operation = new TOperation;

		operation->set_operator_(TEOperatorType_Mul);
		operation->add_datatype(TEDataType_Float);
		operation->add_datatype(TEDataType_Float);

		auto operand1 = operation->add_operand();
		TIdentifier* key = new TIdentifier;
		key->set_valuestring(animation.first + "_extenal");
		operand1->set_allocated_key(key);
		TDataProvider* provider1 = new TDataProvider;
		provider1->set_source(TEProviderSource_ExtModelValue);
		operand1->set_allocated_provider(provider1);

		auto operand2 = operation->add_operand();
		TDataProvider* provider2 = new TDataProvider;
		TVariant* variant1 = new TVariant;
		TNumericValue* numeric = new TNumericValue;
		numeric->set_float_(float(animation.second.GetEndTime()));
		variant1->set_allocated_numeric(numeric);
		provider2->set_allocated_variant(variant1);

		operand2->set_allocated_provider(provider2);
		provider->set_allocated_operation(operation);
		binding->set_allocated_provider(provider);
		internalModelValue->set_allocated_binding(binding);
	}
	return animation_interal;
}

void OutputPtw::ConvertCurveInfo(HmiWidget::TWidget* widget, std::string animation_interal) {
	for (auto curveData : raco::guiData::CurveManager::GetInstance().getCurveList()) {
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
			}
			else if (pointData->getInterPolationType() == raco::guiData::HERMIT_SPLINE) {
				TCurvePointInterpolation* incommingInterpolation = new TCurvePointInterpolation;
				incommingInterpolation->set_interpolation(TCurvePointInterpolationType_Hermite);
				TMultidimensionalPoint* lefttangentVector = new TMultidimensionalPoint;
				lefttangentVector->set_domain(0.0);
				TNumericValue* leftValue = new TNumericValue;
				leftValue->set_float_(std::any_cast<double>(pointData->getLeftTagent()));
				lefttangentVector->set_allocated_value(leftValue);
				incommingInterpolation->set_allocated_tangentvector(lefttangentVector);
				point->set_allocated_incomminginterpolation(incommingInterpolation);

				TCurvePointInterpolation* outgoingInterpolation = new TCurvePointInterpolation;
				outgoingInterpolation->set_interpolation(TCurvePointInterpolationType_Hermite);
				TMultidimensionalPoint* RighttangentVector = new TMultidimensionalPoint;
				RighttangentVector->set_domain(0.0);
				TNumericValue* RightValue = new TNumericValue;
				RightValue->set_float_(std::any_cast<double>(pointData->getRightTagent()));
				RighttangentVector->set_allocated_value(RightValue);
				incommingInterpolation->set_allocated_tangentvector(RighttangentVector);
				point->set_allocated_outgoinginterpolation(outgoingInterpolation);
			}
		}
		curve->set_allocated_curvedefinition(curveDefinition);
	}
}

void OutputPtw::ConvertBind(HmiWidget::TWidget* widget, raco::guiData::NodeData& node) {
	if (0 != node.getBindingySize()) {
		HmiWidget::TNodeParam* nodeParam = widget->add_nodeparam();
		TIdentifier* identifier = new TIdentifier;
		identifier->set_valuestring(node.getName());
		nodeParam->set_allocated_identifier(identifier);
		TDataBinding* paramnode = new TDataBinding;
		TDataProvider* provider = new TDataProvider;
		TVariant* variant = new TVariant;
		variant->set_asciistring(node.getName());
		provider->set_allocated_variant(variant);
		paramnode->set_allocated_provider(provider);
		nodeParam->set_allocated_node(paramnode);
		for (auto cuvebindList : node.NodeExtendRef().curveBindingRef().bindingMap()) {
			for (auto curveProP : cuvebindList.second) {
				if (curveProP.first.find("translation") == 0) {
					if (nodeParam->has_transform()) {
						auto transform = nodeParam->mutable_transform();
						if (transform->has_translation()) {
							ModifyTranslation(curveProP, transform);
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
				} else {
					AddUniform(curveProP, nodeParam);
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

void OutputPtw::WriteAsset(std::string filePath) {
	filePath = filePath.substr(0, filePath.find(".rca"));
	HmiWidget::TWidgetCollection widgetCollection;
	HmiWidget::TWidget* widget = widgetCollection.add_widget();
	std::string animation_interal = ConvertAnimationInfo(widget);
	ConvertCurveInfo(widget, animation_interal);
	ConvertBind(widget, NodeDataManager::GetInstance().root());
	std::string output;
	google::protobuf::TextFormat::PrintToString(widgetCollection, &output);
	std::cout << output << std::endl;

	QDir* folder = new QDir;
	if (!folder->exists(QString::fromStdString(filePath))) {
		bool ok = folder->mkpath(QString::fromStdString(filePath));
	}
	delete folder;
	std::ofstream outfile;
	outfile.open(filePath + "/widget.ptw", std::ios_base::out | std::ios_base::trunc);
	outfile << output << std::endl;
	outfile.close();
}

void OutputPtw::ModifyTranslation(std::pair<std::string, std::string> curveProP, HmiWidget::TNodeTransform* transform) {
	auto translation = transform->mutable_translation();
	auto provider = translation->mutable_provider();
	auto operation = provider->mutable_operation();
	if (curveProP.first.compare("translation.x") == 0) {
		TDataBinding* operand = operation->mutable_operand(0);
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);
	} else if (curveProP.first.compare("translation.y") == 0) {
		TDataBinding* operand = operation->mutable_operand(1);
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);

	} else if (curveProP.first.compare("translation.z") == 0) {
		TDataBinding* operand = operation->mutable_operand(2);
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);
	}
}
void OutputPtw::CreateTranslation(std::pair<std::string, std::string> curveProP, HmiWidget::TNodeTransform* transform , raco::guiData::NodeData node) {
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
		float a = std::any_cast<Vec3>(node.getSystemData("translation")).y;
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
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);
	} else if (curveProP.first.compare("scale.y") == 0) {
		TDataBinding* operand = operation->mutable_operand(1);
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);

	} else if (curveProP.first.compare("scale.z") == 0) {
		TDataBinding* operand = operation->mutable_operand(2);
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
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);
	} else if (curveProP.first.compare("rotation.y") == 0) {
		TDataBinding* operand = operation->mutable_operand(1);
		operand->clear_provider();
		TDataProvider* provide = operand->mutable_provider();
		TIdentifier* curveReference = new TIdentifier;
		curveReference->set_valuestring(curveProP.second);
		provide->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);

	} else if (curveProP.first.compare("rotation.z") == 0) {
		TDataBinding* operand = operation->mutable_operand(2);
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
		float a = std::any_cast<Vec3>(node.getSystemData("rotation")).y;
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
void OutputPtw::AddUniform(std::pair<std::string, std::string> curveProP, HmiWidget::TNodeParam* nodeParam) {
	auto uniform = nodeParam->add_uniform();
	TDataBinding* name = new TDataBinding;
	TDataProvider* namePrivder = new TDataProvider;
	TVariant* variant = new TVariant;
	variant->set_asciistring(curveProP.first);
	namePrivder->set_allocated_variant(variant);
	name->set_allocated_provider(namePrivder);
	uniform->set_allocated_name(name);
	TDataBinding* value = new TDataBinding;
	TDataProvider* privder = new TDataProvider;
	TIdentifier* curveReference = new TIdentifier;
	curveReference->set_valuestring(curveProP.second);
	privder->MutableExtension(HmiWidget::curve)->set_allocated_curvereference(curveReference);
	value->set_allocated_provider(privder);
	uniform->set_allocated_value(value);
}
}  // namespace raco::dataConvert
