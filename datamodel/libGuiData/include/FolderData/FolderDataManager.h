#ifndef FOLDERDATAMANAGER_H
#define FOLDERDATAMANAGER_H
#include "qobject.h"
#include <list>
#include <mutex>
#include <string>
#include <any>
#include <map>
#include <regex>

struct SCurveProperty {
    std::string curve_;
    bool visible_{true};

    SCurveProperty(std::string curve = std::string()) {
        curve_ = curve;
    }
};

class FolderData {
public:
    FolderData();

    void setVisible(bool visible);
    bool isVisible();

    bool hasFile(std::string curve);
    std::string getFile(std::string curve);

    bool insertCurve(std::string file, std::string curve, bool visible = true);
    bool deleteCurve(std::string file);
    bool hasCurve(std::string file);
    SCurveProperty getCurve(std::string file);
    bool swapCurve(std::string oldKey, std::string newKey);
    bool modifyCurve(std::string file, std::string curve, bool visible);
    std::map<std::string, SCurveProperty> getCurveMap();
private:
    std::map<std::string, SCurveProperty> file2CurveMap_;
    bool visible_{true};
};

class FolderDataManager {
public:
    FolderDataManager();

    std::string createDefaultFolder();

    void clear();
    bool insertCurve(std::string file, std::string curve, bool visible = true);
    bool deleteCurve(std::string file);
    bool hasCurve(std::string file);
    SCurveProperty getCurve(std::string file);
    bool swapCurve(std::string oldKey, std::string newKey);
    bool modifyCurve(std::string file, std::string curve, bool visible);

    bool hasFile(std::string curve);
    std::string getFile(std::string curve);

    bool insertFolderData(std::string folder, FolderData folderData);
    bool hasFolderData(std::string folder);
    FolderData getFolderData(std::string folder);
    bool deleteFolderData(std::string folder);
    bool swapFolder(std::string oldFolder, std::string newFolder);
    bool replaceFolder(std::string folder, FolderData folderData);
    bool replaceFolderCurve(std::string folder, std::string file, std::string curve, bool visible);
    bool setFolderVisible(std::string folder, bool visible);
    std::map<std::string, FolderData> getFolderMap();
private:
    std::map<std::string, FolderData> folderMap_;
    std::map<std::string, SCurveProperty> file2CurveMap_;
    int index_{1};
};

#endif // FOLDERDATAMANAGER_H
