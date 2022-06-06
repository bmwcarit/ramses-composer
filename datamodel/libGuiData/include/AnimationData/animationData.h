#pragma once
#include <string>
#include <map>
#include <set>

namespace raco::guiData {
class animationData{
public:
	animationData();
    animationData(int startTime, int endTime, int loopCount, int updateInterval, int playSpeed);

	void SetStartTime(const int startTime);
	void SetEndTime(const int endTime);
	void SetLoopCount(const int loopCount);
	void SetUpdateInterval(const int updateInterval);
    void SetPlaySpeed(const int playSpeed);
	//get
	int GetStartTime();
	int GetEndTime();
	int GetLoopCount();
	int GetUpdateInterval();
    int GetPlaySpeed();

	//Relate to  Node List
	bool IsHaveNode(const std::string nodeName);
	bool InsertNode(const std::string nodeName);
	bool ModifyNode(const std::string src, const std::string dest);
	bool DeleteNode(const std::string nodeName);
	const std::set<std::string>& getNodeList();
private:
	int m_StartTime;	
	int m_EndTime;
	int m_LoopCount;
	int m_UpdateInterval;
    double m_PlaySpeed;
	std::set<std::string> m_NodeList;
};

class animationDataManager {
private:
	animationDataManager();
public:
	static animationDataManager& GetInstance();
	~animationDataManager(){}
	animationDataManager(const animationDataManager&) = delete;
	animationDataManager& operator=(const animationDataManager&) = delete;
public:
	bool InsertAmimation(std::string sampleProperty);
	bool CopyAmimation(std::string src,std::string dest);
	bool DeleteAnimation(std::string sampleProperty);
	bool IsHaveAnimation(std::string samplerProperty);
    bool ModifyAnimation(std::string oldSampleProperty, std::string newSampleProperty);
    bool ClearAniamtion();
    void SetActiveAnimation(std::string sampleProperty);
    std::string GetActiveAnimation();
    animationData &getAnimationData(const std::string samplerProperty);
    animationData &getActiveAnimationData();
	std::set<std::string> getAniamtionNameList();
	const std::map<std::string, animationData> getAnitnList() { return m_AnitnList; };

private:
	std::map<std::string, animationData> m_AnitnList;
    std::string m_ActiveAnimation;
};
}
