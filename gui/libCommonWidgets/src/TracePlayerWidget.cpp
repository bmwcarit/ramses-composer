/*
 * SPDX-License-Identifier: MPL-2.0
 *
 * This file is part of Ramses Composer
 * (see https://github.com/bmwcarit/ramses-composer).
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License, v. 2.0.
 * If a copy of the MPL was not distributed with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#include "common_widgets/TracePlayerWidget.h"

#include "core/ErrorItem.h"
#include "core/PathManager.h"
#include "style/Icons.h"

#include <utility>

#include <QDesktopServices>
#include <QEvent>
#include <QFileDialog>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSizePolicy>
#include <QSlider>
#include <QSpinBox>
#include <QString>
#include <QTextEdit>

enum Ctrl {
	None = 0x000,
	All = 0xFFF,

	EditButton = 0x001,
	SpeedSpin = 0x002,
	StepSpin = 0x004,
	PlayButton = 0x008,
	StepBackwardButton = 0x010,
	StepForwardButton = 0x020,
	TimelineSlider = 0x100,
	JumpToSpin = 0x200,
	PauseButton = 0x040,
	StopButton = 0x80,
	ReloadButton = 0x400,
	LoopButton = 0x800
};

namespace {
constexpr double SPEED_MIN{0.1};
constexpr double SPEED_MAX{10.0};
constexpr double SPEED_DEFAULT{1.0};
constexpr double SPEED_SINGLE_STEP{0.1};
constexpr int STEP_DEFAULT{1};
}  // namespace

namespace raco::common_widgets {

TracePlayerWidget::TracePlayerWidget(const QString& widgetName, raco::components::TracePlayer* tracePlayer)
	: tracePlayer_(tracePlayer),
	  defaultDir_(QString::fromStdString(raco::core::PathManager::getCachedPath(raco::core::PathManager::FolderTypeKeys::Project).string())) {
	tracePlayer_->setCallbacks(
		[this](raco::components::TracePlayer::PlayerState s) { stateChanged(s); },
		[this](int index) { updateCtrls(index); },
		[this](std::vector<std::string> log, core::ErrorLevel c) { reportLog(log, c); });
	configCtrls();
	configLayout();
	connectCtrls();
	this->setObjectName(widgetName);

	if (const auto tracePlayerState{tracePlayer_->getState()}; tracePlayerState != components::TracePlayer::PlayerState::Init) {
		if (const auto file{tracePlayer_->getFilePath()}; !file.empty()) {
			fileNameLineEdt_->setText(file.c_str());
		}
		stateChanged(tracePlayerState);
		timelineSlider_->setRange(0, tracePlayer_->getTraceLen() - 1);
	}
}

TracePlayerWidget::~TracePlayerWidget() {
	if (tracePlayer_->getState() != components::TracePlayer::PlayerState::Init) {
		tracePlayer_->clearLog();
		tracePlayer_->stop();
	}
}

void TracePlayerWidget::configCtrls() {
	fileNameLineEdt_ = new QLineEdit(this);
	fileNameLineEdt_->setPlaceholderText("trace file absolute path");

	browseBtn_ = new QPushButton(this);
	browseBtn_->setIcon(raco::style::Icons::instance().browse);
	browseBtn_->setToolTip("Browse");

	reloadBtn_ = new QPushButton(this);
	reloadBtn_->setIcon(raco::style::Icons::instance().refresh);
	reloadBtn_->setToolTip("Reload");

	editBtn_ = new QPushButton(this);
	editBtn_->setIcon(raco::style::Icons::instance().openInNew);
	editBtn_->setToolTip("Edit");

	loopBtn_ = new QPushButton(this);
	loopBtn_->setIcon(raco::style::Icons::instance().loopingInactive);
	loopBtn_->setToolTip("Loop playback");

	statusIndicator_ = new QLabel(this);
	QSizePolicy sp_retain{statusIndicator_->sizePolicy()};
	sp_retain.setRetainSizeWhenHidden(true);
	statusIndicator_->setSizePolicy(sp_retain);
	statusIndicator_->hide();

	logTxtEdt_ = new QTextEdit(this);
	logTxtEdt_->setReadOnly(true);
	logTxtEdt_->setMinimumHeight(50);

	clearLogBtn_ = new QPushButton("Clear", logTxtEdt_);

	speedSpin_ = new QDoubleSpinBox(this);
	speedSpin_->setPrefix("Speed x ");
	speedSpin_->setDecimals(1);
	speedSpin_->setRange(SPEED_MIN, SPEED_MAX);
	speedSpin_->setSingleStep(SPEED_SINGLE_STEP);
	speedSpin_->setValue(SPEED_DEFAULT);
	speedSpin_->setToolTip(QString::number(SPEED_MIN) + " ... " + QString::number(SPEED_MAX) +
						   " (" + QString::number(SPEED_SINGLE_STEP) + ")");

	playBtn_ = new QPushButton(this);
	playBtn_->setIcon(raco::style::Icons::instance().playInactive);
	playBtn_->setToolTip("Play");

	pauseBtn_ = new QPushButton(this);
	pauseBtn_->setIcon(raco::style::Icons::instance().pauseInactive);
	pauseBtn_->setToolTip("Pause");

	stopBtn_ = new QPushButton(this);
	stopBtn_->setIcon(raco::style::Icons::instance().stopInactive);
	stopBtn_->setToolTip("Stop");

	stepForwardBtn_ = new QPushButton(this);
	stepForwardBtn_->setIcon(raco::style::Icons::instance().skipNext);
	stepForwardBtn_->setToolTip("Step Forward");

	stepBackwardBtn_ = new QPushButton(this);
	stepBackwardBtn_->setIcon(raco::style::Icons::instance().skipPrevious);
	stepBackwardBtn_->setToolTip("Step Backward");

	stepSpin_ = new QSpinBox(this);
	stepSpin_->setPrefix("Step size: ");
	stepSpin_->setValue(STEP_DEFAULT);

	timelineSlider_ = new QSlider(Qt::Orientation::Horizontal, this);
	timelineSlider_->setTickPosition(QSlider::TicksBelow);

	jumpToSpin_ = new QSpinBox(this);
	jumpToSpin_->setPrefix("Jump to: ");
	jumpToSpin_->installEventFilter(this);

	enableCtrls(Ctrl::None);
}

void TracePlayerWidget::configLayout() {
	layout_ = new QGridLayout(this);
	/// row#1
	layout_->addWidget(fileNameLineEdt_, 0, 0, 1, 4);
	QHBoxLayout* topRightCtrls = new QHBoxLayout();
	topRightCtrls->addWidget(browseBtn_);
	topRightCtrls->addWidget(editBtn_);
	topRightCtrls->addWidget(reloadBtn_);
	topRightCtrls->addWidget(statusIndicator_);
	layout_->addLayout(topRightCtrls, 0, 4);
	/// row#2
	layout_->addWidget(playBtn_, 1, 0);
	layout_->addWidget(pauseBtn_, 1, 1);
	layout_->addWidget(stopBtn_, 1, 2);
	layout_->addWidget(stepBackwardBtn_, 1, 3);
	layout_->addWidget(stepForwardBtn_, 1, 4);
	/// row#3
	layout_->addWidget(loopBtn_, 2, 0);
	layout_->addWidget(jumpToSpin_, 2, 1);
	layout_->addWidget(speedSpin_, 2, 2);
	layout_->addWidget(stepSpin_, 2, 3, 1, 2);
	/// row#4
	layout_->addWidget(timelineSlider_, 3, 0, 1, 5);
	/// row#5
	layout_->addWidget(logTxtEdt_, 4, 0, 1, 5);
	/// row#6
	layout_->addWidget(clearLogBtn_, 5, 4);
}

void TracePlayerWidget::connectCtrls() {
	QObject::connect(fileNameLineEdt_, &QLineEdit::returnPressed, this, &TracePlayerWidget::parseTraceFile);
	QObject::connect(browseBtn_, &QPushButton::clicked, this, &TracePlayerWidget::loadClicked);
	QObject::connect(editBtn_, &QPushButton::clicked, this, &TracePlayerWidget::editClicked);
	QObject::connect(speedSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &TracePlayerWidget::speedChanged);
	QObject::connect(playBtn_, &QPushButton::clicked, this, &TracePlayerWidget::playClicked);
	QObject::connect(pauseBtn_, &QPushButton::clicked, this, &TracePlayerWidget::pauseClicked);
	QObject::connect(stopBtn_, &QPushButton::clicked, this, &TracePlayerWidget::stopClicked);
	QObject::connect(stepBackwardBtn_, &QPushButton::clicked, this, &TracePlayerWidget::stepBackwardClicked);
	QObject::connect(stepForwardBtn_, &QPushButton::clicked, this, &TracePlayerWidget::stepForwardClicked);
	QObject::connect(timelineSlider_, &QSlider::sliderMoved, this, &TracePlayerWidget::sliderMoved);
	QObject::connect(jumpToSpin_, &QSpinBox::editingFinished, this, &TracePlayerWidget::jumpToChanged);
	QObject::connect(clearLogBtn_, &QPushButton::clicked, this, &TracePlayerWidget::clearLog);
	QObject::connect(reloadBtn_, &QPushButton::clicked, this, &TracePlayerWidget::parseTraceFile);
	QObject::connect(loopBtn_, &QPushButton::clicked, this, &TracePlayerWidget::loopClicked);
}

void TracePlayerWidget::enableCtrls(uint flags) {
	editBtn_->setEnabled(flags & Ctrl::EditButton);
	speedSpin_->setEnabled(flags & Ctrl::SpeedSpin);
	stepSpin_->setEnabled(flags & Ctrl::StepSpin);
	playBtn_->setEnabled(flags & Ctrl::PlayButton);
	pauseBtn_->setEnabled(flags & Ctrl::PauseButton);
	stopBtn_->setEnabled(flags & Ctrl::StopButton);
	stepBackwardBtn_->setEnabled(flags & Ctrl::StepBackwardButton);
	stepForwardBtn_->setEnabled(flags & Ctrl::StepForwardButton);
	timelineSlider_->setEnabled(flags & Ctrl::TimelineSlider);
	jumpToSpin_->setEnabled(flags & Ctrl::JumpToSpin);
	reloadBtn_->setEnabled(flags & Ctrl::ReloadButton);
	loopBtn_->setEnabled(flags & Ctrl::LoopButton);
}

void TracePlayerWidget::loadClicked() {
	const QString qFileName{QFileDialog::getOpenFileName(this, "Load trace file", defaultDir_, "Ramses Composer Trace (*.rctrace);;Any files (*)")};

	if (!qFileName.isValidUtf16()) {
		reportLog({"Invalid trace file name! (fileName: " + qFileName.toStdString() + " )"}, core::ErrorLevel::ERROR, true);
	} else if (qFileName.size() > 0) {
		fileNameLineEdt_->setText(qFileName);
		parseTraceFile();
		defaultDir_ = QFileInfo(qFileName).absoluteDir().absolutePath();
	} else {
		/* do nothing, user canceled trace loading */
	}

	browseBtn_->setBackgroundRole(QPalette::Highlight);
}

void TracePlayerWidget::parseTraceFile() {
	if (const auto filename{fileNameLineEdt_->text().toStdString()}; tracePlayer_->loadTrace(filename)) {
		const auto traceLen{tracePlayer_->getTraceLen()};
		const auto lastFrameIndex{traceLen - 1};
		stepSpin_->setRange(1, lastFrameIndex);
		timelineSlider_->setRange(0, lastFrameIndex);
		jumpToSpin_->setRange(0, lastFrameIndex);
		timelineSlider_->setToolTip(QString::number(traceLen) + " frames");
		jumpToSpin_->setToolTip(QString::number(0) + " ... " + QString::number(lastFrameIndex));
		reloadBtn_->setIcon(raco::style::Icons::instance().refresh);
		loadedTraceFileChangeListener_ = loadedTraceFileChangeMonitor_.registerFileChangedHandler(
			filename,
			[this]() {
			reportLog({"Loaded trace file has been modified! Press Reload to reparse file."}, core::ErrorLevel::WARNING);
			reloadBtn_->setIcon(raco::style::Icons::instance().refreshNeeded); });
	} else {
		/* do nothing */
	}
}

void TracePlayerWidget::editClicked() {
	if (const auto qFilename{fileNameLineEdt_->text()}; !QDesktopServices::openUrl(QUrl::fromLocalFile(qFilename))) {
		reportLog({"Unable to open trace file!  (fileName: " + qFilename.toStdString() + " )"}, core::ErrorLevel::ERROR);
	}
	editBtn_->setBackgroundRole(QPalette::Highlight);
}

void TracePlayerWidget::speedChanged() {
	if (tracePlayer_) {
		tracePlayer_->setSpeed(speedSpin_->value());
		speedSpin_->setBackgroundRole(QPalette::Highlight);
	}
}

void TracePlayerWidget::playClicked() {
	if (tracePlayer_) {
		tracePlayer_->play();
		playBtn_->setBackgroundRole(QPalette::Highlight);
	}
}

void TracePlayerWidget::pauseClicked() {
	if (tracePlayer_) {
		tracePlayer_->pause();
		pauseBtn_->setBackgroundRole(QPalette::Highlight);
	}
}

void TracePlayerWidget::stopClicked() {
	if (tracePlayer_) {
		tracePlayer_->stop();
		stopBtn_->setBackgroundRole(QPalette::Highlight);
	}
}

void TracePlayerWidget::stepBackwardClicked() {
	if (tracePlayer_) {
		tracePlayer_->step(-stepSpin_->value());
		stepBackwardBtn_->setBackgroundRole(QPalette::Highlight);
	}
}

void TracePlayerWidget::stepForwardClicked() {
	if (tracePlayer_) {
		tracePlayer_->step(stepSpin_->value());
		stepForwardBtn_->setBackgroundRole(QPalette::Highlight);
	}
}

void TracePlayerWidget::sliderMoved(int newSliderPos) {
	if (tracePlayer_) {
		tracePlayer_->jumpTo(newSliderPos);
		timelineSlider_->setBackgroundRole(QPalette::Highlight);
	}
}

void TracePlayerWidget::jumpToChanged() {
	if (tracePlayer_) {
		if (jumpToInFocus_) {
			tracePlayer_->jumpTo(jumpToSpin_->value());
			jumpToSpin_->setBackgroundRole(QPalette::Highlight);
		}
	}
}

void TracePlayerWidget::updateCtrls(int frameIndex) {
	if (timelineSlider_->value() != frameIndex) {
		timelineSlider_->setValue(frameIndex);
	}
	if (jumpToSpin_->value() != frameIndex && !jumpToInFocus_) {
		jumpToSpin_->setValue(frameIndex);
	}
}

void TracePlayerWidget::reportLog(const std::vector<std::string>& tracePlayerReport, core::ErrorLevel highestCriticality, bool widgetMsg) {
	if (widgetMsg) {
		++widgetLogLen_;
		totalLogLen_ += widgetLogLen_;
	} else {
		totalLogLen_ = tracePlayerReport.size() + widgetLogLen_;
		logTxtEdt_->clear();
	}
	if (totalLogLen_ == 0) {
		logCleared();
		return;
	}

	statusIndicator_->setToolTip(QString::number(totalLogLen_) + " active messages");

	for (auto itr{tracePlayerReport.crbegin()}; itr != tracePlayerReport.rend(); ++itr) {
		logTxtEdt_->insertPlainText(QString::fromStdString(*itr + '\n'));
	}

	switch (highestCriticality) {
		case core::ErrorLevel::NONE:
			logCleared();
			break;
		case core::ErrorLevel::INFORMATION:
			statusIndicator_->setPixmap(raco::style::Icons::instance().info.pixmap(reloadBtn_->iconSize()));
			break;
		case core::ErrorLevel::WARNING:
			statusIndicator_->setPixmap(raco::style::Icons::instance().warning.pixmap(reloadBtn_->iconSize()));
			statusIndicator_->show();
			break;
		case core::ErrorLevel::ERROR:
			statusIndicator_->setPixmap(raco::style::Icons::instance().error.pixmap(reloadBtn_->iconSize()));
			statusIndicator_->show();
			break;
		default:
			statusIndicator_->hide();
			break;
	}
}

void TracePlayerWidget::logCleared() {
	logTxtEdt_->clear();
	totalLogLen_ = 0;
	widgetLogLen_ = 0;
	statusIndicator_->setToolTip("");
	statusIndicator_->hide();
}

void TracePlayerWidget::clearLog() {
	if (tracePlayer_) {
		tracePlayer_->clearLog();
		if (tracePlayer_->getState() == components::TracePlayer::PlayerState::Faulty) {
			tracePlayer_->stop();
		}
	}
}

void TracePlayerWidget::stateChanged(raco::components::TracePlayer::PlayerState state) {
	enableCtrls(Ctrl::All);
	playBtn_->setIcon(raco::style::Icons::instance().playInactive);
	pauseBtn_->setIcon(raco::style::Icons::instance().pauseInactive);
	stopBtn_->setIcon(raco::style::Icons::instance().stopInactive);
	switch (state) {
		case raco::components::TracePlayer::PlayerState::Init:
			break;
		case raco::components::TracePlayer::PlayerState::Faulty:
			enableCtrls(Ctrl::EditButton);
			break;
		case raco::components::TracePlayer::PlayerState::Playing:
			playBtn_->setIcon(raco::style::Icons::instance().playActive);
			break;
		case raco::components::TracePlayer::PlayerState::Paused:
			pauseBtn_->setIcon(raco::style::Icons::instance().pauseActive);
			break;
		case raco::components::TracePlayer::PlayerState::Stopped:
			stopBtn_->setIcon(raco::style::Icons::instance().stopActive);
			updateCtrls(0);
			break;
		default:
			reportLog({"Undefined playback state!"}, core::ErrorLevel::ERROR);
			break;
	}
}

void TracePlayerWidget::loopClicked() {
	if (tracePlayer_) {
		tracePlayer_->toggleLooping();
		if (tracePlayer_->getLoopingStatus()) {
			loopBtn_->setIcon(raco::style::Icons::instance().loopingActive);
		} else {
			loopBtn_->setIcon(raco::style::Icons::instance().loopingInactive);
		}
	}
}

bool TracePlayerWidget::eventFilter(QObject* obj, QEvent* event) {
	if (obj == jumpToSpin_) {
		if (event->type() == event->FocusIn) {
			jumpToSpin_->clear();
			jumpToInFocus_ = true;
		} else if (event->type() == event->FocusOut) {
			jumpToSpin_->setValue(jumpToSpin_->value());
			jumpToInFocus_ = false;
		}
		return false;
	}
	// standard event processing
	return QObject::eventFilter(obj, event);
}

}  // namespace raco::common_widgets