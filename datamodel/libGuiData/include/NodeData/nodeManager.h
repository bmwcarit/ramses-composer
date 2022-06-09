#ifndef NODE_H
#define NODE_H

#include "NodeData/nodeDataEx.h"
#include "core/CommandInterface.h"
#include "PropertyData/PropertyType.h"
#include <iostream>
#include <set>
#include <vector>
#include <map>
#include <functional>

namespace raco::guiData {

class NodeData {
public:
	NodeData() : nodeName_{""}, objectID_{""}, parentNode_{nullptr} {}

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

	std::string getName() {
		return nodeName_;
	}
	void setName(std::string name) {
		nodeName_ = name;
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
		//return 0;
	}

	int getChildCount() {
		return childNodeMap_.size();
	}
	int getBindingySize() {
		return nodeExtend_.getCurveBindingSize();
		//return 0;
	}

private:
	std::string nodeName_;	// 也可以路径
	// 当前结点在ValueHandle中的ID，可以用来索引Handle
	std::string objectID_;
	// 扩展的属性
	NodeExtend nodeExtend_;
	// 添加系统属性
	std::map<std::string, std::any> systemDataMap_;
	// 从渲染引擎中获取数据句柄指针（最好是指针或者引用）
	//void* handle_;					 // 渲染引擎中的数据，建议保存，因为handleProp_是node的一部分
	// 去掉handle，在逻辑层加一个映射，map<std::string,handleType>  nodeNameHandleMap
	NodeData* parentNode_;							// 指向父节点
	std::map<std::string, NodeData> childNodeMap_;	// 子结点列表
};

class NodeDataManager  // 处理node相关的逻辑
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
		if (!pNode) {  // 点击matrial 也会切换结点，但是这个结点不在nodetree中，导致切换active结点为NULL
			std::cout << "[ERROR]：设置活动结点为空！" << std::endl;
			return;
		}
        activeNode_ = pNode;
    }

    NodeData& root() {
        return root_;
    }
  //  void setRoot(NodeData& root) {
		//root_ = root;
  //  }

    void insertNode(NodeData& node);    // 当前活跃的结点下，插入子结点
    void deleting(NodeData& pNode);     // 递归删除结点函数
    bool deleteNode(NodeData& pNode);   // 删除当前活跃的结点
    bool deleteActiveNode();
    bool clearNodeData();

    NodeData* searchNodeByID(const std::string& objectID);  // 从根结点查找
    void searchingByID(NodeData* pNode, const std::string& objectID);

    NodeData* searchNodeByName(std::string& nodeName);  // 从根结点查找
	void searchingByName(NodeData* pNode, std::string& nodeName);

    void preOrderReverse(std::function<void(NodeData*)> fun = nullptr);					  // 先序遍历
	void preOrderReverse(NodeData* pNode, std::function<void(NodeData*)> fun = nullptr);	// 先序遍历

private:
     // 存放所有的node，objectID_是node的惟一索引
    NodeData* activeNode_; // 一直指向当前活跃的结点
    NodeData root_;
    NodeData* searchedNode_;
};


}

#endif // NODE_H
