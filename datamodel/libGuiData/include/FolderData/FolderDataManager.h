#ifndef FOLDERDATAMANAGER_H
#define FOLDERDATAMANAGER_H
#include "core/StructCommon.h"
#include "qobject.h"
#include <list>
#include <mutex>
#include <string>
#include <any>
#include <map>
#include <regex>
#include <QStringList>
#include "core/ChangeBase.h"

namespace raco::guiData {
class Folder {
public:
    Folder();

    void clear();
    std::string createDefaultFolder();
    std::string createDefaultCurve();

    void setVisible(bool visible);
    bool isVisible();

    void setParent(Folder *folder);
    Folder *parent();

    void setFolderName(std::string name);
    std::string getFolderName();

    bool hasCurve(std::string curve);
    void insertCurve(STRUCT_CURVE_PROP *curveProp);
    bool insertCurve(std::string curve, bool bVisible = true);
    STRUCT_CURVE_PROP *takeCurve(std::string curve);
    bool deleteCurve(std::string curve);
    STRUCT_CURVE_PROP *getCurve(std::string curve);
    std::list<STRUCT_CURVE_PROP*> getCurveList();

    bool hasFolder(std::string folderName);
    void insertFolder(Folder *folder);
    bool insertFolder(std::string folderName);
    bool deleteFolder(std::string folderName);
    Folder *takeFolder(std::string folderName);
    Folder *getFolder(std::string folderName);
    std::list<Folder*> getFolderList();
private:
    std::string folderName_;
    std::list<STRUCT_CURVE_PROP*> curveList_;
    std::list<Folder*> folderList_;
    Folder *parent_{nullptr};
    bool visible_{true};
};

class FolderDataManager {
public:
    static FolderDataManager &GetInstance();
    ~FolderDataManager();
    FolderDataManager(const FolderDataManager&) = delete;
    FolderDataManager& operator=(const FolderDataManager&) = delete;

    void merge(QVariant data);
    void mergeFolder(Folder *folder, STRUCT_FOLDER folderData);

    STRUCT_FOLDER converFolderData();
    void fillFolderData(STRUCT_FOLDER &data, Folder *folder);

    void clear();
    Folder *getRootFolder();

    bool isCurve(std::string curveName);
    bool folderFromPath(std::string path, Folder **folder);
    bool curveFromPath(std::string curveName,  Folder **folder, STRUCT_CURVE_PROP **curveProp);
    bool pathFromCurve(std::string curve, Folder *folder, std::string &path);

private:
    FolderDataManager();
private:
    Folder *rootFolder_{nullptr};
};
}

#endif // FOLDERDATAMANAGER_H
