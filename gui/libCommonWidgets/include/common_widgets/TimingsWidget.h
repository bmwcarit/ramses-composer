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

#include "common_widgets/NoContentMarginsLayout.h"
#include <QGridLayout>
#include <QLabel>
#include <QObject>
#include <QWidget>
#include <chrono>

namespace raco::common_widgets {

class TimingsModel : public QObject {
	Q_OBJECT
public:
	explicit TimingsModel(QObject* parent = nullptr) : QObject{parent} {}

public Q_SLOTS:
	void addLogicEngineTotalExecutionDuration(long long microseconds) {
		logicEngineExecutionTimeAverage_ = static_cast<long long>(logicEngineExecutionTimeAverage_ * averageDampening_ + (microseconds * (1.0 - averageDampening_)));
		Q_EMIT logicEngineExecutionTimeAverageUpdated(logicEngineExecutionTimeAverage_);
	}

Q_SIGNALS:
	void logicEngineExecutionTimeAverageUpdated(long long value);

private:
	double averageDampening_{0.9};
	long long logicEngineExecutionTimeAverage_{0};
};

class TimingsWidget : public QWidget {
public:
	explicit TimingsWidget(TimingsModel* model, QWidget* parent = nullptr) : QWidget{parent} {
		layout_.addWidget(new QLabel{"Total logic engine execution time:", this}, 0, 0);
		auto valueLabel = new QLabel{"", this};
		QObject::connect(model, &TimingsModel::logicEngineExecutionTimeAverageUpdated, this, [this, valueLabel](long long value) {
			valueLabel->setText(QString{"%1"}.arg(value));
		});
		layout_.addWidget(valueLabel, 0, 1);
	}

private:
	NoContentMarginsLayout<QGridLayout> layout_{this};
};

}	// namespace raco::common_widgets