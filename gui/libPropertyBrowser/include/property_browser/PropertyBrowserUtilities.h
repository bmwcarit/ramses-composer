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

#include <algorithm>
#include <numeric>
#include <optional>

namespace raco::property_browser {

template<typename Container>
Container sorted(const Container& container) {
	Container result(container.begin(), container.end());
	std::sort(result.begin(), result.end());
	return result;
}

template <typename Container>
Container intersection(const std::optional<Container>& set_1, const Container& set_2) {
	if (set_1.has_value()) {
		Container result;
		std::set_intersection(set_1.value().begin(), set_1.value().end(), set_2.begin(), set_2.end(), std::inserter(result, result.end()));
		return result;
	}
	return set_2;
}

template <typename Container>
Container set_union(const std::optional<Container>& set_1, const Container& set_2) {
	if (set_1.has_value()) {
		Container result;
		std::set_union(set_1.value().begin(), set_1.value().end(), set_2.begin(), set_2.end(), std::inserter(result, result.end()));
		return result;
	}
	return set_2;
}

// This version only works with full C++17 support which gcc 8 doesn't have completely
/*
template <class T, class Container, class ReductionOp, class TransformOp>
constexpr T map_reduce_17(Container container, ReductionOp reduce, TransformOp transform) {
	auto result = std::transform_reduce(container.begin(), container.end(), std::optional<T>(), reduce, transform);
	if (result.has_value()) {
		return result.value();
	}
	return T();
}
*/

// This version uses the basic transform and accumulate functions available in gcc 8
template <class T, class Container, class ReductionOp, class TransformOp>
constexpr T map_reduce(Container container, ReductionOp reduce, TransformOp transform) {
	std::vector<T> transformed;
	std::transform(container.begin(), container.end(), std::inserter(transformed, transformed.end()), transform);

	auto result = std::accumulate(transformed.begin(), transformed.end(), std::optional<T>(), reduce);

	if (result.has_value()) {
		return result.value();
	}
	return T();
}


}  // namespace raco::property_browser
