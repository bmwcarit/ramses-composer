
#include "data_Convert/OutputPtx.h"
#include "data_Convert/ProgramDefine.h"

#include "proto/Numeric.pb.h"
#include "proto/Common.pb.h"
#include "proto/Scenegraph.pb.h"
#include "proto/HmiWidget.pb.h"
#include "proto/HmiBase.pb.h"
#include <google/protobuf/text_format.h>

namespace raco::dataConvert {

	std::string delNodeNameSuffix(std::string nodeName) {

	int index = nodeName.rfind(".objectID");
	if (-1 != index)
		nodeName = nodeName.substr(0, nodeName.length() - 9);
	return nodeName;
	}


void OutputPtx::setMeshBaseNode(NodeData* node, HmiScenegraph::TNode* baseNode) {
	// 1. name
	std::string nodeName = node->getName();
	int index = nodeName.rfind(".objectID");
	if (-1 != index)
		nodeName = nodeName.substr(0, nodeName.length() - 9);
	std::string baseNodeName = nodeName + "Shape";
	baseNode->set_name(baseNodeName);

	// 1. scale
	TVector3f* scale = new TVector3f();
	scale->set_x(1.0);
	scale->set_y(1.0);
	scale->set_z(1.0);
	baseNode->set_allocated_scale(scale);
	// 2. rotation
	TVector3f* rotation = new TVector3f();
	rotation->set_x(0.0);
	rotation->set_y(0.0);
	rotation->set_z(0.0);
	baseNode->set_allocated_rotation(rotation);
	// 3. translation
	TVector3f* translation = new TVector3f();
	translation->set_x(0.0);
	translation->set_y(0.0);
	translation->set_z(0.0);
	baseNode->set_allocated_translation(translation);
}

void OutputPtx::setPtxTMesh(NodeData* node, HmiScenegraph::TMesh& tMesh) {
	// 1. set baseNode data
	HmiScenegraph::TNode* baseNode = new HmiScenegraph::TNode();
	setMeshBaseNode(node, baseNode);
	tMesh.set_allocated_basenode(baseNode);

	MeshData meshData;
	if(raco::guiData::MeshDataManager::GetInstance().getMeshData(node->objectID(), meshData)){
		// 2. set meshresource
		tMesh.set_meshresource(meshData.getMeshUri());
		// 4. attributes
		//for 循环重写
		for (auto& attr : meshData.getAttributes()) {
			HmiScenegraph::TMesh_TAttributeParamteter tempAttr;
			tempAttr.set_name(attr.name);
			HmiScenegraph::TMesh_TAttributeParamteter* itAttr = tMesh.add_attributeparameter();
			*itAttr = tempAttr;
		}
	}

	MaterialData materialData;
	if (raco::guiData::MaterialManager::GetInstance().getMaterialData(node->objectID(), materialData)) {
		tMesh.set_materialreference(materialData.getObjectName());
	}
	
}

void OutputPtx::setPtxTCamera(NodeData* childNode, HmiScenegraph::TNode& hmiNode) {
    // camera为默认值，因此先不用Node信息
	Q_UNUSED(childNode);
	HmiScenegraph::TCamera* camera = new HmiScenegraph::TCamera();
	camera->set_horizontalfov(0.7);
	camera->set_aspectratio(1.0);
	camera->set_nearplane(0.01);
	camera->set_farplane(100.0); 
	camera->set_projectiontype(HmiScenegraph::TECameraProjectionType::TECameraProjectionType_FOV);
    // 接收一个在堆区的指针，不用销毁，否则崩溃
	hmiNode.set_allocated_camera(camera);
}

void OutputPtx::setPtxNode(NodeData* childNode, HmiScenegraph::TNode& hmiNode) {
	// 1. name 
    std::string nodeName = childNode->getName();
	int index = nodeName.rfind(".objectID");

	if (-1 != index)
		nodeName = nodeName.substr(0, nodeName.length() - 9);
	hmiNode.set_name(nodeName);

    if (nodeName == "PerspectiveCamera") {
		setPtxTCamera(childNode, hmiNode);
    }

    // 2. 缩放、旋转和位移
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
    // 3. renderorder和childSortOrderRank
    hmiNode.set_renderorder(0);
	hmiNode.set_childsortorderrank(0);

    // 4. mesh
	HmiScenegraph::TMesh mesh;
	setPtxTMesh(childNode, mesh);
	HmiScenegraph::TMesh* it = hmiNode.add_mesh();
	*it = mesh;
}

void OutputPtx::setRootSRT(HmiScenegraph::TNode* hmiNode) {
	// 1. scale
	TVector3f* scale = new TVector3f();
	scale->set_x(1.0);
	scale->set_y(1.0);
	scale->set_z(1.0);
	hmiNode->set_allocated_scale(scale);
	// 2. rotation
	TVector3f* rotation = new TVector3f();
	rotation->set_x(0.0);
	rotation->set_y(0.0);
	rotation->set_z(0.0);
	hmiNode->set_allocated_rotation(rotation);
	// 3. translation
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
	//parent = it;
	//*it = hmiNode;
	*it = hmiNode;
	parent = const_cast<HmiScenegraph::TNode*>(&(parent->child(parent->child_size() - 1)));
	
	for (auto it = pNode->childMapRef().begin(); it != pNode->childMapRef().end(); ++it) {
		writeNodePtx(&(it->second), parent);
	}
}

void setMaterialRenderMode(RenderMode& renderMode, HmiScenegraph::TRenderMode* rRenderMode) {
	int enumMaptemp = 0;
	enumMaptemp = renderMode.getWindingType();
	rRenderMode->set_winding(TEWinding(enumMaptemp));
	enumMaptemp = renderMode.getCulling();
	rRenderMode->set_culling(TEFace(enumMaptemp));

	HmiScenegraph::TBlendMode* tblending = new HmiScenegraph::TBlendMode();
	Blending blending = renderMode.getBlending();
	enumMaptemp = blending.getBlendOperationColor();
	if (enumMaptemp > 4)
		enumMaptemp = 1;
	tblending->set_blendoperationcolor(TEBlendOperation(enumMaptemp));

	enumMaptemp = blending.getBlendOperationAlpha();
	if (enumMaptemp > 4)
		enumMaptemp = 1;
	tblending->set_blendoperationalpha(TEBlendOperation(enumMaptemp));

	enumMaptemp = blending.getSrcAlphaFactor();
	tblending->set_sourcealphafactor(TEBlendFactor(enumMaptemp));

	enumMaptemp = blending.getSrcColorFactor();
	tblending->set_sourcecolorfactor(TEBlendFactor(enumMaptemp));

	enumMaptemp = blending.getDesAlphaFactor();
	tblending->set_destinationalphafactor(TEBlendFactor(enumMaptemp));

	enumMaptemp = blending.getDesColorFactor();
	tblending->set_destinationcolorfactor(TEBlendFactor(enumMaptemp));

	rRenderMode->set_allocated_blending(tblending);

	enumMaptemp = renderMode.getDepthCompare();
	if (enumMaptemp > 8)
		enumMaptemp = 1;
	rRenderMode->set_depthcompare(TECompareFunction(enumMaptemp));

	rRenderMode->set_depthwrite(renderMode.getDepthWrite());

	HmiScenegraph::TRenderMode_TColorWrite* tColorWrite = new HmiScenegraph::TRenderMode_TColorWrite();
	ColorWrite colorWrite = renderMode.getColorWrite();
	tColorWrite->set_alpha(colorWrite.getAlpha());
	tColorWrite->set_blue(colorWrite.getBlue());
	tColorWrite->set_green(colorWrite.getGreen());
	tColorWrite->set_red(colorWrite.getRed());
	rRenderMode->set_allocated_colorwrite(tColorWrite);

}

void uniformTypeValue(Uniform data, HmiScenegraph::TUniform& tUniform) {
	tUniform.set_name(data.getName());
	TNumericValue* tNumericValue = new TNumericValue();

	switch (data.getType()) {
		case UniformType::Bool: {
			uint32_t temp = std::any_cast<bool>(data.getValue());
			tNumericValue->set_uint(temp);
			tUniform.set_allocated_value(tNumericValue);

			tUniform.set_type(TENumericType::TENumericType_unsignedInteger);

			break;
		}
		case UniformType::Double: {
			float temp = std::any_cast<double>(data.getValue());
			tNumericValue->set_float_(temp);
			tUniform.set_allocated_value(tNumericValue);

			tUniform.set_type(TENumericType::TENumericType_float);
			break;
		}
		case UniformType::Int: {
			int temp = std::any_cast<int>(data.getValue());
			tNumericValue->set_int_(temp);
			tUniform.set_allocated_value(tNumericValue);

			tUniform.set_type(TENumericType::TENumericType_unsignedInteger);
			break;
		}
		case UniformType::Null: {
			int temp = std::any_cast<int>(data.getValue());
			tNumericValue->set_uint(temp);
			tUniform.set_allocated_value(tNumericValue);

			tUniform.set_type(TENumericType::TENumericType_unsignedInteger);
			break;
		}
		default:
			break;
	}

}

bool OutputPtx::writeProgram2Ptx(QString filePath) {
	QFile file(filePath + ".ptx");
	if (!file.open(QIODevice::ReadWrite)) {
		return false;
	}
	file.resize(0);

	NodeData* rootNode = &(raco::guiData::NodeDataManager::GetInstance().root());
	HmiScenegraph::TScene scene;
	// name
    HmiScenegraph::TNode* tRoot = new HmiScenegraph::TNode();
	tRoot->set_name(PTX_SCENE_NAME.toStdString());
	// scale rotation translation
	setRootSRT(tRoot);
	// child 
    for (auto& child : rootNode->childMapRef()) {
		NodeData* childNode = &(child.second);
        writeNodePtx(childNode, tRoot);
	}

    scene.set_allocated_root(tRoot);

	// materiallibrary
	HmiScenegraph::TMaterialLib* materialLibrary = new HmiScenegraph::TMaterialLib();
	std::map<std::string, MaterialData> materialMap = raco::guiData::MaterialManager::GetInstance().getMaterialDataMap();
	for (auto& material : materialMap) {
		HmiScenegraph::TMaterial tMaterial;
		MaterialData data = material.second;
		// name 
		tMaterial.set_name(data.getObjectName());

		// RenderMode
		HmiScenegraph::TRenderMode* rRenderMode = new HmiScenegraph::TRenderMode();
		RenderMode renderMode = data.getRenderMode();
		setMaterialRenderMode(renderMode, rRenderMode);
		tMaterial.set_allocated_rendermode(rRenderMode);

		// shaderReference
		tMaterial.set_shaderreference(data.getShaderRef());

		// textures 待补充
		TextureData textureData = data.getTexture();
		if (textureData.getName() != "") {
			HmiScenegraph::TTexture tTextture;
			tTextture.set_name(textureData.getName());
			tTextture.set_bitmapreference(textureData.getBitmapRef());
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

	// shaders
	std::map<std::string, Shader> shaderMap = raco::guiData::MaterialManager::GetInstance().getShaderDataMap();
	for (auto& shader : shaderMap) {
		HmiScenegraph::TShader tShader;
		tShader.set_name(shader.second.getName());
		tShader.set_fragmentshader(shader.second.getFragmentShader());
		tShader.set_vertexshader(shader.second.getVertexShader());
		HmiScenegraph::TShader* tShaderIt = materialLibrary->add_shader();
		*tShaderIt = tShader;
	}

	// bitmaps
	std::map<std::string, Bitmap> bitMaps = raco::guiData::MaterialManager::GetInstance().getBitmapDataMap();
	for (auto& bitData : bitMaps) {
		HmiScenegraph::TBitmap tBitMap;
		tBitMap.set_name(bitData.second.getName());
		tBitMap.set_resource(bitData.second.getResource());
		tBitMap.set_generatemipmaps(bitData.second.getGenerateMipmaps());
		HmiScenegraph::TBitmap* tBitMapIt = materialLibrary->add_bitmap();
		*tBitMapIt = tBitMap;
	}

	scene.set_allocated_materiallibrary(materialLibrary);

    std::string output;
	google::protobuf::TextFormat::PrintToString(scene, &output);

    QByteArray byteArray = QByteArray::fromStdString(output);
	file.write(byteArray);
	file.close();
	return true;
}

}
