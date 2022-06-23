#include "data_Convert/ProgramManager.h"
#include "data_Convert/ProgramDefine.h"

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
QString dataType2String(int type) {
    QString strType;
    switch(type) {
    case PROPERTY_TYPE_FLOAT: {
        strType = "FLOAT";
        break;
    }
    case PROPERTY_TYPE_INT: {
        strType = "INT";
        break;
    }
    case PROPERTY_TYPE_BOOL: {
        strType = "BOOL";
        break;
    }
    case PROPERTY_TYPE_STRING: {
        strType = "STRING";
        break;
    }
    case PROPERTY_TYPE_SAMPLETEXTURE: {
        strType = "SAMPLETEXTURE";
        break;
    }
    case PROPERTY_TYPE_VEC2i: {
        strType = "VEC2i";
        break;
    }
    case PROPERTY_TYPE_VEC3i: {
        strType = "VEC3i";
        break;
    }
    case PROPERTY_TYPE_VEC4i: {
        strType = "VEC24";
        break;
    }
    case PROPERTY_TYPE_VEC2f: {
        strType = "VEC2f";
        break;
    }
    case PROPERTY_TYPE_VEC3f: {
        strType = "VEC3f";
        break;
    }
    case PROPERTY_TYPE_VEC4f: {
        strType = "VEC4f";
        break;
    }
    case PROPERTY_TYPE_MAT2: {
        strType = "MAT2";
        break;
    }
    case PROPERTY_TYPE_MAT3: {
        strType = "MAT3";
        break;
    }
    case PROPERTY_TYPE_MAT4: {
        strType = "MAT4";
        break;
    }
    }
    return strType;
}

QString windingType2String(WindingType type) {
    switch (type) {
		case M_TEWinding_CounterClockWise: {
        return QString("TEWinding_CounterClockWise");
    }
	default: {
		return QString();
    }
    }
}

QString culling2String(Culling culling) {
    switch (culling) {
    case CU_Disabled: {
        return QString("TEFace_None");
    }
    case CU_Front: {
        return QString("TEFace_Front");
    }
    case CU_Back: {
        return QString("TEFace_Back");
    }
    case CU_FrontAndBack: {
        return QString("TEFace_FrontAndBack");
	}
	default: {
		return QString();
	}
    }
}

QString depthCompare2String(DepthCompare depthCompare) {
    switch (depthCompare) {
    case DC_Disabled: {
        return QString("TECompareFunction_Always");
    }
    case DC_GreaterThan: {
        return QString("TECompareFunction_GreaterThan");
    }
    case DC_GreaterOrEqualTo: {
        return QString("TECompareFunction_GreaterOrEqualTo");
    }
    case DC_LessThan: {
        return QString("TECompareFunction_LessThan");
    }
    case DC_LessThanOrEqualTo: {
        return QString("TECompareFunction_LessThanOrEqualTo");
    }
    case DC_Equal: {
        return QString("TECompareFunction_Equal");
    }
    case DC_NotEqual: {
        return QString("TECompareFunction_NotEqual");
    }
    case DC_True: {
        return QString("TECompareFunction_True");
    }
    case DC_False: {
        return QString("TECompareFunction_False");
	}
	default: {
		return QString();
	}
    }
}

QString bool2String(bool temp) {
    if (temp) {
        return QString("true");
    } else {
        return QString("false");
    }
}

QString blendOperation2String(BlendOperation opreation) {
    switch (opreation) {
    case BO_Disabled: {
        return QString("TEBlendOperation_Disabled");
    }
    case BO_Add: {
        return QString("TEBlendOperation_Add");
    }
    case BO_Subtract: {
        return QString("TEBlendOperation_Subtract");
    }
    case BO_ReverseSub: {
        return QString("TEBlendOperation_ReverseSub");
    }
    case BO_Min: {
        return QString("TEBlendOperation_Min");
    }
    case BO_Max: {
        return QString("TEBlendOperation_Max");
	}
	default: {
		return QString();
	}
    }
}

QString blendFactor2String(BlendFactor factor) {
    switch (factor) {
    case Zero: {
        return QString("TEBlendFactor_Zero");
    }
    case One: {
        return QString("TEBlendFactor_One");
    }
    case SrcAlpha: {
        return QString("TEBlendFactor_SourceAlpha");
    }
    case OneMinusSrcAlpha: {
        return QString("TEBlendFactor_InverseSourceAlpha");
    }
    case DstAlpha: {
        return QString("TEBlendFactor_DestinationAlpha");
    }
    case OneMinusSrcColor: {
        return QString("TEBlendFactor_InverseSourceColor");
    }
    case DstColor: {
        return QString("TEBlendFactor_DestinationColor");
    }
    case OneMinusDstColor: {
        return QString("TEBlendFactor_InverseDestinationColor");
	}
	default: {
		return QString();
	}
    }
}

QString filter2String(Filter filter) {
    switch (filter) {
    case Linear: {
        return QString("TETextureFilter_Linear");
    }
    case NearestMipMapNearest: {
        return QString("TETextureFilter_NearestMipMapNearest");
    }
    case NearestMipMapLinear: {
        return QString("TETextureFilter_NearestMipMapLinear");
    }
    case LinearMipMapNearest: {
        return QString("TETextureFilter_LinearMipMapNearest");
    }
    case LinearMipMapLinear: {
        return QString("TETextureFilter_LinearMipMapLinear");
    }
    default: {
        return QString();
    }
    }
}

QString wrapMode2String(WrapMode wMode) {
    switch (wMode) {
    case Clamp: {
        return QString("TETextureWrapMode_ClampToEdge");
    }
    case Repeat: {
        return QString("TETextureWrapMode_RepeatToEdge");
    }
    case Mirror: {
        return QString("TETextureWrapMode_MirrorToEdge");
    }
    default: {
        return QString();
    }
    }
}

void initSystemProerty(QJsonObject& jsonObj, raco::guiData::NodeData& node) {
	QJsonObject translation;
    if(node.hasSystemData("translation")) {
        Vec3 tran = std::any_cast<Vec3>(node.getSystemData("translation"));
        translation.insert(JSON_X, tran.x);
        translation.insert(JSON_Y, tran.y);
        translation.insert(JSON_Z, tran.z);
    }
    jsonObj.insert(JSON_TRANSLATION, translation);

	QJsonObject rotation;
	if (node.hasSystemData("rotation")) {
		Vec3 rota = std::any_cast<Vec3>(node.getSystemData("rotation"));
		rotation.insert(JSON_X, rota.x);
		rotation.insert(JSON_Y, rota.y);
		rotation.insert(JSON_Z, rota.z);
	}
    jsonObj.insert(JSON_ROTATION, rotation);

	QJsonObject scale;
	if (node.hasSystemData("scale")) {
		Vec3 scal = std::any_cast<Vec3>(node.getSystemData("scale"));
		scale.insert(JSON_X, scal.x);
		scale.insert(JSON_Y, scal.y);
		scale.insert(JSON_Z, scal.z);
	}
    jsonObj.insert(JSON_SCALE, scale);
}
void initCustomPropertyProerty(QJsonObject& jsonObj, raco::guiData::NodeData& node) {
	auto customMap = node.NodeExtendRef().customPropRef().customTypeMapRef();
	for (auto customProp : customMap) {
		if (customProp.second.type() == typeid(BoolType)) {
			jsonObj.insert(QString::fromStdString(customProp.first), std::any_cast<BoolType>(customProp.second));
		}
		if (customProp.second.type() == typeid(stringTpye)) {
			jsonObj.insert(QString::fromStdString(customProp.first), QString::fromStdString(std::any_cast<stringTpye>(customProp.second)));
		}
		if (customProp.second.type() == typeid(IntType)) {
			jsonObj.insert(QString::fromStdString(customProp.first), std::any_cast<IntType>(customProp.second));
		}
		if (customProp.second.type() == typeid(DoubleType))
			jsonObj.insert(QString::fromStdString(customProp.first), std::any_cast<DoubleType>(customProp.second));
		if (customProp.second.type() == typeid(FloatType)) {
			jsonObj.insert(QString::fromStdString(customProp.first), std::any_cast<FloatType>(customProp.second));
		}

		if (customProp.second.type() == typeid(Vec2)) {
			QJsonObject vec2JsonObj;
            vec2JsonObj.insert(JSON_X, std::any_cast<Vec2>(customProp.second).x);
            vec2JsonObj.insert(JSON_Y, std::any_cast<Vec2>(customProp.second).y);
			jsonObj.insert(QString::fromStdString(customProp.first), vec2JsonObj);
		}
		if (customProp.second.type() == typeid(Vec3)) {
			QJsonObject vec3JsonObj;
            vec3JsonObj.insert(JSON_X, std::any_cast<Vec3>(customProp.second).x);
            vec3JsonObj.insert(JSON_Y, std::any_cast<Vec3>(customProp.second).y);
            vec3JsonObj.insert(JSON_Z, std::any_cast<Vec3>(customProp.second).z);
			jsonObj.insert(QString::fromStdString(customProp.first), vec3JsonObj);
		}
		if (customProp.second.type() == typeid(Vec4)) {
			QJsonObject vec4JsonObj;
            vec4JsonObj.insert(JSON_X, std::any_cast<Vec4>(customProp.second).x);
            vec4JsonObj.insert(JSON_Y, std::any_cast<Vec4>(customProp.second).y);
            vec4JsonObj.insert(JSON_Z, std::any_cast<Vec4>(customProp.second).z);
            vec4JsonObj.insert(JSON_W, std::any_cast<Vec4>(customProp.second).w);
			jsonObj.insert(QString::fromStdString(customProp.first), vec4JsonObj);
		}
		if (customProp.second.type() == typeid(mat2)) {
			QJsonObject mat2JsonObj;
            mat2JsonObj.insert(JSON_MATRX_OO, std::any_cast<mat2>(customProp.second).m00);
            mat2JsonObj.insert(JSON_MATRX_O1, std::any_cast<mat2>(customProp.second).m01);
            mat2JsonObj.insert(JSON_MATRX_10, std::any_cast<mat2>(customProp.second).m10);
            mat2JsonObj.insert(JSON_MATRX_11, std::any_cast<mat2>(customProp.second).m11);
			jsonObj.insert(QString::fromStdString(customProp.first), mat2JsonObj);
		}
		if (customProp.second.type() == typeid(mat3)) {
			QJsonObject mat3JsonObj;
            mat3JsonObj.insert(JSON_MATRX_OO, std::any_cast<mat3>(customProp.second).m00);
            mat3JsonObj.insert(JSON_MATRX_O1, std::any_cast<mat3>(customProp.second).m01);
            mat3JsonObj.insert(JSON_MATRX_O2, std::any_cast<mat3>(customProp.second).m02);
            mat3JsonObj.insert(JSON_MATRX_10, std::any_cast<mat3>(customProp.second).m10);
            mat3JsonObj.insert(JSON_MATRX_11, std::any_cast<mat3>(customProp.second).m11);
            mat3JsonObj.insert(JSON_MATRX_12, std::any_cast<mat3>(customProp.second).m12);
            mat3JsonObj.insert(JSON_MATRX_20, std::any_cast<mat3>(customProp.second).m20);
            mat3JsonObj.insert(JSON_MATRX_21, std::any_cast<mat3>(customProp.second).m21);
            mat3JsonObj.insert(JSON_MATRX_22, std::any_cast<mat3>(customProp.second).m21);
			jsonObj.insert(QString::fromStdString(customProp.first), mat3JsonObj);
		}
		if (customProp.second.type() == typeid(mat4)) {
			QJsonObject mat4JsonObj;
            mat4JsonObj.insert(JSON_MATRX_OO, std::any_cast<mat4>(customProp.second).m00);
            mat4JsonObj.insert(JSON_MATRX_O1, std::any_cast<mat4>(customProp.second).m01);
            mat4JsonObj.insert(JSON_MATRX_O2, std::any_cast<mat4>(customProp.second).m02);
            mat4JsonObj.insert(JSON_MATRX_O3, std::any_cast<mat4>(customProp.second).m03);
            mat4JsonObj.insert(JSON_MATRX_10, std::any_cast<mat4>(customProp.second).m10);
            mat4JsonObj.insert(JSON_MATRX_11, std::any_cast<mat4>(customProp.second).m11);
            mat4JsonObj.insert(JSON_MATRX_12, std::any_cast<mat4>(customProp.second).m12);
            mat4JsonObj.insert(JSON_MATRX_13, std::any_cast<mat4>(customProp.second).m13);
            mat4JsonObj.insert(JSON_MATRX_20, std::any_cast<mat4>(customProp.second).m20);
            mat4JsonObj.insert(JSON_MATRX_21, std::any_cast<mat4>(customProp.second).m21);
            mat4JsonObj.insert(JSON_MATRX_22, std::any_cast<mat4>(customProp.second).m22);
            mat4JsonObj.insert(JSON_MATRX_23, std::any_cast<mat4>(customProp.second).m23);
            mat4JsonObj.insert(JSON_MATRX_30, std::any_cast<mat4>(customProp.second).m30);
            mat4JsonObj.insert(JSON_MATRX_31, std::any_cast<mat4>(customProp.second).m31);
            mat4JsonObj.insert(JSON_MATRX_32, std::any_cast<mat4>(customProp.second).m32);
            mat4JsonObj.insert(JSON_MATRX_33, std::any_cast<mat4>(customProp.second).m33);
			jsonObj.insert(QString::fromStdString(customProp.first), mat4JsonObj);
		}
		//Todo sampleTexture
		{
		}
	}
}
void initCurveBinding(QJsonObject& jsonObj, raco::guiData::NodeData& node) {
	for (auto cuvebindList : node.NodeExtendRef().curveBindingRef().bindingMap()) {
        QJsonArray curveBindingAry;
		for (auto curveProP : cuvebindList.second) {
			QJsonObject curveBinding;
            curveBinding.insert(JSON_PROPERTY, QString::fromStdString(curveProP.first));
            curveBinding.insert(JSON_CURVE, QString::fromStdString(curveProP.second));
            curveBindingAry.append(curveBinding);
		}
        jsonObj.insert(QString::fromStdString(cuvebindList.first), curveBindingAry);
	}
}
void InitNodeJson(QJsonObject& jsonObj, raco::guiData::NodeData& node) {
    jsonObj.insert(JSON_NAME, QString::fromStdString(node.getName()));
    jsonObj.insert(JSON_OBJECTID, QString::fromStdString(node.objectID()));
	if (node.systemDataMapSize() != 0) {
		QJsonObject sysTemProperty;
		initSystemProerty(sysTemProperty, node);
        jsonObj.insert(JSON_BASIC_PROPERTY, sysTemProperty);
	}

	if (0 != node.getCustomPropertySize()) {
		QJsonObject customProperty;
		initCustomPropertyProerty(customProperty, node);
        jsonObj.insert(JSON_CUSTOM_PROPERTY, customProperty);
	}
	if (0 != node.getBindingySize()) {
		QJsonObject curveBinding;
		initCurveBinding(curveBinding, node);
        jsonObj.insert(JSON_CURVEBINDING, curveBinding);
	}

	if (node.getChildCount() != 0) {
		QJsonArray childs;
		for (auto childNode : node.childMapRef()) {
            QJsonObject child;
			InitNodeJson(child, childNode.second);
			childs.append(child);
		}
        jsonObj.insert(JSON_CHILD, childs);
	} else {
		return;
	}
}
void InitAnimationJson(QJsonObject& jsonObj) {
    auto animationList = raco::guiData::animationDataManager::GetInstance().getAnitnList();
    for (auto animation : animationList) {
		QJsonObject aniData;
        aniData.insert(JSON_START_TIME, animation.second.GetStartTime());
        aniData.insert(JSON_END_TIME, animation.second.GetEndTime());
        aniData.insert(JSON_LOOP_COUNT, animation.second.GetLoopCount());
        aniData.insert(JSON_UPDATE_INTERVAL, animation.second.GetUpdateInterval());
        jsonObj.insert(QString::fromStdString(animation.first), aniData);
	}
    jsonObj.insert(JSON_ACTIVE_ANIMATION, QString::fromStdString(raco::guiData::animationDataManager::GetInstance().GetActiveAnimation()));
}
void InitCurveJson(QJsonArray& jsonObj) {
	for (auto curve : raco::guiData::CurveManager::GetInstance().getCurveList()) {
		QJsonObject curveJson;
        curveJson.insert(JSON_NAME, QString::fromStdString(curve->getCurveName()));
        curveJson.insert(JSON_DATA_TYPE, int(curve->getDataType()));
		QJsonArray pointList;
		for (auto point : curve->getPointList()) {
            QJsonObject pointJson;
            pointJson.insert(JSON_KEYFRAME, point->getKeyFrame());
            pointJson.insert(JSON_INTERPOLATION_TYPE, int(point->getInterPolationType()));
			if (raco::guiData::Type_FLOAT == curve->getDataType()) {
				if (point->getDataValue().type() == typeid(float)) {
                    pointJson.insert(JSON_DATA, std::any_cast<float>(point->getDataValue()));
                    pointJson.insert(JSON_LEFT_TANGENT, std::any_cast<float>(point->getLeftTagent()));
                    pointJson.insert(JSON_RIGHT_TANGENT, std::any_cast<float>(point->getRightTagent()));
				}

				if (point->getDataValue().type() == typeid(double)) {
                    pointJson.insert(JSON_DATA, std::any_cast<double>(point->getDataValue()));
                    pointJson.insert(JSON_LEFT_TANGENT, std::any_cast<double>(point->getLeftTagent()));
                    pointJson.insert(JSON_RIGHT_TANGENT, std::any_cast<double>(point->getRightTagent()));
				}
			}

			if (raco::guiData::Type_VEC2 == curve->getDataType()) {
				QJsonObject dataJson;
                dataJson.insert(JSON_X, std::any_cast<Vec2>(point->getDataValue()).x);
                dataJson.insert(JSON_Y, std::any_cast<Vec2>(point->getDataValue()).y);
                pointJson.insert(JSON_DATA, dataJson);

				QJsonObject leftTagentJson;
                leftTagentJson.insert(JSON_X, std::any_cast<Vec2>(point->getLeftTagent()).x);
                leftTagentJson.insert(JSON_Y, std::any_cast<Vec2>(point->getLeftTagent()).y);
                pointJson.insert(JSON_LEFT_TANGENT, leftTagentJson);

				QJsonObject rightTagentJson;
                rightTagentJson.insert(JSON_X, std::any_cast<Vec2>(point->getRightTagent()).x);
                rightTagentJson.insert(JSON_Y, std::any_cast<Vec2>(point->getRightTagent()).y);
                pointJson.insert(JSON_RIGHT_TANGENT, rightTagentJson);
			}
			if (raco::guiData::Type_VEC3 == curve->getDataType()) {
				QJsonObject dataJson;
                dataJson.insert(JSON_X, std::any_cast<Vec3>(point->getDataValue()).x);
                dataJson.insert(JSON_Y, std::any_cast<Vec3>(point->getDataValue()).y);
                dataJson.insert(JSON_Z, std::any_cast<Vec3>(point->getDataValue()).z);
                pointJson.insert(JSON_DATA, dataJson);

				QJsonObject leftTagentJson;
                leftTagentJson.insert(JSON_X, std::any_cast<Vec3>(point->getLeftTagent()).x);
                leftTagentJson.insert(JSON_Y, std::any_cast<Vec3>(point->getLeftTagent()).y);
                leftTagentJson.insert(JSON_Z, std::any_cast<Vec3>(point->getLeftTagent()).z);
                pointJson.insert(JSON_LEFT_TANGENT, leftTagentJson);

				QJsonObject rightTagentJson;
                rightTagentJson.insert(JSON_X, std::any_cast<Vec3>(point->getRightTagent()).x);
                rightTagentJson.insert(JSON_Y, std::any_cast<Vec3>(point->getRightTagent()).y);
                rightTagentJson.insert(JSON_Z, std::any_cast<Vec3>(point->getRightTagent()).z);
                pointJson.insert(JSON_RIGHT_TANGENT, rightTagentJson);
			}
			if (raco::guiData::Type_VEC4 == curve->getDataType()) {
				QJsonObject dataJson;
                dataJson.insert(JSON_X, std::any_cast<Vec4>(point->getDataValue()).x);
                dataJson.insert(JSON_Y, std::any_cast<Vec4>(point->getDataValue()).y);
                dataJson.insert(JSON_Z, std::any_cast<Vec4>(point->getDataValue()).z);
                dataJson.insert(JSON_W, std::any_cast<Vec4>(point->getDataValue()).w);
                pointJson.insert(JSON_DATA, dataJson);

				QJsonObject leftTagentJson;
                leftTagentJson.insert(JSON_X, std::any_cast<Vec4>(point->getLeftTagent()).x);
                leftTagentJson.insert(JSON_Y, std::any_cast<Vec4>(point->getLeftTagent()).y);
                leftTagentJson.insert(JSON_Z, std::any_cast<Vec4>(point->getLeftTagent()).z);
                leftTagentJson.insert(JSON_W, std::any_cast<Vec4>(point->getDataValue()).w);
                pointJson.insert(JSON_LEFT_TANGENT, leftTagentJson);

				QJsonObject rightTagentJson;
                rightTagentJson.insert(JSON_X, std::any_cast<Vec4>(point->getRightTagent()).x);
                rightTagentJson.insert(JSON_Y, std::any_cast<Vec4>(point->getRightTagent()).y);
                rightTagentJson.insert(JSON_Z, std::any_cast<Vec4>(point->getRightTagent()).z);
                rightTagentJson.insert(JSON_W, std::any_cast<Vec4>(point->getRightTagent()).w);
                pointJson.insert(JSON_RIGHT_TANGENT, rightTagentJson);
			}
			pointList.append(pointJson);
		}
        curveJson.insert(JSON_POINT_LIST, pointList);
		jsonObj.append(curveJson);
	}
}

void initPropertyJson(QJsonObject& jsonObj) {
    std::map<std::string, raco::guiData::EPropertyType> systemPropertyTypeMap = raco::guiData::PropertyDataManager::GetInstance().getSystemPropertyTypeMap();
    QJsonObject systemData;
    for (const auto &systemProperty : systemPropertyTypeMap) {
        systemData.insert(QString::fromStdString(systemProperty.first),  dataType2String(systemProperty.second));
    }
    jsonObj.insert(JSON_SYSTEMPROPERTY, systemData);

    std::map<std::string, raco::guiData::EPropertyType> customPropertyMap = raco::guiData::PropertyDataManager::GetInstance().getCustomPropertyTypeMap();
    QJsonObject customData;
    for (const auto &customProperty : customPropertyMap) {
        customData.insert(QString::fromStdString(customProperty.first), dataType2String(customProperty.second));
    }
    jsonObj.insert(JSON_CUSTOM_PROPERTY, customData);

    std::map<std::string, raco::guiData::EPropertyType> animationPropertyMap = raco::guiData::PropertyDataManager::GetInstance().getAnimationPropertyTypeMap();
    QJsonObject animationData;
    for (const auto &animationProperty : animationPropertyMap) {
        animationData.insert(QString::fromStdString(animationProperty.first),  dataType2String(animationProperty.second));
    }
    jsonObj.insert(JSON_ANIMATIONPROPERTY, animationData);
}

void initUniformJson(MaterialData materialData, QJsonObject& jsonObj) {
    using PrimitiveType = raco::core::PrimitiveType;
    std::vector<Uniform> uniformVec = materialData.getUniforms();
    for (Uniform uniform : uniformVec) {
        QJsonObject uniformObj;
        uniformObj.insert(JSON_NAME, QString::fromStdString(uniform.getName()));

        int type = uniform.getType();
        uniformObj.insert(JSON_TYPE, dataType2String(type));
        QJsonObject valueObj;
        std::any any = uniform.getValue();

        switch(type) {
		case Double: {
            valueObj.insert(JSON_FLOAT, std::any_cast<double>(any));
            break;
        }
        case Int: {
            valueObj.insert(JSON_INT, std::any_cast<int>(any));
            break;
        }
		case Bool: {
            valueObj.insert(JSON_BOOL, bool2String(std::any_cast<bool>(any)));
            break;
        }
        case String: {
            valueObj.insert(JSON_STRING, QString::fromStdString(std::any_cast<std::string>(any)));
            break;
        }
        case Vec2i:
        case Vec3i:
        case Vec4i: {
            std::vector<std::any> vec = std::any_cast<std::vector<std::any>>(any);
            for (std::any data : vec) {
                valueObj.insert(JSON_INT, std::any_cast<int>(data));
            }
            break;
        }
        case Vec2f:
        case Vec3f:
        case Vec4f: {
            std::vector<std::any> vec = std::any_cast<std::vector<std::any>>(any);
            for (std::any data : vec) {
                valueObj.insert(JSON_FLOAT, std::any_cast<double>(data));
            }
            break;
        }
        }
        uniformObj.insert(JSON_VALUE, valueObj);
        jsonObj.insert(JSON_UNIFORM, uniformObj);
    }
}

void initMaterialJson(QJsonObject& jsonObj) {
    std::map<std::string, MaterialData> materialDataMap = MaterialManager::GetInstance().getMaterialDataMap();
	QJsonArray materialAry;
    for (const auto &material : materialDataMap) {
        QJsonObject MaterialObj;
        MaterialData materialData = material.second;
        MaterialObj.insert(JSON_NAME, QString::fromStdString(materialData.getObjectName()));

        QJsonObject renderModeObj;
        renderModeObj.insert(JSON_WINDING, windingType2String(materialData.getRenderMode().getWindingType()));
        renderModeObj.insert(JSON_CULLING, culling2String(materialData.getRenderMode().getCulling()));
        renderModeObj.insert(JSON_DEPTH_COMPARE, depthCompare2String(materialData.getRenderMode().getDepthCompare()));
        renderModeObj.insert(JSON_DEPTH_WRITE, bool2String(materialData.getRenderMode().getDepthWrite()));

        QJsonObject blendingObj;
        blendingObj.insert(JSON_BLEND_OPT_COLOR, blendOperation2String(materialData.getRenderMode().getBlending().getBlendOperationColor()));
        blendingObj.insert(JSON_BLEND_OPT_ALPHA, blendOperation2String(materialData.getRenderMode().getBlending().getBlendOperationAlpha()));
        blendingObj.insert(JSON_SRC_COLOR_FACTOR, blendFactor2String(materialData.getRenderMode().getBlending().getSrcColorFactor()));
        blendingObj.insert(JSON_SRC_ALPHA_FACTOR, blendFactor2String(materialData.getRenderMode().getBlending().getSrcAlphaFactor()));
        blendingObj.insert(JSON_DEST_COLOR_FACTOR, blendFactor2String(materialData.getRenderMode().getBlending().getDesColorFactor()));
        blendingObj.insert(JSON_DEST_ALPHA_FACTOR, blendFactor2String(materialData.getRenderMode().getBlending().getDesAlphaFactor()));
        renderModeObj.insert(JSON_BLENDING, blendingObj);

        QJsonObject colorWriteObj;
        colorWriteObj.insert(JSON_RED, materialData.getRenderMode().getColorWrite().getRed());
        colorWriteObj.insert(JSON_GREEN, materialData.getRenderMode().getColorWrite().getGreen());
        colorWriteObj.insert(JSON_BLUE, materialData.getRenderMode().getColorWrite().getBlue());
        colorWriteObj.insert(JSON_ALPHA, materialData.getRenderMode().getColorWrite().getAlpha());
        renderModeObj.insert(JSON_COLOR_WRITE, colorWriteObj);
        MaterialObj.insert(JSON_RENDER_MODE, renderModeObj);

        MaterialObj.insert(JSON_SHADER_REFERENCE, QString::fromStdString(materialData.getShaderRef()));

        QJsonObject textureObj;
        textureObj.insert(JSON_NAME, QString::fromStdString(materialData.getTexture().getName()));
        textureObj.insert(JSON_BITMAP_REFERENCE, QString::fromStdString(materialData.getTexture().getBitmapRef()));
        textureObj.insert(JSON_MIN_FILTER, filter2String(materialData.getTexture().getMinFilter()));
        textureObj.insert(JSON_MAG_FILTER, filter2String(materialData.getTexture().getMagFilter()));
        textureObj.insert(JSON_ANISOTROPIC_SAMPLES, materialData.getTexture().getAnisotropicSamples());
        textureObj.insert(JSON_WRAPE_MODEU, wrapMode2String(materialData.getTexture().getWrapModeU()));
        textureObj.insert(JSON_WRAPE_MODEV, wrapMode2String(materialData.getTexture().getWrapModeV()));
        textureObj.insert(JSON_UNIFORM_NAME, QString::fromStdString(materialData.getTexture().getUniformName()));
        MaterialObj.insert(JSON_TEXTURE, textureObj);

        initUniformJson(materialData, MaterialObj);
		materialAry.append(MaterialObj);
    }
	jsonObj.insert(JSON_MATERIAL, materialAry);
}

void initShaderJson(QJsonObject& jsonObj) {
    std::map<std::string, Shader> shaderDataMap = MaterialManager::GetInstance().getShaderDataMap();
    QJsonArray shaderAry;
    for (auto it : shaderDataMap) {
        QJsonObject shaderObject;
        shaderObject.insert(JSON_NAME, QString::fromStdString(it.second.getName()));
        shaderObject.insert(JSON_VERTEX_SHADER, QString::fromStdString(it.second.getVertexShader()));
        shaderObject.insert(JSON_FRAGMENT_SHADER, QString::fromStdString(it.second.getFragmentShader()));
        shaderAry.append(shaderObject);
    }
    jsonObj.insert(JSON_SHADER, shaderAry);
}

void initBitmapJson(QJsonObject& jsonObj) {
    std::map<std::string, Bitmap> bitmapDataMap = MaterialManager::GetInstance().getBitmapDataMap();
    QJsonArray bitmapAry;
    for (auto it : bitmapDataMap) {
        QJsonObject bitMapObject;
        bitMapObject.insert(JSON_NAME, QString::fromStdString(it.second.getName()));
        bitMapObject.insert(JSON_RESOURCE, QString::fromStdString(it.second.getResource()));
        bitMapObject.insert(JSON_GENERATE_MIPMAPS, it.second.getGenerateMipmaps());
        bitmapAry.append(bitMapObject);
    }
    jsonObj.insert(JSON_BITMAP, bitmapAry);
}

void readJsonFillSystemProperty(QJsonObject jsonObj, NodeData &node) {
    QJsonObject translation = jsonObj.value(JSON_TRANSLATION).toObject();
    Vec3 vec3;
    if (translation.contains(JSON_X)) {
        vec3.x = translation.value(JSON_X).toDouble();
    }
    if (translation.contains(JSON_Y)) {
        vec3.y = translation.value(JSON_Y).toDouble();
    }
    if (translation.contains(JSON_Z)) {
        vec3.z = translation.value(JSON_Z).toDouble();
    }
    node.insertSystemData("translation", vec3);

    QJsonObject rotation = jsonObj.value(JSON_ROTATION).toObject();
    if (rotation.contains(JSON_X)) {
        vec3.x = rotation.value(JSON_X).toDouble();
    }
    if (rotation.contains(JSON_Y)) {
        vec3.y = rotation.value(JSON_Y).toDouble();
    }
    if (rotation.contains(JSON_Z)) {
        vec3.z = rotation.value(JSON_Z).toDouble();
    }
    node.insertSystemData("rotation", vec3);

    QJsonObject scale = jsonObj.value(JSON_SCALE).toObject();
    if (scale.contains(JSON_X)) {
        vec3.x = scale.value(JSON_X).toDouble();
    }
    if (scale.contains(JSON_Y)) {
        vec3.y = scale.value(JSON_Y).toDouble();
    }
    if (scale.contains(JSON_Z)) {
        vec3.z = scale.value(JSON_Z).toDouble();
    }
    node.insertSystemData("scale", vec3);
}

void readJsonFillCustomProperty(QJsonObject jsonObj, NodeData &node) {
    QMap<QString, QVariant> varMap = jsonObj.toVariantMap();
    for (auto it : varMap.toStdMap()) {
        // check customProperty is valid
        if (PropertyDataManager::GetInstance().hasCustomProperty(it.first.toStdString())) {
            std::any any = std::any_cast<double>(it.second.toDouble());
            node.NodeExtendRef().customPropRef().insertType(it.first.toStdString(), any);
        }
    }
}

void readJsonFillCurveBinding(QJsonObject jsonObj, NodeData &node) {
    QMap<QString, QVariant> varMap = jsonObj.toVariantMap();
    QString key = varMap.firstKey();
    QJsonArray curveBindingAry = varMap.first().toJsonArray();
    for (int i{0}; i < curveBindingAry.size(); ++i) {
        QJsonObject curveBindingObj = curveBindingAry[i].toObject();
        std::string curve = curveBindingObj.value(JSON_CURVE).toString().toStdString();
        std::string property = curveBindingObj.value(JSON_PROPERTY).toString().toStdString();
        node.NodeExtendRef().curveBindingRef().insertBindingDataItem(key.toStdString(), property, curve);
    }
}

void readJsonFillAnimationData(QJsonObject jsonObj) {
    for (auto it : jsonObj.toVariantMap().toStdMap()) {
		if (it.first.compare(JSON_ACTIVE_ANIMATION) != 0) {
			QJsonObject aniNodeObj = it.second.toJsonObject();
			animationDataManager::GetInstance().InsertAmimation(it.first.toStdString());
			animationDataManager::GetInstance().getAnimationData(it.first.toStdString()).SetStartTime(aniNodeObj.value(JSON_START_TIME).toInt());
			animationDataManager::GetInstance().getAnimationData(it.first.toStdString()).SetEndTime(aniNodeObj.value(JSON_END_TIME).toInt());
			animationDataManager::GetInstance().getAnimationData(it.first.toStdString()).SetLoopCount(aniNodeObj.value(JSON_LOOP_COUNT).toInt());
			animationDataManager::GetInstance().getAnimationData(it.first.toStdString()).SetUpdateInterval(aniNodeObj.value(JSON_UPDATE_INTERVAL).toInt());
        }
    }
}

void readJsonFilleNodeData(QJsonObject jsonObj, NodeData &node) {
    node.setName(jsonObj.value(JSON_NAME).toString().toStdString());
    node.setObjectID(jsonObj.value(JSON_OBJECTID).toString().toStdString());

    if (jsonObj.contains(JSON_BASIC_PROPERTY)) {
        QJsonObject systemObj = jsonObj.value(JSON_BASIC_PROPERTY).toObject();
        readJsonFillSystemProperty(systemObj, node);
    }
    if (jsonObj.contains(JSON_CUSTOM_PROPERTY)) {
        QJsonObject customObj = jsonObj.value(JSON_CUSTOM_PROPERTY).toObject();
        readJsonFillCustomProperty(customObj, node);
    }
    if (jsonObj.contains(JSON_CURVEBINDING)) {
        QJsonObject curveBindingObj = jsonObj.value(JSON_CURVEBINDING).toObject();
        readJsonFillCurveBinding(curveBindingObj, node);
    }

    if (jsonObj.contains(JSON_CHILD)) {
        QJsonArray childAry = jsonObj.value(JSON_CHILD).toArray();
        std::map<std::string, NodeData> childNodeMap;
        for (int i{0}; i < childAry.size(); ++i) {
            NodeData childNode;
            readJsonFilleNodeData(childAry[i].toObject(), childNode);
            childNodeMap.emplace(childNode.getName(), childNode);
        }
		node.setChildList(childNodeMap);
    }
}

void readJsonFillCurveData(QJsonArray jsonAry) {
    for (int i{0}; i < jsonAry.size(); ++i) {
        QJsonObject curveObj = jsonAry.at(i).toObject();
        Curve* curve = new Curve();
        curve->setCurveName(curveObj.value(JSON_NAME).toString().toStdString());
        curve->setDataType(static_cast<EDataType>(curveObj.value(JSON_DATA_TYPE).toInt()));
        QJsonArray pointAry = curveObj.value(JSON_POINT_LIST).toArray();
        for (int j{0}; j < pointAry.size(); ++j) {
            QJsonObject pointObj = pointAry.at(j).toObject();
            Point* point = new Point;
            point->setDataValue(pointObj.value(JSON_DATA).toDouble());
            point->setInterPolationType(static_cast<EInterPolationType>(pointObj.value(JSON_INTERPOLATION_TYPE).toInt()));
			point->setLeftTagent(pointObj.value(JSON_LEFT_TANGENT).toDouble());
            point->setRightTagent(pointObj.value(JSON_RIGHT_TANGENT).toDouble());
            point->setKeyFrame(pointObj.value(JSON_KEYFRAME).toInt());
            curve->insertPoint(point);
        }
        CurveManager::GetInstance().addCurve(curve);
    }
}

void readJsonFillPropertyData(QJsonObject jsonObj) {
    QJsonObject animationProObj = jsonObj.value(JSON_ANIMATIONPROPERTY).toObject();
    QMap<QString, QVariant> animationMap = animationProObj.toVariantMap();
    for (const auto &it : animationMap.toStdMap()) {
        PropertyDataManager::GetInstance().insertAnimationProItem(it.first.toStdString(), static_cast<EPropertyType>(it.second.toInt()));
    }

    QJsonObject customObj = jsonObj.value(JSON_CUSTOM_PROPERTY).toObject();
    QMap<QString, QVariant> customMap = customObj.toVariantMap();
    for (const auto &it : customMap.toStdMap()) {
        PropertyDataManager::GetInstance().insertCustomProItem(it.first.toStdString(), static_cast<EPropertyType>(it.second.toInt()));
    }

    QJsonObject systemObj = jsonObj.value(JSON_SYSTEMPROPERTY).toObject();
    QMap<QString, QVariant> systemMap = systemObj.toVariantMap();
    for (const auto &it : systemMap.toStdMap()) {
        PropertyDataManager::GetInstance().insertSystemProItem(it.first.toStdString(), static_cast<EPropertyType>(it.second.toInt()));
    }
}

void writeAsset(std::string filePath) {
	filePath = filePath.substr(0, filePath.find(".rca"));
	HmiWidget::TWidgetCollection widgetCollection;
	HmiWidget::TWidget* widget = widgetCollection.add_widget();

	TIdentifier *type = new TIdentifier;
	type->set_valuestring("eWidgetType_Generate");
	widget->set_allocated_type(type);

	TIdentifier *prototype = new TIdentifier;
	prototype->set_valuestring("eWidgetType_Model");
	widget->set_allocated_prototype(prototype);

	HmiWidget::TExternalModelParameter* externalModelValue = widget->add_externalmodelvalue();
	TIdentifier *key = new TIdentifier;
	key->set_valuestring("WidgetNameHint");
	externalModelValue->set_allocated_key(key);
	TVariant *variant = new TVariant;
	variant->set_asciistring("WIDGET_SCENE");
	externalModelValue->set_allocated_variant(variant);

	externalModelValue = widget->add_externalmodelvalue();
	TIdentifier *key1 = new TIdentifier;
	key1->set_valuestring("eParam_ModelResourceId");
	externalModelValue->set_allocated_key(key1);
	TVariant *variant1 = new TVariant;
	variant1->set_resourceid("scene.ptx");
	externalModelValue->set_allocated_variant(variant1);

	std::string output;
	google::protobuf::TextFormat::PrintToString(widgetCollection, &output);
	std::cout << output << std::endl;

    QDir* folder = new QDir;
    if (!folder->exists(QString::fromStdString(filePath))) {
		bool ok = folder->mkpath(QString::fromStdString(filePath));
	}
	delete folder;
	std::ofstream outfile;
	outfile.open(filePath +"/widget.ptw", std::ios_base::out | std::ios_base::trunc);
	outfile << output << std::endl;
	outfile.close();
}
namespace raco::dataConvert {

void ProgramManager::setRelativePath(QString path) {
    relativePath_ = path;
}

bool ProgramManager::writeCTMFile() {
    QDir folder(relativePath_ + "/ctms");
    if (!folder.exists()) {
        folder.mkpath(relativePath_ + "/ctms");
    } else {
        for (int i{0}; i < folder.count(); i++) {
            folder.remove(folder[i]);
        }
    }

    for (const auto &meshIt : MeshDataManager::GetInstance().getMeshDataMap()) {
        MeshData mesh = meshIt.second;
        std::string path = relativePath_.toStdString() + mesh.getMeshName();
        CTMuint aVerCount = mesh.getNumVertices();
        CTMuint aTriCount = mesh.getNumTriangles();

        CTMfloat *aVertices = new CTMfloat[aVerCount];
        CTMuint *aIndices = new CTMuint[aTriCount];

        CTMcontext context;
        CTMenum ret;

        CTMfloat *ptrVertices = aVertices;
        CTMuint *ptrIndices = aIndices;

        for (int j = 0; j < aVerCount; j++) {
//            *ptrVertices = j*0.1f;
//            *(ptrVertices + 1) = j*0.2f;
//            *(ptrVertices + 2) = j*0.3f;
//            ptrVertices += 3;
        }

        for (int i = 0; i < aTriCount; i++) {
//            *(ptrIndices + 0) = i;
//            *(ptrIndices + 1) = i + 1;
//            *(ptrIndices + 2) = i + 2;
//            ptrIndices += 3;
        }

        context = ctmNewContext(CTM_EXPORT);

        // fill mesh
        ctmDefineMesh(context, aVertices, aVerCount, aIndices, aTriCount, NULL);

        // fill attributes
        for (const auto &attriIt : mesh.getAttributes()) {
            ctmAddAttribMap(context, attriIt.data.data(), attriIt.name.c_str());
        }

        ctmCompressionMethod(context, CTM_METHOD_MG1);

        ctmSave(context, path.c_str());
        ctmFreeContext(context);
    }
    return true;
}

bool ProgramManager::writeProgram2Json(QString filePath) {
//    writeCTMFile();
	writeAsset(filePath.toStdString());
	QFile file(filePath + ".json");
	if (!file.open(QIODevice::ReadWrite)) {
		return false;
	}
	file.resize(0);
	QJsonObject jsonObj;

    QJsonObject NodeObj;
    InitNodeJson(NodeObj, raco::guiData::NodeDataManager::GetInstance().root());
    NodeObj.insert(JSON_ACTIVE_NODE, QString::fromStdString(raco::guiData::NodeDataManager::GetInstance().getActiveNode()->objectID()));
    jsonObj.insert(JSON_NODE, NodeObj);

    QJsonObject AnimationObj;
    InitAnimationJson(AnimationObj);
    jsonObj.insert(JSON_ANIMATION, AnimationObj);

    QJsonArray CurveObj;
    InitCurveJson(CurveObj);
    jsonObj.insert(JSON_CURVE, CurveObj);

    QJsonObject PropertyObj;
    initPropertyJson(PropertyObj);
    jsonObj.insert(JSON_PROPERTY, PropertyObj);

    initMaterialJson(jsonObj);
    initShaderJson(jsonObj);
    initBitmapJson(jsonObj);

	QJsonDocument document;
	document.setObject(jsonObj);
	QByteArray byteArray = document.toJson();
	file.write(byteArray);
	file.close();
	return true;
}
bool ProgramManager::readProgramFromJson(QString filePath) {
    QFile file(filePath + ".json");
    if (!file.open(QIODevice::ReadWrite)) {
        return false;
    }
    QJsonParseError jsonParserError;
    QJsonDocument jsonDocument = QJsonDocument::fromJson(file.readAll(), &jsonParserError);
    QJsonObject jsonObject;

    if (!(!jsonDocument.isNull() && jsonParserError.error == QJsonParseError::NoError)) {
        return false;
    }
    if (!jsonDocument.isObject()) {
        return false;
    }
    jsonObject = jsonDocument.object();

    // property
    QJsonObject propertyObj = jsonObject.value(JSON_PROPERTY).toObject();
    readJsonFillPropertyData(propertyObj);
    signal::signalProxy::GetInstance().sigInitPropertyView();

    // animation
    QJsonObject animationObj = jsonObject.value(JSON_ANIMATION).toObject();
    readJsonFillAnimationData(animationObj);

    // avtive animation
	if (animationObj.contains(JSON_ACTIVE_ANIMATION)) {
        std::string activeAnimation = animationObj.value(JSON_ACTIVE_ANIMATION).toString().toStdString();
        if (animationDataManager::GetInstance().IsHaveAnimation(activeAnimation)) {
            animationDataManager::GetInstance().SetActiveAnimation(activeAnimation);
        }
    }
    signal::signalProxy::GetInstance().sigInitAnimationView();

    // curve
    QJsonArray curveAry = jsonObject.value(JSON_CURVE).toArray();
    readJsonFillCurveData(curveAry);
    signal::signalProxy::GetInstance().sigInitCurveView();

    // node
	NodeDataManager::GetInstance().clearNodeData();
    QJsonObject nodeObj = jsonObject.value(JSON_NODE).toObject();
	readJsonFilleNodeData(nodeObj, NodeDataManager::GetInstance().root());

    // active node
    NodeData* nodeData = NodeDataManager::GetInstance().searchNodeByID(nodeObj.value(JSON_ACTIVE_NODE).toString().toStdString());
    if (nodeData) {
        NodeDataManager::GetInstance().setActiveNode(nodeData);
        Q_EMIT selectObject(QString::fromStdString(nodeData->objectID()));
    }
    signal::signalProxy::GetInstance().sigInitPropertyBrowserView();

    return true;
}
}
