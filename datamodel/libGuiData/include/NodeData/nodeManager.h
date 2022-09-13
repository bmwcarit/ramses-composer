#ifndef NODEMANAGER_H
#define NODEMANAGER_H

#include "NodeData/nodeDataEx.h"
#include "core/CommandInterface.h"
#include "PropertyData/PropertyType.h"
#include <iostream>
#include <set>
#include <vector>
#include <list>
#include <map>
#include <functional>

namespace raco::guiData {

class NodeData {
public:
	NodeData() : nodeName_{""}, objectID_{""}, materialsID_{""}, meshID_{""}, parentNode_{nullptr} {}

	NodeExtend& NodeExtendRef() {
		return nodeExtend_;
	}
	void setNodeExtend(NodeExtend& nodeEx) {
		nodeExtend_ = std::move(nodeEx);
	}

	std::string& objectID() {
		return objectID_;
	}
	void setObjectID(std::string id) {
		if (!id.compare("objectID"))
			return;
		objectID_ = id;
	}

	std::string delNodeNameSuffix(std::string nodeName) {
		int index = nodeName.rfind(".objectID");
		if (-1 != index)
			nodeName = nodeName.substr(0, nodeName.length() - 9);
		return nodeName;
    }

	std::string getName() {
		return nodeName_;
	}

    void setName(std::string name) {
		nodeName_ = delNodeNameSuffix(name);
	}

	std::string getMaterialName() {
		return materialName_;
	}

	void setMaterialName(std::string name) {
        materialName_ = name;
	}

	std::string getMaterialsID() {
		return materialsID_;
	}
	void setMaterialsID(std::string materialsID) {
		materialsID_ = materialsID;
	}

	std::string getMeshID() {
		return meshID_;
	}
	void setMeshID(std::string meshId) {
		meshID_ = meshId;
	}
	
	std::map<std::string, std::any>& systemDataMapNewRef() {
		return systemDataMap_;
	}

	size_t systemDataMapSize() {
		return systemDataMap_.size();
	}

	std::any getSystemData(std::string name) {
		return systemDataMap_[name];
	}

	bool insertSystemData(std::string name,std::any value) {
		auto ret = systemDataMap_.emplace(name, std::any_cast<Vec3>(value));
		if (ret.second == false) {
			std::cout << "element [" << name << "] already existed " << std::endl;
			return false;
		}
		return true;
	}

	bool deleteSystemData(std::string name) {
		auto iter = systemDataMap_.find(name);
		if (iter == systemDataMap_.end()) {
			std::cout << "can't find [" << name << "] !\n";
			return false;
		}
		systemDataMap_.erase(iter);
		return true;
	}

	bool modifySystemData(std::string name, std::any value) {
		auto iter = systemDataMap_.find(name);
		if (iter == systemDataMap_.end()) {
			std::cout << "can't find [" << name << "] !\n";
			return false;
		}
		iter->second = std::any_cast<Vec3>(value);
		return true;
	}

	bool hasSystemData(std::string name) {
		auto iter = systemDataMap_.find(name);
		if (iter == systemDataMap_.end()) {
			return false;
		}
		return true;
	}


	NodeData* getParent() {
		return parentNode_;
	}
	void setParent(NodeData* parent) {
		parentNode_ = parent;
	}

	std::map<std::string, NodeData>& childMapRef() {
		return childNodeMap_;
	}

	bool addChild(std::string name, NodeData data) {
		auto it = childNodeMap_.find(name);
		if (it != childNodeMap_.end()) {
			return false;
		}
		childNodeMap_.emplace(name, data);
		return true;
	}

    bool removeChild(std::string name) {
        auto it = childNodeMap_.find(name);
        if (it == childNodeMap_.end()) {
            return false;
        }
        childNodeMap_.erase(it);
        return true;
    }

	void setChildList(std::map<std::string, NodeData>& childMap) {
		childNodeMap_ = childMap;
	}

	void traverseNode() {
		std::cout << "objectID_ :" << objectID_ << std::endl;
		std::cout << "nodeExtend_ :" << std::endl;
		nodeExtend_.traverseNodeExtend();
		//std::cout << "handle_ :" << handle_ << std::endl;
		std::cout << "parentNode_ :" << parentNode_ << std::endl;
		for (auto& it : childNodeMap_) {
			std::cout << it.first << "   ";
		}
		std::cout << std::endl;
	}

	int getCustomPropertySize() {
		return nodeExtend_.getCustomPropSize();
	}

	int getChildCount() {
		return childNodeMap_.size();
	}
	int getBindingySize() {
		return nodeExtend_.getCurveBindingSize();
	}

	bool hasUniform(std::string name) {
		for (auto& it : uniforms_) {
			if (it.getName() == name) {
				return true;
			}
		}
		return false;
	}

	bool insertUniformData(Uniform uniform) {
		if (hasUniform(uniform.getName())) {
			return false;
		}
		uniforms_.push_back(uniform);
		return true;
	}

	bool deleteUniformData(std::string name) {
		for (auto it = uniforms_.begin(); it < uniforms_.end(); ++it) {
			if (it->getName() == name) {
				uniforms_.erase(it);
				return true;
			}
		}

		return false;
	}

	bool modifyUniformData(std::string name, std::any data) {
		for (auto it = uniforms_.begin(); it < uniforms_.end(); ++it) {
			if (it->getName() == name) {
				it->setValue(data);
				return true;
			}
		}
		return false;
	}

	bool modifyUniformType(std::string name, UniformType uniformType) {
		for (auto it = uniforms_.begin(); it < uniforms_.end(); ++it) {
			if (it->getName() == name) {
				it->setType(uniformType);
				return true;
			}
		}
		return false;
	}

	std::vector<Uniform> getUniforms() {
		return uniforms_;
	}

	int getUniformsSize() {
		return uniforms_.size();
	}

	void uniformClear() {
		uniforms_.clear();
	}

	void setMaterialIsChanged(bool changed) {
		materialChanged_ = changed;
	}

	bool materialIsChanged() {
		return materialChanged_;

	}

private:
	std::string nodeName_;
	std::string objectID_;
	NodeExtend nodeExtend_;
	std::map<std::string, std::any> systemDataMap_;
	// materials ID
	std::string materialsID_;
	// mesh ID
	std::string meshID_;
	// uniforms
	std::string materialName_;
	bool materialChanged_{false};
	std::vector<Uniform> uniforms_;

	NodeData* parentNode_;
	std::map<std::string, NodeData> childNodeMap_;
};

class NodeDataManager
{
private:
	NodeDataManager() 
        : activeNode_{nullptr}, searchedNode_{nullptr} {
		activeNode_ = &root_;
		std::string name = "root";
		root_.setName(name);
	}
public:
	static NodeDataManager& GetInstance();
	~NodeDataManager() {}
	NodeDataManager(const NodeDataManager&) = delete;
	NodeDataManager& operator=(const NodeDataManager&) = delete;

public:
    NodeData* getActiveNode() {
        return activeNode_;
    }

    void setActiveNode(NodeData* pNode) {
		if (!pNode) {
			std::cout << "[ERROR]：setActiveNode null！" << std::endl;
			return;
		}
        activeNode_ = pNode;
    }

    NodeData& root() {
        return root_;
    }
    void setRoot(NodeData root) {
        root_ = root;
    }

    void insertNode(NodeData& node);
    void deleting(NodeData& pNode);
    bool deleteNode(NodeData& pNode);
    bool deleteActiveNode();
    bool clearNodeData();

    NodeData* searchNodeByID(const std::string& objectID);
    void searchingByID(NodeData* pNode, const std::string& objectID);

    NodeData* searchNodeByName(std::string& nodeName);
	void searchingByName(NodeData* pNode, std::string& nodeName);

    void preOrderReverse(std::function<void(NodeData*)> fun = nullptr);
	void preOrderReverse(NodeData* pNode, std::function<void(NodeData*)> fun = nullptr);

	void delCurveBindingByName(std::string curveName);
	void delOneCurveBindingByName(NodeData* pNode, std::string curveName);

private:
    NodeData* activeNode_;
    NodeData root_;
    NodeData* searchedNode_;
};
}

#endif // NODEMANAGER_H
