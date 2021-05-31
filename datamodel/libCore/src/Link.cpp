/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/GENIVI/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "core/Link.h"
#include "core/EditorObject.h"
#include "core/PropertyDescriptor.h"

namespace raco::core {

Link::Link(): ClassWithReflectedMembers(getProperties())
{
}

Link::Link(const PropertyDescriptor& start, const PropertyDescriptor& end, bool isValid) : ClassWithReflectedMembers(getProperties()) {
	*startObject_ = start.object();
	startProp_->set(start.propertyNames());
	*endObject_ = end.object();
	endProp_->set(end.propertyNames());
	isValid_ = isValid;
}

bool Link::isValid() const {
	return isValid_.asBool();
}

std::vector<std::pair<std::string, ValueBase*>> Link::getProperties() {
	return {{"startObject", &startObject_},
		{"startProp", &startProp_},
		{"endObject", &endObject_},
		{"endProp", &endProp_},
		{"isValid", &isValid_}};
}

LinkDescriptor Link::descriptor() const {
	return LinkDescriptor{startProp(),endProp(), isValid()};
}

std::vector<std::string> Link::startPropertyNamesVector() const {
	return startProp_->asVector<std::string>();
}

std::vector<std::string> Link::endPropertyNamesVector() const {
	return endProp_->asVector<std::string>();
}

PropertyDescriptor Link::startProp() const {
	return PropertyDescriptor{*startObject_, startProp_->asVector<std::string>()};
}

PropertyDescriptor Link::endProp() const {
	return PropertyDescriptor{*endObject_, endProp_->asVector<std::string>()};
}

bool Link::compareStartPropertyNames(const std::vector<std::string>& propertyNames) {
	return startProp_->compare(propertyNames);
}

bool Link::compareEndPropertyNames(const std::vector<std::string>& propertyNames) {
	return endProp_->compare(propertyNames);
}

SLink Link::cloneLinkWithTranslation(const SLink& link, std::function<SEditorObject(SEditorObject)> translateRef) {
	auto clonedLink = std::make_shared<Link>(*link);
	clonedLink->startObject_ = translateRef(clonedLink->startObject_.asRef());
	clonedLink->endObject_ = translateRef(clonedLink->endObject_.asRef());
	return clonedLink;
}

bool operator==(const LinkDescriptor& lhs, const LinkDescriptor& rhs) {
	return lhs.start.object() == rhs.start.object() &&
		   lhs.start.propertyNames() == rhs.start.propertyNames() &&
		   lhs.end.object() == rhs.end.object() &&
		   lhs.end.propertyNames() == rhs.end.propertyNames();
}

bool operator<(const LinkDescriptor& lhs, const LinkDescriptor& rhs) {
	return lhs.start.object() < rhs.start.object() ||
		   lhs.start.object() == rhs.start.object() &&
			   (lhs.start.propertyNames() < rhs.start.propertyNames() ||
				   lhs.start.propertyNames() == rhs.start.propertyNames() && (lhs.end.object() < rhs.end.object() ||
																				 lhs.end.object() == rhs.end.object() && lhs.end.propertyNames() < rhs.end.propertyNames()));
}

bool compareLinksByObjectID(const Link& left, const Link& right) {
	return (*left.startObject_)->objectID() == (*right.startObject_)->objectID() &&
		   (*left.endObject_)->objectID() == (*right.endObject_)->objectID() &&
		   *left.startProp_ == *right.startProp_ && *left.endProp_ == *right.endProp_;
}

}  // namespace raco::core