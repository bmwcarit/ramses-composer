#include "time_axis/TimeAxisMainWindow.h"

#include <QLineEdit>
#include <QActionGroup>
#include "style/Icons.h"
#include "core/EngineInterface.h"
#include "core/Undo.h"
#include "visual_curve/VisualCurvePosManager.h"

using namespace raco::style;
namespace raco::time_axis{
TimeAxisMainWindow::TimeAxisMainWindow(raco::components::SDataChangeDispatcher dispatcher,
                                       raco::core::CommandInterface* commandInterface,
                                       QWidget* parent) :
    QWidget(parent),
    commandInterface_(commandInterface) {

    keyFrameMgr_ = new KeyFrameManager();
    hTitleLayout = new QHBoxLayout(this);
    vBoxLayout_ = new QVBoxLayout(this);
    hBoxLayout = new QHBoxLayout(this);

    stackedWidget_ = new QStackedWidget(this);
    timeAxisScrollArea_ = new TimeAxisScrollArea(this);
    visualCurveScrollArea_ = new VisualCurveScrollArea(this);
    stackedWidget_->addWidget(timeAxisScrollArea_);
    stackedWidget_->addWidget(visualCurveScrollArea_);

    timeAxisWidget_ = new TimeAxisWidget(timeAxisScrollArea_->viewport(), commandInterface, keyFrameMgr_);
    connect(timeAxisScrollArea_, &TimeAxisScrollArea::viewportRectChanged, timeAxisWidget_, &TimeAxisWidget::setViewportRect);
    connect(timeAxisWidget_, &TimeAxisWidget::AnimationStop, this, &TimeAxisMainWindow::startOrStopAnimation);
    connect(timeAxisWidget_, &TimeAxisWidget::switchCurveType, this, &TimeAxisMainWindow::slotSwitchCurveWidget);

    visualCurveWidget_ = new VisualCurveWidget(visualCurveScrollArea_->viewport(), commandInterface);
    connect(visualCurveScrollArea_, &VisualCurveScrollArea::viewportRectChanged, visualCurveWidget_, &VisualCurveWidget::setViewportRect);
    connect(visualCurveWidget_, &VisualCurveWidget::AnimationStop, this, &TimeAxisMainWindow::startOrStopAnimation);
    connect(visualCurveWidget_, &VisualCurveWidget::sigSwitchCurveType, this, &TimeAxisMainWindow::slotSwitchCurveWidget);
    connect(visualCurveWidget_, &VisualCurveWidget::sigPressKey, this, &TimeAxisMainWindow::slotPressKey);

    timeAxisScrollArea_->setCenterWidget(timeAxisWidget_);
    visualCurveScrollArea_->setCenterWidget(visualCurveWidget_);

    visualCurveInfoWidget_ = new VisualCurveInfoWidget(this);
    visualCurveNodeTreeView_ = new VisualCurveNodeTreeView(this);

    initTitle(this);
    initTree(this);
    initAnimationMenu();
    hBoxLayout->addWidget(editorView_);
    hBoxLayout->addWidget(visualCurveNodeTreeView_);
    hBoxLayout->addWidget(stackedWidget_);
    hBoxLayout->addWidget(visualCurveInfoWidget_);
    hBoxLayout->setStretchFactor(editorView_, 2);
    hBoxLayout->setStretchFactor(visualCurveNodeTreeView_, 2);
    hBoxLayout->setStretchFactor(stackedWidget_, 10);
    hBoxLayout->setStretchFactor(visualCurveInfoWidget_, 1);

    vBoxLayout_->addWidget(titleWidget_);
    vBoxLayout_->addLayout(hBoxLayout);
    vBoxLayout_->setStretchFactor(titleWidget_, 1);
    vBoxLayout_->setStretchFactor(hBoxLayout, 7);
    this->setLayout(vBoxLayout_);

    slotSwitchCurveWidget();

    connect(&signalProxy::GetInstance(), &signalProxy::sigRepaintTimeAxis_From_NodeUI, this, &TimeAxisMainWindow::slotRefreshTimeAxis);
    connect(&signalProxy::GetInstance(), &signalProxy::sigRepaintTimeAixs_From_CurveUI, this, &TimeAxisMainWindow::slotRefreshTimeAxis);
    connect(&signalProxy::GetInstance(), &signalProxy::sigInsertKeyFrame_From_NodeUI, this, &TimeAxisMainWindow::slotCreateKeyFrame);
    connect(&signalProxy::GetInstance(), &signalProxy::sigUpdateAnimation_From_AnimationUI, this, &TimeAxisMainWindow::slotUpdateAnimation);
    connect(&signalProxy::GetInstance(), &signalProxy::sigUpdateAnimationKey_From_AnimationUI, this, &TimeAxisMainWindow::slotUpdateAnimationKey);
    connect(&signalProxy::GetInstance(), &signalProxy::sigResetAllData_From_MainWindow, this, &TimeAxisMainWindow::slotResetAnimation);
    connect(&signalProxy::GetInstance(), &signalProxy::sigInitAnimationView, this, &TimeAxisMainWindow::slotInitAnimationMgr);
    connect(&signalProxy::GetInstance(), &signalProxy::sigInitCurveView, this, &TimeAxisMainWindow::slotInitCurves);
    connect(&signalProxy::GetInstance(), &signalProxy::sigSwitchCurrentNode, this, &TimeAxisMainWindow::slotSwitchNode);
    connect(visualCurveWidget_, &VisualCurveWidget::sigUpdateSelKey, visualCurveInfoWidget_, &VisualCurveInfoWidget::slotUpdateSelKey);
    connect(visualCurveWidget_, &VisualCurveWidget::sigUpdateCursorX, visualCurveInfoWidget_, &VisualCurveInfoWidget::slotUpdateCursorX);
    connect(visualCurveWidget_, &VisualCurveWidget::sigDeleteCurve, visualCurveNodeTreeView_, &VisualCurveNodeTreeView::slotDeleteCurveFromVisualCurve);
    connect(visualCurveInfoWidget_, &VisualCurveInfoWidget::sigRefreshVisualCurve, visualCurveWidget_, &VisualCurveWidget::slotRefreshVisualCurve);
    connect(visualCurveInfoWidget_, &VisualCurveInfoWidget::sigRefreshCursorX, visualCurveWidget_, &VisualCurveWidget::slotRefreshCursorX);
    connect(visualCurveInfoWidget_, &VisualCurveInfoWidget::sigSwitchCurveType, visualCurveWidget_, &VisualCurveWidget::slotSwitchCurveType);
    connect(visualCurveNodeTreeView_, &VisualCurveNodeTreeView::sigRefreshVisualCurve, visualCurveWidget_, &VisualCurveWidget::slotRefreshVisualCurve);
    connect(visualCurveNodeTreeView_, &VisualCurveNodeTreeView::sigSwitchVisualCurveInfoWidget, this, &TimeAxisMainWindow::slotSwitchVisualCurveInfoWidget);
}

void TimeAxisMainWindow::startOrStopAnimation() {
    if (animationStarted_) {
        startBtn_->setFlat(true);
        startBtn_->setIcon(Icons::instance().animationStart);
//        startBtn_->setStyleSheet("QPushButton{background-image: url(:/animationStart);}");
        timeAxisWidget_->stopAnimation();
    } else {
        startBtn_->setFlat(true);
        startBtn_->setIcon(Icons::instance().animationStop);
//        startBtn_->setStyleSheet("QPushButton{background-image: url(:/animationStop);}");
        timeAxisWidget_->startAnimation();
    }
    animationStarted_ = !animationStarted_;
}

void TimeAxisMainWindow::slotTreeMenu(const QPoint &pos) {
    QMenu menu;

    QModelIndex curIndex = editorView_->indexAt(pos);
    QModelIndex index = curIndex.sibling(curIndex.row(),0);
	if (index.isValid()) {
		m_pLoad->setVisible(true);
		m_pCopy->setVisible(true);
		m_pDelete->setVisible(true);
		m_pProperty->setVisible(true);
    } else {
		m_pLoad->setVisible(false);
		m_pCopy->setVisible(false);
		m_pDelete->setVisible(false);
		m_pProperty->setVisible(false);
	}
	if (copyItemName_ == QString()) {
		pasteAction_->setVisible(false);
	} else {
		pasteAction_->setVisible(true);
	}
    m_Menu.exec(QCursor::pos());  //显示菜单
}

void TimeAxisMainWindow::slotLoad() {
    bool loadAction{true};
    QMessageBox::StandardButton resBtn = QMessageBox::question(this, "Ramses Composer",
        tr("Load This Animation?\n"),
        QMessageBox::Cancel | QMessageBox::No | QMessageBox::Yes,
        QMessageBox::Yes);
    loadAction = resBtn != QMessageBox::Cancel;
    if (resBtn == QMessageBox::Yes) {
        loadOperation();
    }
}

void TimeAxisMainWindow::slotCopy() {
	copyItemName_ = curItemName_+ "_cp";
}

void TimeAxisMainWindow::slotPaste() {
    if (model_ == nullptr) {
        return;
    }
    QStandardItem* item = new QStandardItem(copyItemName_);
	itemMap_.insert(copyItemName_, item);
    model_->appendRow(item);

    animationDataManager::GetInstance().InsertAmimation(copyItemName_.toStdString());
	copyItemName_ = QString();
}

void TimeAxisMainWindow::slotDelete() {
    if (curItemName_.compare(copyItemName_) == 0) {
        copyItemName_ = QString();
    }

    if (itemMap_.contains(curItemName_)) {
        QStandardItem* item = itemMap_.value(curItemName_);
        model_->removeRow(item->row());
        itemMap_.remove(curItemName_);
    }
    animationDataManager::GetInstance().DeleteAnimation(curItemName_.toStdString());
}

void TimeAxisMainWindow::slotProperty() {
    Q_EMIT signalProxy::GetInstance().sigResetAnimationProperty_From_AnimationLogic();
}

void TimeAxisMainWindow::slotCreateNew() {
    if (model_ == nullptr) {
        return;
    }

    QString strDefault = "DefaultAnimation" + QString::number(UUID_);
	if (animationDataManager::GetInstance().IsHaveAnimation(strDefault.toStdString())){
        UUID_++;
        slotCreateNew();
        return;
    }

    QStandardItem* item = new QStandardItem(strDefault);
    itemMap_.insert(strDefault, item);
    model_->appendRow(item);
    UUID_++;

    animationDataManager::GetInstance().InsertAmimation(strDefault.toStdString());
}

void TimeAxisMainWindow::slotCurrentRowChanged(const QModelIndex &index) {
    editorView_->resizeColumnToContents(index.row());
    curItemName_ = model_->itemData(index).values()[0].toString();
}

void TimeAxisMainWindow::slotItemChanged(QStandardItem* item) {
    if (itemMap_.values().contains(item)) {
        QString oldKey = itemMap_.key(item);
        QString newKey = item->data(Qt::DisplayRole).toString();
        if (itemMap_.contains(newKey)) {
            item->setText(oldKey);
            return;
        }
        itemMap_.remove(oldKey);
        itemMap_.insert(newKey, item);

        animationDataManager::GetInstance().ModifyAnimation(oldKey.toStdString(), newKey.toStdString());
        if (animationDataManager::GetInstance().GetActiveAnimation().compare(oldKey.toStdString())) {
            animationDataManager::GetInstance().SetActiveAnimation(newKey.toStdString());
        }
        Q_EMIT signalProxy::GetInstance().sigResetAnimationProperty_From_AnimationLogic();
    }
}

void TimeAxisMainWindow::slotUpdateAnimation() {
    timeAxisWidget_->stopAnimation();
    lineBegin_->setText(QString::number(animationDataManager::GetInstance().getActiveAnimationData().GetStartTime()));
    lineEnd_->setText(QString::number(animationDataManager::GetInstance().getActiveAnimationData().GetEndTime()));
    timeAxisWidget_->update();
}

void TimeAxisMainWindow::slotUpdateAnimationKey(QString oldKey, QString newKey) {
    timeAxisWidget_->stopAnimation();
}

void TimeAxisMainWindow::slotResetAnimation() {
    animationDataManager::GetInstance().ClearAniamtion();

    for (auto it : itemMap_.toStdMap()) {
        delete it.second;
        it.second = nullptr;
    }
    itemMap_.clear();
    timeAxisWidget_->clearKeyFrames();
    editorView_->update();
}

void TimeAxisMainWindow::slotSwitchCurveWidget() {
    switch (curCurveType_) {
    case CURVE_TYPE_ENUM::TIME_AXIS: {
        startBtn_->hide();
        nextBtn_->hide();
        previousBtn_->hide();
        curCurveType_ = VISUAL_CURVE;
        stackedWidget_->setCurrentWidget(visualCurveScrollArea_);
        visualCurveWidget_->setFocus(Qt::MouseFocusReason);

        // right switch
        visualCurveNodeTreeView_->show();
        editorView_->hide();

        // left switch
        visualCurveInfoWidget_->show();
        visualCurveInfoWidget_->update();

        break;
    }
    case CURVE_TYPE_ENUM::VISUAL_CURVE: {
        startBtn_->show();
        nextBtn_->show();
        previousBtn_->show();
        curCurveType_ = TIME_AXIS;
        stackedWidget_->setCurrentWidget(timeAxisScrollArea_);
        timeAxisWidget_->setFocus(Qt::MouseFocusReason);

        // right switch
        visualCurveNodeTreeView_->hide();
        editorView_->show();

        // left switch
        visualCurveInfoWidget_->hide();
        timeAxisWidget_->refreshKeyFrameView();

        break;
    }
    }
}

void TimeAxisMainWindow::slotPressKey() {
    MOUSE_PRESS_ACTION pressType = VisualCurvePosManager::GetInstance().getPressAction();
    switch (pressType) {
    case MOUSE_PRESS_KEY:
    case MOUSE_PRESS_LEFT_WORKER_KEY:
    case MOUSE_PRESS_RIGHT_WORKER_KEY: {
        visualCurveInfoWidget_->setKeyWidgetVisible();
        visualCurveNodeTreeView_->switchCurSelCurve(VisualCurvePosManager::GetInstance().getCurrentPointInfo().first);
        break;
    }
    case MOUSE_PRESS_NONE: {
        visualCurveInfoWidget_->setCursorWidgetVisible();
        visualCurveNodeTreeView_->cancelSelCurve();
        break;
    }
    }
}

void TimeAxisMainWindow::slotSwitchVisualCurveInfoWidget() {
    visualCurveInfoWidget_->setKeyWidgetVisible();
}

void TimeAxisMainWindow::slotInitAnimationMgr() {
    if (model_) {
        for (const auto& it : animationDataManager::GetInstance().getAniamtionNameList()) {
            if (!itemMap_.contains(QString::fromStdString(it))) {
                QStandardItem* item = new QStandardItem(QString::fromStdString(it));
                itemMap_.insert(QString::fromStdString(it), item);
                model_->appendRow(item);
            }
        }
        curItemName_ = QString::fromStdString(animationDataManager::GetInstance().GetActiveAnimation());
        loadOperation();
    }
    timeAxisWidget_->refreshKeyFrameView();
}

void TimeAxisMainWindow::slotCreateKeyFrame(QString curve) {
    timeAxisWidget_->createKeyFrame();
    visualCurveWidget_->createKeyFrame(curve);
}

void TimeAxisMainWindow::slotRefreshTimeAxis() {
    timeAxisWidget_->refreshKeyFrameView();
//    visualCurveNodeTreeView_->initCurves();
}

void TimeAxisMainWindow::slotInitCurves() {
    visualCurveNodeTreeView_->initCurves();
    visualCurveWidget_->refreshKeyFrameView();

//    raco::core::UndoState undoState;
//    undoState.push(VisualCurvePosManager::GetInstance().convertDataStruct());
//    undoState.push(FolderDataManager::GetInstance().converFolderData());
//    undoState.push(CurveManager::GetInstance().convertCurveData());
//    commandInterface_->undoStack().push("Initial Animation", undoState);
}

void TimeAxisMainWindow::slotSwitchNode(core::ValueHandle &handle) {
    if (handle) {
        std::string node = handle[1].asString();
        keyFrameMgr_->setCurNodeName(QString::fromStdString(node));
    }
}

bool TimeAxisMainWindow::initTitle(QWidget* parent) {
    titleWidget_ = new QWidget(this);

    QWidget* spacerLeft = new QWidget(titleWidget_);
    spacerLeft->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    startBtn_ = new QPushButton(titleWidget_);
    startBtn_->setFlat(true);
    startBtn_->setIcon(Icons::instance().animationStart);
    connect(startBtn_, &QPushButton::clicked, this, &TimeAxisMainWindow::startOrStopAnimation);
    previousBtn_ = new QPushButton(titleWidget_);
    previousBtn_->setFlat(true);
    previousBtn_->setIcon(Icons::instance().animationPrevious);
    connect(previousBtn_, &QPushButton::clicked, timeAxisWidget_, &TimeAxisWidget::setCurFrameToBegin);

    nextBtn_ = new QPushButton(titleWidget_);
    nextBtn_->setFlat(true);
    nextBtn_->setIcon(Icons::instance().animationNext);
    connect(nextBtn_, &QPushButton::clicked, timeAxisWidget_, &TimeAxisWidget::setCurFrameToEnd);

    QWidget* spacerRight = new QWidget(titleWidget_);
    spacerRight->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    lineBegin_ = new QLineEdit(titleWidget_);
    lineBegin_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    lineBegin_->resize(100, 20);
    lineBegin_->setText("0");

    lineEnd_ = new QLineEdit(titleWidget_);
    lineEnd_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    lineEnd_->resize(100, 20);
    lineEnd_->setText("200");
    connect(lineBegin_, &QLineEdit::textChanged, timeAxisWidget_, &TimeAxisWidget::setStartFrame);
	connect(lineEnd_, &QLineEdit::textChanged, timeAxisWidget_, &TimeAxisWidget::setFinishFrame);

    hTitleLayout->addWidget(spacerLeft);
    hTitleLayout->addWidget(previousBtn_);
    hTitleLayout->addWidget(startBtn_);
    hTitleLayout->addWidget(nextBtn_);

    hTitleLayout->addWidget(spacerRight);
    hTitleLayout->addWidget(lineBegin_);
    hTitleLayout->addWidget(lineEnd_);
    hTitleLayout->setSizeConstraint(QLayout::SetMaximumSize);
    hTitleLayout->setSpacing(0);
    titleWidget_->setLayout(hTitleLayout);
    return true;
}

bool TimeAxisMainWindow::initTree(QWidget *parent) {
    //test tree view.................
    editorView_ = new AnimationEditorView(parent);
    model_ = new QStandardItemModel(editorView_);

    editorView_->setModel(model_);

    editorView_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(editorView_, &AnimationEditorView::customContextMenuRequested, this, &TimeAxisMainWindow::slotTreeMenu);
    connect(editorView_, &QTreeView::pressed, this, &TimeAxisMainWindow::slotCurrentRowChanged);
    connect(model_, &QStandardItemModel::itemChanged, this, &TimeAxisMainWindow::slotItemChanged);

    return true;
}

bool TimeAxisMainWindow::initAnimationMenu() {
	m_pLoad = m_Menu.addAction(QStringLiteral("Load"), this, SLOT(slotLoad()));
	m_pCopy = m_Menu.addAction(QStringLiteral("Copy"), this, SLOT(slotCopy()));
	m_pDelete = m_Menu.addAction(QStringLiteral("Delete"), this, SLOT(slotDelete()));
	m_pProperty = m_Menu.addAction(QStringLiteral("Property"), this, SLOT(slotProperty()));
	pasteAction_ = new QAction(QStringLiteral("Paste"), this);
	connect(pasteAction_, &QAction::triggered, this, &TimeAxisMainWindow::slotPaste);
	m_Menu.addAction(pasteAction_);
	m_Menu.addAction(QStringLiteral("Create new animation"), this, SLOT(slotCreateNew()));
	return true;
}

void TimeAxisMainWindow::loadOperation() {
	if (curItemName_.isEmpty()) {
		return;
    }
    keyFrameMgr_->setCurAnimation(curItemName_);
    timeAxisWidget_->stopAnimation();
    animationDataManager::GetInstance().SetActiveAnimation(curItemName_.toStdString());
    animationData data = animationDataManager::GetInstance().getAnimationData(curItemName_.toStdString());
    lineBegin_->setText(QString::number(data.GetStartTime()));
    lineEnd_->setText(QString::number(data.GetEndTime()));
    timeAxisWidget_->update();

    Q_EMIT signalProxy::GetInstance().sigUpdateActiveAnimation_From_AnimationLogic(curItemName_);
    Q_EMIT signalProxy::GetInstance().sigUpdateKeyFram_From_AnimationLogic(timeAxisWidget_->getCurrentKeyFrame());

    for (const auto& it : itemMap_.toStdMap()) {
        QStandardItem* item = it.second;
        item->setForeground(QBrush(Qt::white));
        if (it.first.compare(curItemName_) == 0) {
            item->setForeground(QBrush(Qt::red));
        }
    }
}
}
