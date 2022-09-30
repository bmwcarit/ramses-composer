/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Link.h"

#include <iterator>
#include <map>
#include <set>
#include <string>

namespace raco::core {

class LinkIterator {
public:
	LinkIterator(std::map<std::string, std::set<SLink>>::const_iterator start, std::map<std::string, std::set<SLink>>::const_iterator end);

	SLink operator*() const;

	LinkIterator& operator++();

	bool operator==(const LinkIterator& other) const;
	bool operator!=(const LinkIterator& other) const;

private:
	std::map<std::string, std::set<SLink>>::const_iterator currentContIt_;
	std::map<std::string, std::set<SLink>>::const_iterator endContIt_;

	std::set<SLink>::iterator currentLinkIt_;
	std::set<SLink>::iterator endLinkIt_;
};

struct LinkContainer {
	LinkIterator begin() const;
	LinkIterator end() const;

	size_t size() const;

	void addLink(SLink link);
	void removeLink(SLink link);

	void clear();

	SLink findLinkByObjectID(SLink link) const;
	static SLink findLinkByObjectID(const std::map<std::string, std::set<SLink>>& linksByEndPointID, SLink link);

	// This map contains all links used by the project,
	// using the link start object ID as the key value, for easier lookup.
	// Mostly used for link-related functions in raco::core::Queries.
	std::map<std::string, std::set<SLink>> linkStartPoints_;

	// This map contains all links used by the project,
	// using the link end object ID as the key value, for easier lookup.
	std::map<std::string, std::set<SLink>> linkEndPoints_;
};

}  // namespace raco::core

template <>
struct std::iterator_traits<raco::core::LinkIterator> {
	using value_type = raco::core::SLink;
	using reference = raco::core::SLink&;
	using iterator_category = std::forward_iterator_tag;
};
