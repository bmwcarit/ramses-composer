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

/**
 * Common location for all debug actions to make it easier to see what happens specificly for debug action, add new ones and manage includes
 */

#include "ui_mainwindow.h"
#include <QWidget>

namespace raco::core {
class CommandInterface;
}

void configureDebugActions(Ui::MainWindow* ui, QWidget* widget, raco::core::CommandInterface* commandInterface);
