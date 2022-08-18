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

namespace raco::user_types {
// Engine default values copy&paste

constexpr int DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_SRC_COLOR{2};
constexpr int DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_DEST_COLOR{3};
constexpr int DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_SRC_ALPHA{1};
constexpr int DEFAULT_VALUE_MATERIAL_BLEND_FACTOR_DEST_ALPHA{1};

constexpr int DEFAULT_VALUE_MATERIAL_BLEND_OPERATION_COLOR{0};
constexpr int DEFAULT_VALUE_MATERIAL_BLEND_OPERATION_ALPHA{0};

constexpr int DEFAULT_VALUE_MATERIAL_CULL_MODE{2};

constexpr int DEFAULT_VALUE_MATERIAL_DEPTH_FUNCTION{4};

constexpr int DEFAULT_VALUE_TEXTURE_SAMPLER_TEXTURE_ADDRESS_MODE_CLAMP{0};

constexpr int DEFAULT_VALUE_TEXTURE_SAMPLER_TEXTURE_MIN_SAMPLING_METHOD_NEAREST{0};
constexpr int DEFAULT_VALUE_TEXTURE_SAMPLER_TEXTURE_MAG_SAMPLING_METHOD_NEAREST{0};

constexpr int DEFAULT_VALUE_TEXTURE_FORMAT_RGBA8{5};
constexpr int DEFAULT_VALUE_TEXTURE_ORIGIN_BOTTOM{0};

constexpr int DEFAULT_VALUE_RENDER_BUFFER_FORMAT{4};

constexpr int DEFAULT_VALUE_RENDER_LAYER_ORDER{0};

};  // namespace raco::user_types
