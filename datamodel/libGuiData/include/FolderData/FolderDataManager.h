#ifndef FOLDERDATAMANAGER_H
#define FOLDERDATAMANAGER_H
#include "qobject.h"
#include <list>
#include <mutex>
#include <string>
#include <any>
#include <map>
#include <regex>
#include <QStringList>

struct SCurveProperty {
    std::string curve_;
    bool visible_{true};

    SCurveProperty(std::string curve = std::string()) {
        curve_ = curve;
    }
};

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
    void insertCurve(SCurveProperty *curveProp);
    bool insertCurve(std::string curve, bool bVisible = true);
    SCurveProperty *takeCurve(std::string curve);
    bool deleteCurve(std::string curve);
    SCurveProperty *getCurve(std::string curve);
    std::list<SCurveProperty*> getCurveList();

    bool hasFolder(std::string folderName);
    void insertFolder(Folder *folder);
    bool insertFolder(std::string folderName);
    bool deleteFolder(std::string folderName);
    Folder *takeFolder(std::string folderName);
    Folder *getFolder(std::string folderName);
    std::list<Folder*> getFolderList();
private:
    std::string folderName_;
    std::list<SCurveProperty*> curveList_;
    std::list<Folder*> folderList_;
    Folder *parent_{nullptr};
    bool visible_{true};
};

class FolderDataManager {
public:
    FolderDataManager();

    void clear();
    Folder *getRootFolder();

    bool isCurve(std::string curveName);
    bool folderFromPath(std::string path, Folder **folder);
    bool curveFromPath(std::string curveName,  Folder **folder, SCurveProperty **curveProp);
    bool pathFromCurve(std::string curve, Folder *folder, std::string &path);
private:
    Folder *rootFolder_{nullptr};
};

#endif // FOLDERDATAMANAGER_H
