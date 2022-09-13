#include "NodeData/nodeManager.h"


namespace raco::guiData {
NodeDataManager& NodeDataManager::GetInstance() {
	static NodeDataManager Instance;
	return Instance;
}

void NodeDataManager::insertNode(NodeData& pNode) {
    std::cout << "activeNode_ = " << activeNode_ << std::endl;
    pNode.setParent(activeNode_);
    activeNode_->childMapRef().emplace(pNode.getName(),pNode);
	activeNode_ =&(activeNode_->childMapRef().find(pNode.getName())->second);
}

void NodeDataManager::deleting(NodeData& pNode) {
    if(!pNode.childMapRef().empty()) {
		std::map<std::string,NodeData> tempList = pNode.childMapRef();
        for(auto& it: tempList){
            deleting(it.second);
        }
		pNode.childMapRef().clear();
        deleting(pNode);
    }else {
        std::cout << " delete : " << pNode.objectID() << "\n";
		NodeData* parentData = pNode.getParent();
		if (parentData) {
            parentData->removeChild(pNode.getName());
        }
    }
}


bool NodeDataManager::deleteActiveNode() {
    return deleteNode(*activeNode_);
}

bool NodeDataManager::clearNodeData() {
    deleting(root_);
    activeNode_ = nullptr;
    return true;
}

bool NodeDataManager::deleteNode(NodeData& pNode) {
	if (!pNode.getParent()) {
		std::cout << " rooot node  can't delete \n";
		return false;
	}
    std::cout << " deleteNode pNode->objectID(): " << pNode.objectID() << "\n";

    activeNode_ = pNode.getParent();
    deleting(pNode);
    std::cout << " deleteNode pNode: " << &pNode << "\n";
    return true;
}

NodeData* NodeDataManager::searchNodeByID(const std::string &objectID) {
    NodeData* p = &(root_);
    searchedNode_ = nullptr;
	searchingByID(p, objectID);
    return searchedNode_;
}

void NodeDataManager::searchingByID(NodeData* pNode, const std::string &objectID) {
    if(pNode->objectID() == objectID) {
        searchedNode_ = pNode;
        return ;
    }
    for(auto it = pNode->childMapRef().begin();it != pNode->childMapRef().end(); ++it) {
		searchingByID(&(it->second), objectID);
    }
}

NodeData* NodeDataManager::searchNodeByName(std::string& name) {
    NodeData* p = &(root_);
	searchedNode_ = nullptr;
	searchingByName(p, name);
	return searchedNode_;
}

void NodeDataManager::searchingByName(NodeData* pNode, std::string& name) {
	if (pNode->getName() == name) {
		searchedNode_ = pNode;
		return;
	}
	for (auto it = pNode->childMapRef().begin(); it != pNode->childMapRef().end(); ++it) {
		searchingByName(&(it->second), name);
	}
}

void NodeDataManager::preOrderReverse(std::function<void(NodeData*)> fun) {
    if(!root_.childMapRef().size())
        return ;
    std::cout << " preOrderReverse: " ;
    preOrderReverse(&root_,fun);
    if(activeNode_)
        std::cout << "   ->" << activeNode_->objectID();
    std::cout << std::endl ;
}

void NodeDataManager::preOrderReverse(NodeData* pNode,  std::function<void(NodeData*)> fun) {
    if(!pNode)
        return ;
	if (fun)
		fun(pNode);

    for (auto it = pNode->childMapRef().begin(); it != pNode->childMapRef().end(); ++it) {
		preOrderReverse(&(it->second));
    }
}

void NodeDataManager::delCurveBindingByName(std::string curveName) {
	if (!root_.childMapRef().size())
		return;
	std::cout << " preOrderReverse: ";
	delOneCurveBindingByName(&root_, curveName);
	if (activeNode_)
		std::cout << "   ->" << activeNode_->objectID();
	std::cout << std::endl;
}

void NodeDataManager::delOneCurveBindingByName(NodeData* pNode, std::string curveName) {
	if (!pNode)
		return;
	std::map<std::string, std::map<std::string, std::string>> bindingMap = pNode->NodeExtendRef().curveBindingRef().bindingMap();

    for (auto& an : bindingMap) {
		for (auto& prop : an.second) {
			if (curveName == prop.second)
				pNode->NodeExtendRef().curveBindingRef().deleteBindingDataItem(an.first, prop.first, prop.second); 
        }
    }

	for (auto it = pNode->childMapRef().begin(); it != pNode->childMapRef().end(); ++it) {
		delOneCurveBindingByName(&(it->second), curveName);
	}
}
}
