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

#include <common_widgets/NoContentMarginsLayout.h>

#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace raco::property_browser {

using PropertyBrowserGridLayout = raco::common_widgets::NoContentMarginsLayout<QGridLayout>;
using PropertyBrowserVBoxLayout = raco::common_widgets::NoContentMarginsLayout<QVBoxLayout>;
using PropertyBrowserHBoxLayout = raco::common_widgets::NoContentMarginsLayout<QHBoxLayout>;

}  // namespace raco::property_browser
