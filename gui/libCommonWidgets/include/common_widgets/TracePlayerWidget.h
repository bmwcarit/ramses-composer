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

#include "components/FileChangeMonitorImpl.h"
#include "components/TracePlayer.h"

#include <functional>
#include <string>
#include <vector>

#include <QWidget>

class QDoubleSpinBox;
class QEvent;
class QFrame;
class QGridLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QSlider;
class QSpinBox;
class QString;
class QTextEdit;

namespace raco::core {
enum class ErrorLevel;
}  // namespace raco::core
namespace raco::components {
class TracePlayer;
using TraceFileChangeMonitor = ProjectFileChangeMonitor;
}  // namespace raco::components

namespace raco::common_widgets {
class TracePlayerWidget : public QWidget {
	Q_OBJECT

public:
	~TracePlayerWidget();
	TracePlayerWidget() = delete;
	TracePlayerWidget(const TracePlayerWidget&) = delete;
	TracePlayerWidget& operator=(const TracePlayerWidget&) = delete;
	TracePlayerWidget(const TracePlayerWidget&&) = delete;
	TracePlayerWidget& operator=(const TracePlayerWidget&&) = delete;
	TracePlayerWidget(const QString& widgetName, raco::components::TracePlayer* tracePlayer);

protected:
	bool eventFilter(QObject* object, QEvent* event) override;

private:
	void configLayout();
	void configCtrls();
	void addWidgets();
	void connectCtrls();
	void enableCtrls(uint flags);
	void updateCtrls(int frameIndex);
	void loadClicked();
	void parseTraceFile();
	void editClicked();
	void speedChanged();
	void playClicked();
	void pauseClicked();
	void stopClicked();
	void stepBackwardClicked();
	void stepForwardClicked();
	void sliderMoved(int newSliderPos);
	void jumpToChanged();
	void clearLog();
	void reportLog(const std::vector<std::string>& tracePlayerReport, core::ErrorLevel highestCriticality, bool widgetMsg = false);
	void logCleared();
	void stateChanged(raco::components::TracePlayer::PlayerState state);
	void loopClicked();

	raco::components::TracePlayer* tracePlayer_{nullptr};
	QString defaultDir_{};
	/// UI Controls
	QGridLayout* layout_{nullptr};
	QLineEdit* fileNameLineEdt_{nullptr};
	QPushButton* browseBtn_{nullptr};
	QPushButton* reloadBtn_{nullptr};
	QPushButton* editBtn_{nullptr};
	QPushButton* loopBtn_{nullptr};
	QDoubleSpinBox* speedSpin_{nullptr};
	QPushButton* playBtn_{nullptr};
	QPushButton* pauseBtn_{nullptr};
	QPushButton* stepBackwardBtn_{nullptr};
	QPushButton* stopBtn_{nullptr};
	QPushButton* stepForwardBtn_{nullptr};
	QSpinBox* stepSpin_{nullptr};
	QSlider* timelineSlider_{nullptr};
	QSpinBox* jumpToSpin_{nullptr};
	QLabel* statusIndicator_{nullptr};
	QTextEdit* logTxtEdt_{nullptr};
	QPushButton* clearLogBtn_{nullptr};
	std::size_t widgetLogLen_{0};
	std::size_t totalLogLen_{0};
	components::TraceFileChangeMonitor loadedTraceFileChangeMonitor_;
	components::TraceFileChangeMonitor::UniqueListener loadedTraceFileChangeListener_;
	bool jumpToInFocus_{false};
};

}  // namespace raco::common_widgets