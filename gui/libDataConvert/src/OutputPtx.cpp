
#include "data_Convert/OutputPtx.h"
#include "data_Convert/ProgramDefine.h"


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
        for (const auto &texture : data.getTextures()) {
            TextureData textureData = texture;
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
