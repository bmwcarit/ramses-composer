#include "NodeData/nodeManager.h"

namespace raco::guiData {
NodeDataManager& NodeDataManager::GetInstance() {
	// TODO: 在此处插入 return 语句
	static NodeDataManager Instance;
	return Instance;
}
/**
 * @brief NodeTree::insertNode
 * @note 在当前选中的结点处，插入子结点，
 *       如果树为空的话，插入的当前结点是根节点
 *       activeNode 指向当前插入的结点
 */
void NodeDataManager::insertNode(NodeData& pNode) {
    // 如果这个树是一个空树，插入的结点为根结点
    std::cout << "nodeName = " << pNode.getName() << std::endl;
    //if(!root_) {
    //    root_ = &pNode;
    //    activeNode_ = &pNode;
    //    pNode.setParent(nullptr);
    //}else{ // 设置结点关系
        std::cout << "activeNode_ = " << activeNode_ << std::endl;
        pNode.setParent(activeNode_);
        activeNode_->childMapRef().emplace(pNode.getName(),pNode);
		activeNode_ =&(activeNode_->childMapRef().find(pNode.getName())->second);
    //}
}

/**
 * @brief NodeTree::deleting
 * @note  deleteNode的子函数，递归删除结点
 */
void NodeDataManager::deleting(NodeData& pNode) {
    if(!pNode.childMapRef().empty()) {
        // 1 删除子结点
		std::map<std::string,NodeData> tempList = pNode.childMapRef();
        for(auto& it: tempList){
            deleting(it.second);
        }
        // 2 清空子结点表
		pNode.childMapRef().clear();
        deleting(pNode);
        //        // 3 父结点中删除记录，暂时不能删除，会修改childlist的长度
        //        if(pNode->getParent())
        //            pNode->getParent()->getChildList().remove(pNode);
    }else {
        // 删除结点，所有的结点都是new出来的，也就是说所有的结点放在堆区
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

/**
 * @brief NodeTree::deleteNode
 * @note 删除 pNode 指向的结点及其子结点
 */
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

/**
 * @brief NodeTree::searchNode
 * @note  根据objectID查找相应的结点
 * @return 找到的话返回结点地址，找不到的返回空
 */
NodeData* NodeDataManager::searchNodeByID(const std::string &objectID) {
    std::cout << " objectID: " << objectID << "\n";

    NodeData* p = &(root_);  // 第一个结点 
    //NodeData* p =&(root_.childMapRef().begin()->second);  // 第一个结点  
    searchedNode_ = nullptr;
	searchingByID(p, objectID);
    return searchedNode_;
}
/**
 * @brief NodeTree::searching
 * @note  searchNode的子函数，递归进行查找
 */
void NodeDataManager::searchingByID(NodeData* pNode, const std::string &objectID) {
    std::cout << " searching pNode->objectID(): " << pNode->objectID() << "\n";
    if(pNode->objectID() == objectID) {
        searchedNode_ = pNode;
        return ;
    }
    for(auto it = pNode->childMapRef().begin();it != pNode->childMapRef().end(); ++it) {
		searchingByID(&(it->second), objectID);
    }
}


NodeData* NodeDataManager::searchNodeByName(std::string& name) {
	std::cout << " searchNodeByName name: " << name << "\n";

    NodeData* p = &(root_);  // 第一个结点
	searchedNode_ = nullptr;
	searchingByName(p, name);
	return searchedNode_;
}
/**
 * @brief NodeTree::searching
 * @note  searchNode的子函数，递归进行查找
 */
void NodeDataManager::searchingByName(NodeData* pNode, std::string& name) {
	std::cout << " searchingByName pNode->getName(): " << pNode->getName() << "\n";
	if (pNode->getName() == name) {
		searchedNode_ = pNode;
		return;
	}
	for (auto it = pNode->childMapRef().begin(); it != pNode->childMapRef().end(); ++it) {
		searchingByName(&(it->second), name);
	}
}

/**
 * @brief NodeTree::preOrderReverse
 * @note  先序遍历
 */
void NodeDataManager::preOrderReverse(std::function<void(NodeData*)> fun) {
    if(!root_.childMapRef().size())
        return ;
    std::cout << " preOrderReverse: " ;
	preOrderReverse(&root_,fun);
    if(activeNode_)
        std::cout << "   ->" << activeNode_->objectID();
    std::cout << std::endl ;
}


/**
 * @brief NodeTree::preOrderReverse
 * @note  先序遍历所传递结点
 */
void NodeDataManager::preOrderReverse(NodeData* pNode,  std::function<void(NodeData*)> fun) {
    if(!pNode)
        return ;
	if (fun)
		fun(pNode);
    std::cout << pNode->objectID() << "  ";

    for (auto it = pNode->childMapRef().begin(); it != pNode->childMapRef().end(); ++it) {
		preOrderReverse(&(it->second));
    }
}

}
