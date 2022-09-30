/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "core/LinkContainer.h"

#include "core/EditorObject.h"

namespace raco::core {

LinkIterator::LinkIterator(std::map<std::string, std::set<SLink>>::const_iterator start, std::map<std::string, std::set<SLink>>::const_iterator end) {
	currentContIt_ = start;
	endContIt_ = end;
	if (currentContIt_ != endContIt_) {
		currentLinkIt_ = currentContIt_->second.begin();
		endLinkIt_ = currentContIt_->second.end();
	}
}

SLink LinkIterator::operator*() const {
	if (currentContIt_ != endContIt_) {
		return *currentLinkIt_;
	}
	return {};
}

LinkIterator& LinkIterator::operator++() {
	if (currentContIt_ != endContIt_) {
		if (currentLinkIt_ != endLinkIt_) {
			++currentLinkIt_;
		}
		if (currentLinkIt_ == endLinkIt_) {
			++currentContIt_;
			if (currentContIt_ != endContIt_) {
				currentLinkIt_ = currentContIt_->second.begin();
				endLinkIt_ = currentContIt_->second.end();
			}
		}
	}
	return *this;
}

bool LinkIterator::operator==(const LinkIterator& other) const {
	if (currentContIt_ == endContIt_) {
		return currentContIt_ == other.currentContIt_;
	}
	return currentContIt_ == other.currentContIt_ &&
		   currentLinkIt_ == other.currentLinkIt_;
}

bool LinkIterator::operator!=(const LinkIterator& other) const {
	if (currentContIt_ == endContIt_) {
		return currentContIt_ != other.currentContIt_;
	}
	return currentContIt_ != other.currentContIt_ ||
		   currentLinkIt_ != other.currentLinkIt_;
}

LinkIterator LinkContainer::begin() const {
	return LinkIterator(linkEndPoints_.begin(), linkEndPoints_.end());
}

LinkIterator LinkContainer::end() const {
	return LinkIterator(linkEndPoints_.end(), linkEndPoints_.end());
}

size_t LinkContainer::size() const {
	size_t s{0};
	for (const auto& [id, cont] : linkEndPoints_) {
		s += cont.size();
	}
	return s;
}

void LinkContainer::addLink(SLink link) {
	linkStartPoints_[link->startObject_.asRef()->objectID()].insert(link);
	linkEndPoints_[link->endObject_.asRef()->objectID()].insert(link);
}

void LinkContainer::removeLink(SLink link) {
	const auto& startObjID = link->startObject_.asRef()->objectID();
	linkStartPoints_[startObjID].erase(link);
	if (linkStartPoints_[startObjID].empty()) {
		linkStartPoints_.erase(startObjID);
	}

	const auto& endObjID = link->endObject_.asRef()->objectID();
	linkEndPoints_[endObjID].erase(link);
	if (linkEndPoints_[endObjID].empty()) {
		linkEndPoints_.erase(endObjID);
	}
}

void LinkContainer::clear() {
	linkStartPoints_.clear();
	linkEndPoints_.clear();
}

SLink LinkContainer::findLinkByObjectID(SLink link) const {
	return findLinkByObjectID(linkEndPoints_, link);
}

SLink LinkContainer::findLinkByObjectID(const std::map<std::string, std::set<SLink>>& linksByEndPointID, SLink link) {
	auto linkEndObjID = (*link->endObject_)->objectID();

	auto linkEndPointIt = linksByEndPointID.find(linkEndObjID);

	if (linkEndPointIt != linksByEndPointID.end()) {
		auto& endPointLinks = linkEndPointIt->second;
		for (const auto& endPointLink : endPointLinks) {
			if (compareLinksByObjectID(*endPointLink, *link)) {
				return endPointLink;
			}
		}
	}

	return nullptr;
}

}  // namespace raco::core