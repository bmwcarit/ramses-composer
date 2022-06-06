#include "node_logic/NodeLogic.h"
#include "PropertyData/PropertyType.h"
#include <QDebug>
namespace raco::node_logic {
NodeLogic::NodeLogic(raco::core::CommandInterface *commandInterface, QObject *parent)
    : QObject{parent}, commandInterface_{commandInterface} {
    connect(&signalProxy::GetInstance(), &signalProxy::sigUpdateKeyFram_From_AnimationLogic, this, &NodeLogic::slotUpdateKeyFrame);
    connect(&signalProxy::GetInstance(), &signalProxy::sigUpdateActiveAnimation_From_AnimationLogic, this, &NodeLogic::slotUpdateActiveAnimation);
}

void NodeLogic::setCommandInterface(core::CommandInterface *commandInterface) {
    commandInterface_ = commandInterface;
}

	//PropName "objectID"    PropName "objectName"    PropName "children"   PropName "tags"
	//PropName "visible"     PropName "translation"   PropName "rotation"   PropName "scale"
	//PropName "mesh"	     PropName "materials"     PropName "instanceCount"


void NodeLogic::Analyzing(NodeData *pNode) {
	if (!pNode )
		return;
	qDebug() << "Name :" << QString::fromStdString(pNode->getName()) << " ID :" << QString::fromStdString(pNode->objectID()) << "  ";
	
	if (pNode->objectID() != "" && pNode->objectID() != "objectID") {
		auto it = nodeObjectIDHandleReMap_.find(pNode->objectID());
		if (it != nodeObjectIDHandleReMap_.end()) {
			raco::core::ValueHandle valueHandle = nodeObjectIDHandleReMap_.find(pNode->objectID())->second;
			initBasicProperty(valueHandle, pNode);
		}
	}
	for (auto it = pNode->childMapRef().begin(); it != pNode->childMapRef().end(); ++it) {
		Analyzing(&(it->second));
    }
}

bool NodeLogic::getValueHanlde(std::string property, core::ValueHandle &valueHandle) {
    QString qstrPropety = QString::fromStdString(property);
    QStringList list = qstrPropety.split(".");

    auto func = [&](core::ValueHandle &tempHandle, std::string tempProp)->bool {
          if (tempHandle.hasProperty(tempProp)) {
              tempHandle = tempHandle.get(tempProp);
          } else {
              return false;
          }
          return tempHandle.isProperty();
    };

    bool bInvalid = false;
    if (valueHandle.isObject()) {
        for (int i{1}; i < list.size(); i++) {
            QString str = list[i];
            bInvalid = func(valueHandle, str.toStdString());
            if (!bInvalid) {
                i++;
				if (i >= list.size())
                    break;
                str += "." + list[i];
                bInvalid = func(valueHandle, str.toStdString());
                if (!bInvalid)
                    break;
            }
        }
    }
    return bInvalid;
}

void NodeLogic::AnalyzeHandle() {
	raco::guiData::NodeDataManager &nodeManager = NodeDataManager::GetInstance();

	Analyzing(&(nodeManager.root()));
}

void NodeLogic::initBasicProperty(raco::core::ValueHandle valueHandle, NodeData *node) {
	QString strObjId, strPropName;

	for (int i{0}; i < valueHandle.size(); i++) {
		if (!valueHandle[i].isObject()) {	//不是节点就解析 

			raco::core::ValueHandle tempHandle = valueHandle[i];   // 取出里面的所有属性，进行遍历
			QString str = QString::fromStdString(tempHandle.getPropName());
			if (QString::fromStdString(tempHandle.getPropName()).compare("objectID") == 0) {
				strObjId = QString::fromStdString(tempHandle.asString());
				strPropName = QString::fromStdString(tempHandle.getPropertyPath());
				qDebug() << i << " strObjId: " << strObjId << "  objName: " << strPropName;
			}

			if (QString::fromStdString(tempHandle.getPropName()).compare("translation") == 0) {
				Vec3 trans;
				trans.x = tempHandle.get("x").asDouble();
				trans.y = tempHandle.get("y").asDouble();
				trans.z = tempHandle.get("z").asDouble();
				node->insertSystemData("translation", trans);
			} else if (QString::fromStdString(tempHandle.getPropName()).compare("rotation") == 0) {
				Vec3 rota;
				rota.x = tempHandle.get("x").asDouble();
				rota.y = tempHandle.get("y").asDouble();
				rota.z = tempHandle.get("z").asDouble();
				node->insertSystemData("rotation", rota);

			} else if (QString::fromStdString(tempHandle.getPropName()).compare("scale") == 0) {
				Vec3 scal;
				scal.x = tempHandle.get("x").asDouble();
				scal.y = tempHandle.get("y").asDouble();
				scal.z = tempHandle.get("z").asDouble();
				node->insertSystemData("scale", scal);

			}  
            initBasicProperty(valueHandle[i], node);
		}
    }
}

void NodeLogic::setUniformProperty(raco::core::ValueHandle valueHandle, NodeData *node,bool bVec ) {
	using PrimitiveType = core::PrimitiveType;
	for (int i{0}; i < valueHandle.size(); i++) {
		raco::core::ValueHandle tempHandle = valueHandle[i];
		switch (tempHandle.type()) {
			case PrimitiveType::String: {
				break;
			}
			case PrimitiveType::Bool: {
				if (bVec) {
					//PropertyValue *proValue = property->getValue();
					//std::string parentName = property->getValue()->getValueHandle().getPropName();
					std::string curveName = tempHandle.getPropName();
					QString path = QString::fromStdString(tempHandle.getPropertyPath());
					//proValue->addValue(QString::fromStdString(curveName), tempHandle.asBool());
				} else {
					QString path = QString::fromStdString(tempHandle.getPropertyPath());
					//PropertyValue *proValue = new PropertyValue(tempHandle);
					qDebug() << "x = " << tempHandle.asBool();
	/*				proValue->addValue("x", tempHandle.asBool());
					tempProperty = new PropertyTree(proValue, property);
					property->addChild(tempProperty);*/
				}
				break;
			}
			case PrimitiveType::Int: {
				if (bVec) {
					//PropertyValue *proValue = property->getValue();
					//std::string parentName = property->getValue()->getValueHandle().getPropName();
					QString path = QString::fromStdString(tempHandle.getPropertyPath());
					std::string curveName = tempHandle.getPropName();
					qDebug() << " curve name :" << QString::fromStdString(curveName);
					qDebug() << " tempHandle.asInt() :" << tempHandle.asInt();
					//proValue->addValue(QString::fromStdString(curveName), tempHandle.asInt());
				} else {
					//PropertyValue *proValue = new PropertyValue(tempHandle);
					//proValue->addValue("x", tempHandle.asInt());
					QString path = QString::fromStdString(tempHandle.getPropertyPath());
					qDebug() << path << " x = " << tempHandle.asInt();
	/*				tempProperty = new PropertyTree(proValue, property);
					property->addChild(tempProperty);*/
				}
				break;
			}
			case PrimitiveType::Double: {
				if (bVec) {
					//PropertyValue *proValue = property->getValue();
					//std::string parentName = property->getValue()->getValueHandle().getPropName();
					QString path = QString::fromStdString(tempHandle.getPropertyPath());
					std::string curveName = tempHandle.getPropName();
					//proValue->addValue(QString::fromStdString(curveName), tempHandle.asDouble());

					qDebug() << " curve name :" << QString::fromStdString(curveName);
					qDebug() << " tempHandle.asDouble() :" << tempHandle.asDouble();
				} else {
					//PropertyValue *proValue = new PropertyValue(tempHandle);
					//proValue->addValue("x", tempHandle.asDouble());
					//tempProperty = new PropertyTree(proValue, property);
					//property->addChild(tempProperty);
					QString path = QString::fromStdString(tempHandle.getPropertyPath());
					qDebug() << path << " x = " << tempHandle.asDouble();
				}
				break;
			}
			// case libNodeLogic::Vec2f:  // PrimitiveType属性值更新
			// case PrimitiveType::Vec3f:
			// case PrimitiveType::Vec4f:
			// case PrimitiveType::Vec2i:
			// case PrimitiveType::Vec3i:
			// case PrimitiveType::Vec4i:
			case PrimitiveType::Ref:
			case PrimitiveType::Table:
			default: {
				QString path = QString::fromStdString(tempHandle.getPropertyPath());
				break;
			}
		};
		setUniformProperty(valueHandle[i], node, true);
	}
}

void NodeLogic::setMaterial(raco::core::ValueHandle valueHandle, NodeData *node) {
	for (int i{0}; i < valueHandle.size(); i++) {
		if (!valueHandle[i].isObject()) {
			raco::core::ValueHandle tempHandle = valueHandle[i];
			QString strObjId;
			if (QString::fromStdString(tempHandle.getPropName()).compare("objectID") == 0) {
				strObjId = QString::fromStdString(tempHandle.asString());
				qDebug() << " material ObjectId :" << strObjId;
			}
			if (QString::fromStdString(tempHandle.getPropName()).compare("materials") == 0) {
				for (int j{0}; j < tempHandle.size(); j++) {
					if (QString::fromStdString(tempHandle[j].getPropName()).compare("material") == 0) {
						raco::core::ValueHandle materialValue = tempHandle.get("material");
						raco::core::ValueHandle uniformHandle = tempHandle[j].get("uniforms");
						setUniformProperty(uniformHandle, node);
						QString uniformName = QString::fromStdString(uniformHandle.getPropertyPath());
						setMaterial(materialValue, node);
						return;
					}
				}
			}
			if (QString::fromStdString(tempHandle.getPropName()).compare("uniforms") == 0) {
				raco::core::ValueHandle materialValue = tempHandle;
				//raco::core::ValueHandle uniformHandle = tempHandle[j].get("uniforms");
				setUniformProperty(tempHandle, node);
				QString MaterialName = QString::fromStdString(tempHandle.getPropertyPath());
				setMaterial(materialValue, node);
				return;
			}
			setMaterial(tempHandle, node);
		}
	}
}

void NodeLogic::slotUpdateActiveAnimation(QString animation) {
    curAnimation_ = animation;
}

void NodeLogic::slotUpdateKeyFrame(int keyFrame) {
    for (const auto &it : nodeObjectIDHandleReMap_) {
        std::string objectId = it.first;
        std::map<std::string, std::string> bindingDataMap;
		NodeData* nodeData = NodeDataManager::GetInstance().searchNodeByID(objectId);
		if (nodeData) {
			nodeData->NodeExtendRef().curveBindingRef().getPropCurve(curAnimation_.toStdString(), bindingDataMap);
		}
        for (const auto &bindingIt : bindingDataMap) {
            if (CurveManager::GetInstance().getCurve(bindingIt.second)) {
                double value{0};
                if (CurveManager::GetInstance().getCurveValue(bindingIt.second, keyFrame, value)) {
                    setProperty(it.second, bindingIt.first, value);
                }
            }
        }
    }
    //lastKeyFrame_ = keyFrame;
}
}
