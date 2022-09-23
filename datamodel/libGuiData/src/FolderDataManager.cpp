#include "FolderData/FolderDataManager.h"

Folder::Folder() {

}

void Folder::clear() {
    for (auto it = folderList_.begin(); it != folderList_.end(); it++) {
        (*it)->clear();
        delete (*it);
        (*it) = nullptr;
    }
    folderList_.clear();
}

std::string Folder::createDefaultFolder() {
    auto func = [=](std::string str)->bool {
        auto it = folderList_.begin();
        while (it != folderList_.end()) {
            if (str == (*it)->getFolderName()) {
                return false;
            }
            it++;
        }
        return true;
    };

    int index{1};
    std::string folder = "Node" + std::to_string(index);
    while (!func(folder)) {
        index++;
        folder = "Node" + std::to_string(index);
    }
    return folder;
}

std::string Folder::createDefaultCurve() {
    auto func = [=](std::string str)->bool {
        auto it = curveList_.begin();
        while (it != curveList_.end()) {
            if (str == (*it)->curve_) {
                return false;
            }
            it++;
        }
        return true;
    };

    int index{1};
    std::string curve = "Curve" + std::to_string(index);
    while (!func(curve)) {
        index++;
        curve = "Curve" + std::to_string(index);
    }
    return curve;
}

void Folder::setVisible(bool visible) {
    visible_ = visible;
}

bool Folder::isVisible() {
    return visible_;
}

void Folder::setParent(Folder *folder) {
    parent_ = folder;
}

Folder *Folder::parent() {
    return parent_;
}

void Folder::setFolderName(std::string name) {
    folderName_ = name;
}

std::string Folder::getFolderName() {
    return folderName_;
}

bool Folder::hasCurve(std::string curve) {
    for (auto it = curveList_.begin(); it != curveList_.end(); it++) {
        if ((*it)->curve_ == curve) {
            return true;
        }
    }
    return false;
}

void Folder::insertCurve(SCurveProperty *curveProp) {
    curveList_.push_back(curveProp);
}

bool Folder::insertCurve(std::string curve, bool bVisible) {
    for (auto it = curveList_.begin(); it != curveList_.end(); it++) {
        if ((*it)->curve_ == curve) {
            return false;
        }
    }
    SCurveProperty *curveProp = new SCurveProperty(curve);
    curveProp->visible_ = bVisible;
    curveList_.push_back(curveProp);
    return true;
}

SCurveProperty *Folder::takeCurve(std::string curve) {
    for (auto it = curveList_.begin(); it != curveList_.end(); it++) {
        if ((*it)->curve_ == curve) {
            SCurveProperty *curveProp = *it;
            curveList_.erase(it);
            return curveProp;
        }
    }
    return nullptr;
}

bool Folder::deleteCurve(std::string curve) {
    for (auto it = curveList_.begin(); it != curveList_.end(); it++) {
        if ((*it)->curve_ == curve) {
            SCurveProperty *curveProp = *it;
            curveList_.erase(it);
            delete curveProp;
            curveProp = nullptr;
            return true;
        }
    }
    return false;
}

SCurveProperty *Folder::getCurve(std::string curve) {
    for (auto it = curveList_.begin(); it != curveList_.end(); it++) {
        if ((*it)->curve_ == curve) {
            return *it;
        }
    }
    return nullptr;
}

std::list<SCurveProperty *> Folder::getCurveList() {
    return curveList_;
}

bool Folder::hasFolder(std::string folderName) {
    for (auto it = folderList_.begin(); it != folderList_.end(); it++) {
        if ((*it)->getFolderName() == folderName) {
            return true;
        }
    }
    return false;
}

void Folder::insertFolder(Folder *folder) {
    folderList_.push_back(folder);
}

bool Folder::insertFolder(std::string folderName) {
    for (auto it = folderList_.begin(); it != folderList_.end(); it++) {
        if ((*it)->getFolderName() == folderName) {
            return false;
        }
    }
    Folder *folder = new Folder;
    folder->setFolderName(folderName);
    folderList_.push_back(folder);
    return true;
}

bool Folder::deleteFolder(std::string folderName) {
    for (auto it = folderList_.begin(); it != folderList_.end(); it++) {
        if ((*it)->getFolderName() == folderName) {
            Folder *folder = *it;
            folderList_.erase(it);
            folder->clear();
            delete folder;
            folder = nullptr;
            return true;
        }
    }
    return false;
}

Folder *Folder::takeFolder(std::string folderName) {
    for (auto it = folderList_.begin(); it != folderList_.end(); it++) {
        if ((*it)->getFolderName() == folderName) {
            Folder *folder = *it;
            folderList_.erase(it);
            return folder;
        }
    }
    return nullptr;
}

Folder *Folder::getFolder(std::string folderName) {
    for (auto it = folderList_.begin(); it != folderList_.end(); it++) {
        if ((*it)->getFolderName() == folderName) {
            return *it;
        }
    }
    return nullptr;
}

std::list<Folder *> Folder::getFolderList() {
    return folderList_;
}

FolderDataManager::FolderDataManager() {
    defaultFolder_ = new Folder;
}

void FolderDataManager::clear() {
    defaultFolder_->clear();
}

Folder *FolderDataManager::getDefaultFolder() {
    return defaultFolder_;
}

bool FolderDataManager::hasCurve(std::string curveName) {
    auto getFolder = [=](Folder *f, std::string node)->bool {
        f = f->getFolder(node);
        if (!f) {
            return false;
        }
        return true;
    };

    QStringList list = QString::fromStdString(curveName).split("|");
    std::string curve = list.takeLast().toStdString();
    Folder *folder = defaultFolder_;
    for (const QString &node : list) {
        if (!getFolder(folder, node.toStdString())) {
            return false;
        }
    }
    SCurveProperty *curveProp = folder->getCurve(curve);
    if (curveProp) {
        return true;
    }
    return false;
}

bool FolderDataManager::folderFromCurveName(std::string curveName, Folder *folder, SCurveProperty *curveProp) {
    auto getFolder = [=](Folder *f, std::string node)->bool {
        f = f->getFolder(node);
        if (!f) {
            return false;
        }
        return true;
    };

    QStringList list = QString::fromStdString(curveName).split("|");
    std::string curve = list.takeLast().toStdString();
    folder = defaultFolder_;
    for (const QString &node : list) {
        if (!getFolder(folder, node.toStdString())) {
            return false;
        }
    }
    curveProp = folder->getCurve(curve);
    return true;
}

bool FolderDataManager::curveNameFromFolder(std::string curve, Folder *folder, std::string &curveName) {
    if (!folder) {
        return false;
    }
    curveName = folder->getFolderName() + "|" + curve;
    while (!folder->parent()) {
        folder = folder->parent();
        std::string str = folder->getFolderName() + "|";
        curveName = str + curveName;
    }
    return true;
}



