#include "FolderData/FolderDataManager.h"

FolderData::FolderData() {

}

void FolderData::setVisible(bool visible) {
    visible_ = visible;
}

bool FolderData::isVisible() {
    return visible_;
}

bool FolderData::hasFile(std::string curve) {
    auto it = file2CurveMap_.begin();
    while (it != file2CurveMap_.end()) {
        if (curve == it->second.curve_) {
            return true;
        }
        it++;
    }
    return false;
}

std::string FolderData::getFile(std::string curve) {
    auto it = file2CurveMap_.begin();
    while (it != file2CurveMap_.end()) {
        if (curve == it->second.curve_) {
            return it->first;
        }
        it++;
    }
    return std::string();
}

bool FolderData::insertCurve(std::string file, std::string curve, bool visible) {
    auto it = file2CurveMap_.begin();
    while (it != file2CurveMap_.end()) {
        if (file == it->first) {
            return false;
        }
        it++;
    }
    SCurveProperty curvePro(curve);
    curvePro.visible_ = visible;
    file2CurveMap_.emplace(file, curvePro);
    return true;
}

bool FolderData::deleteCurve(std::string file) {
    auto it = file2CurveMap_.find(file);
    if (it != file2CurveMap_.end()) {
        file2CurveMap_.erase(it);
        return true;
    }
    return false;
}

bool FolderData::hasCurve(std::string file) {
    auto it = file2CurveMap_.begin();
    while (it != file2CurveMap_.end()) {
        if (file == it->first) {
            return true;
        }
        it++;
    }
    return false;
}

SCurveProperty FolderData::getCurve(std::string file) {
    auto it = file2CurveMap_.begin();
    while (it != file2CurveMap_.end()) {
        if (it->first == file) {
            return it->second;
        }
        it++;
    }
    return SCurveProperty();
}

bool FolderData::swapCurve(std::string oldKey, std::string newKey) {
    for (auto it = file2CurveMap_.begin(); it != file2CurveMap_.end(); it++) {
        if (it->first == oldKey) {
            SCurveProperty curvePro = it->second;
            it = file2CurveMap_.erase(it);
            file2CurveMap_.emplace(newKey, curvePro);
            return true;
        }
    }
    return false;
}

bool FolderData::modifyCurve(std::string file, std::string curve, bool visible) {
    for (auto it = file2CurveMap_.begin(); it != file2CurveMap_.end(); it++) {
        if (it->first == file) {
            file2CurveMap_.erase(it);

            SCurveProperty curvePro(curve);
            curvePro.visible_ = visible;
            file2CurveMap_.emplace(file, curvePro);
            return true;
        }
    }
    return false;
}

std::map<std::string, SCurveProperty> FolderData::getCurveMap() {
    return file2CurveMap_;
}

FolderDataManager::FolderDataManager() {

}

std::string FolderDataManager::createDefaultFolder() {
    auto func = [=](std::string str)->bool {
        auto it = folderMap_.begin();
        while (it != folderMap_.end()) {
            if (str == it->first) {
                return false;
            }
            it++;
        }
        return true;
    };

    std::string folder = "folder" + std::to_string(index_);
    while (!func(folder)) {
        index_++;
        folder = "folder" + std::to_string(index_);
    }
    return folder;
}

void FolderDataManager::clear() {
    folderMap_.clear();
    file2CurveMap_.clear();
    index_ = 1;
}

bool FolderDataManager::insertCurve(std::string file, std::string curve, bool visible) {
    auto it = file2CurveMap_.begin();
    while (it != file2CurveMap_.end()) {
        if (file == it->first) {
            return false;
        }
        it++;
    }
    SCurveProperty curvePro(curve);
    curvePro.visible_ = visible;
    file2CurveMap_.emplace(file, curvePro);
    return true;
}

bool FolderDataManager::deleteCurve(std::string file) {
    auto it = file2CurveMap_.find(file);
    if (it != file2CurveMap_.end()) {
        file2CurveMap_.erase(it);
        return true;
    }
    return false;
}

bool FolderDataManager::hasCurve(std::string file) {
    auto it = file2CurveMap_.begin();
    while (it != file2CurveMap_.end()) {
        if (file == it->first) {
            return true;
        }
        it++;
    }
    return false;
}

SCurveProperty FolderDataManager::getCurve(std::string file) {
    auto it = file2CurveMap_.begin();
    while (it != file2CurveMap_.end()) {
        if (it->first == file) {
            return it->second;
        }
        it++;
    }
    return SCurveProperty();
}

bool FolderDataManager::swapCurve(std::string oldKey, std::string newKey) {
    for (auto it = file2CurveMap_.begin(); it != file2CurveMap_.end(); it++) {
        if (it->first == oldKey) {
            SCurveProperty curvePro = it->second;
            it = file2CurveMap_.erase(it);
            file2CurveMap_.emplace(newKey, curvePro);
            return true;
        }
    }
    return false;
}

bool FolderDataManager::modifyCurve(std::string file, std::string curve, bool visible) {
    for (auto it = file2CurveMap_.begin(); it != file2CurveMap_.end(); it++) {
        if (it->first == file) {
            file2CurveMap_.erase(it);

            SCurveProperty curvePro(curve);
            curvePro.visible_ = visible;
            file2CurveMap_.emplace(file, curvePro);
            return true;
        }
    }
    return false;
}

bool FolderDataManager::hasFile(std::string curve) {
    auto it = file2CurveMap_.begin();
    while (it != file2CurveMap_.end()) {
        if (curve == it->second.curve_) {
            return true;
        }
        it++;
    }
    return false;
}

std::string FolderDataManager::getFile(std::string curve) {
    auto it = file2CurveMap_.begin();
    while (it != file2CurveMap_.end()) {
        if (curve == it->second.curve_) {
            return it->first;
        }
        it++;
    }
    return std::string();
}

bool FolderDataManager::insertFolderData(std::string folder, FolderData folderData) {
    auto it = folderMap_.begin();
    while (it != folderMap_.end()) {
        if (folder == it->first) {
            return false;
        }
        it++;
    }
    folderMap_.emplace(folder, folderData);
    return true;
}

bool FolderDataManager::hasFolderData(std::string folder) {
    auto it = folderMap_.begin();
    while (it != folderMap_.end()) {
        if (it->first == folder) {
            return true;
        }
        it++;
    }
    return false;
}

FolderData FolderDataManager::getFolderData(std::string folder) {
    auto it = folderMap_.begin();
    while (it != folderMap_.end()) {
        if (it->first == folder) {
            return it->second;
        }
        it++;
    }
    return FolderData();
}

bool FolderDataManager::deleteFolderData(std::string folder) {
    auto it = folderMap_.find(folder);
    if (it != folderMap_.end()) {
        folderMap_.erase(it);
        return true;
    }
    return false;
}

bool FolderDataManager::swapFolder(std::string oldFolder, std::string newFolder) {
    auto it = folderMap_.begin();
    while (it != folderMap_.end()) {
        if (oldFolder == it->first) {
            FolderData folderData = it->second;
            folderMap_.erase(it);
            folderMap_.emplace(newFolder, folderData);
            return true;
        }
        it++;
    }
    return false;
}

bool FolderDataManager::replaceFolder(std::string folder, FolderData folderData) {
    auto it = folderMap_.begin();
    while (it != folderMap_.end()) {
        if (folder == it->first) {
            folderMap_.erase(it);
            folderMap_.emplace(folder, folderData);
            return true;
        }
        it++;
    }
    return false;
}

bool FolderDataManager::replaceFolderCurve(std::string folder, std::string file, std::string curve, bool visible) {
    auto it = folderMap_.find(folder);
    if (it != folderMap_.end()) {
        return it->second.modifyCurve(file, curve, visible);
    }
    return false;
}

bool FolderDataManager::setFolderVisible(std::string folder, bool visible) {
    auto it = folderMap_.find(folder);
    if (it != folderMap_.end()) {
        it->second.setVisible(visible);
        return true;
    }
    return false;
}

std::map<std::string, FolderData> FolderDataManager::getFolderMap() {
    return folderMap_;
}

