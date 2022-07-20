#include"AnimationData/animationData.h"

namespace raco::guiData {
animationData::animationData() {
}

animationData::animationData(int startTime, int endTime, int loopCount, int updateInterval, int playSpeed) :
							m_StartTime(startTime),
						    m_EndTime(endTime),
						    m_LoopCount(loopCount),
                            m_UpdateInterval(updateInterval),
                            m_PlaySpeed(playSpeed){}

void animationData::SetStartTime(const int startTime) {
	m_StartTime = startTime;
	return;
}

void animationData::SetEndTime(const int endTime) {
	m_EndTime = endTime;
	return;
}

void animationData::SetLoopCount(const int loopCount) {
	m_LoopCount = loopCount;
	return;
}

void animationData::SetUpdateInterval(const int updateInterval) {
	m_UpdateInterval = updateInterval;
    return;
}

void animationData::SetPlaySpeed(const int playSpeed) {
    m_PlaySpeed = playSpeed;
}

int animationData::GetStartTime() {
	return m_StartTime;
}

int animationData::GetEndTime() {
	return m_EndTime;
}

int animationData::GetLoopCount() {
	return m_LoopCount;
}

int animationData::GetUpdateInterval() {
    return m_UpdateInterval;
}

int animationData::GetPlaySpeed() {
    return m_PlaySpeed;
}

bool animationData::IsHaveNode(const std::string nodeName)
{
	return (m_NodeList.find(nodeName)!= m_NodeList.end()) ? true : false;
}

bool animationData::InsertNode(const std::string nodeName)
{
	m_NodeList.insert(nodeName);
	return true;
}

bool animationData::ModifyNode(const std::string src, const std::string dest)
{
	if (IsHaveNode(src))
	{
		m_NodeList.erase(src);
		m_NodeList.insert(dest);
	}
	else
	{
		m_NodeList.insert(dest);
	}
	return true;
}

bool animationData::DeleteNode(const std::string nodeName)
{
	if (IsHaveNode(nodeName))
	{
		m_NodeList.erase(nodeName);
	}
	return true;
}

const std::set<std::string>& animationData::getNodeList() {
	return m_NodeList;
}

//----------------------------------------------------------------------------------------------------
inline animationDataManager::animationDataManager(){}

animationDataManager& animationDataManager::GetInstance()
{
	static animationDataManager Instance;
	return Instance;
}


bool animationDataManager::InsertAmimation(std::string sampleProperty) {
	if (IsHaveAnimation(sampleProperty)){
		return false;
	}
	else
	{
        m_AnitnList[sampleProperty] = animationData(0, 200, 1, 17, 1.0);
		return true;
	}
}

bool animationDataManager::CopyAmimation(std::string src, std::string dest) {
	if(IsHaveAnimation(src))
	{
        m_AnitnList[dest] = animationData(m_AnitnList[src].GetStartTime(),m_AnitnList[src].GetEndTime(),m_AnitnList[src].GetLoopCount(),m_AnitnList[src].GetUpdateInterval(), m_AnitnList[src].GetPlaySpeed());
		return true;
	}
	else{
		return false;
	}
}

bool animationDataManager::DeleteAnimation(std::string sampleProperty) {
	if (IsHaveAnimation(sampleProperty)) {
		m_AnitnList.erase(sampleProperty);
		return true;
	} else {
		return false;
	}
}

bool animationDataManager::IsHaveAnimation(std::string samplerProperty) {
    return (m_AnitnList.find(samplerProperty) != m_AnitnList.end())?true:false;
}

bool animationDataManager::ModifyAnimation(std::string oldSampleProperty, std::string newSampleProperty) {
    if (IsHaveAnimation(oldSampleProperty)) {
        animationData data = m_AnitnList[oldSampleProperty];
        DeleteAnimation(oldSampleProperty);
        m_AnitnList.emplace(newSampleProperty, data);
        return true;
    }
    return false;
}

bool animationDataManager::ClearAniamtion() {
    m_AnitnList.clear();
    m_ActiveAnimation = std::string();
    return true;
}

void animationDataManager::SetActiveAnimation(std::string sampleProperty) {
    m_ActiveAnimation = sampleProperty;
}

std::string animationDataManager::GetActiveAnimation() {
    return m_ActiveAnimation;
}

animationData& animationDataManager::getAnimationData(const std::string samplerProperty) {
    return  m_AnitnList[samplerProperty];
}

animationData &animationDataManager::getActiveAnimationData() {
    return m_AnitnList[m_ActiveAnimation];
}

std::set<std::string> animationDataManager::getAniamtionNameList() {
	std::set<std::string> AniNameList;
	if (m_AnitnList.size() > 0)
	{
        for (const auto &Item : m_AnitnList)
		{
			AniNameList.insert(Item.first);
		}
	}
	return AniNameList;
}
}
