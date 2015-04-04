/**
 *  yasli - Serialization Library.
 *  Copyright (C) 2007-2013 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

#include "yasli/Pointers.h"
#include "yasli/Archive.h"
#include "yasli/BinArchive.h"
#include "QPropertyTree.h"
#include "frontend-qt/QUIFacade.h"
#include "PropertyDrawContext.h"
#include "Serialization.h"
#include "PropertyTreeModel.h"

#include "yasli/ClassFactory.h"

#include "PropertyOArchive.h"
#include "PropertyIArchive.h"
#include "Unicode.h"

#include <QRect>
#include <QTimer>
#include <QMimeData>
#include <QMenu>
#include <QMouseEvent>
#include <QScrollBar>
#include <QLineEdit>
#include <QPainter>
#include <QElapsedTimer>
#include <QStyle>
#include "PropertyTreeMenuHandler.h"

#include "MathUtils.h"

// only for clipboard:
#include <QClipboard>
#include <QApplication>
#include "PropertyRowPointer.h"
#include "PropertyRowContainer.h"
// ^^^
#include "PropertyRowObject.h"
#include "frontend-qt/QDrawContext.h"

using yasli::Serializers;

void PropertyTreeMenuHandler::onMenuFilter()
{
	((QPropertyTree*)tree)->startFilter("");
}

void PropertyTreeMenuHandler::onMenuFilterByName()
{
	((QPropertyTree*)tree)->startFilter(filterName.c_str());
}

void PropertyTreeMenuHandler::onMenuFilterByValue()
{
	((QPropertyTree*)tree)->startFilter(filterValue.c_str());
}

void PropertyTreeMenuHandler::onMenuFilterByType()
{
	((QPropertyTree*)tree)->startFilter(filterType.c_str());
}

void PropertyTreeMenuHandler::onMenuUndo()
{
	tree->model()->undo();
}

static QMimeData* propertyRowToMimeData(PropertyRow* row, ConstStringList* constStrings)
{
	PropertyRow::setConstStrings(constStrings);
	SharedPtr<PropertyRow> clonedRow(row->clone(constStrings));
	yasli::BinOArchive oa;
	PropertyRow::setConstStrings(constStrings);
	if (!oa(clonedRow, "row", "Row")) {
		PropertyRow::setConstStrings(0);
        return 0;
	}
	PropertyRow::setConstStrings(0);

	QByteArray byteArray(oa.buffer(), oa.length());
	QMimeData* mime = new QMimeData;
	mime->setData("binary/qpropertytree", byteArray);
	return mime;
}

static bool smartPaste(PropertyRow* dest, SharedPtr<PropertyRow>& source, PropertyTreeModel* model, bool onlyCheck)
{
	bool result = false;
	// content of the pulled container has a priority over the node itself
	PropertyRowContainer* destPulledContainer = static_cast<PropertyRowContainer*>(dest->pulledContainer());
	if((destPulledContainer && strcmp(destPulledContainer->elementTypeName(), source->typeName()) == 0)) {
		PropertyRow* elementRow = model->defaultType(destPulledContainer->elementTypeName());
		YASLI_ESCAPE(elementRow, return false);
		if(strcmp(elementRow->typeName(), source->typeName()) == 0){
			result = true;
			if(!onlyCheck){
				PropertyRow* dest = elementRow;
				if(dest->isPointer() && !source->isPointer()){
					PropertyRowPointer* d = static_cast<PropertyRowPointer*>(dest);
					SharedPtr<PropertyRowPointer> newSourceRoot = static_cast<PropertyRowPointer*>(d->clone(model->constStrings()).get());
					source->swapChildren(newSourceRoot);
					source = newSourceRoot;
				}
				destPulledContainer->add(source.get());
			}
		}
	}
	else if((source->isContainer() && dest->isContainer() &&
			 strcmp(static_cast<PropertyRowContainer*>(source.get())->elementTypeName(),
					static_cast<PropertyRowContainer*>(dest)->elementTypeName()) == 0) ||
			(!source->isContainer() && !dest->isContainer() && strcmp(source->typeName(), dest->typeName()) == 0)){
		result = true;
		if(!onlyCheck){
			if(dest->isPointer() && !source->isPointer()){
				PropertyRowPointer* d = static_cast<PropertyRowPointer*>(dest);
				SharedPtr<PropertyRowPointer> newSourceRoot = static_cast<PropertyRowPointer*>(d->clone(model->constStrings()).get());
				source->swapChildren(newSourceRoot);
				source = newSourceRoot;
			}
			const char* name = dest->name();
			const char* nameAlt = dest->label();
			source->setName(name);
			source->setLabel(nameAlt);
			if(dest->parent())
				dest->parent()->replaceAndPreserveState(dest, source, false);
			else{
				dest->clear();
				dest->swapChildren(source);
			}
			source->setLabelChanged();
		}
	}
	else if(dest->isContainer()){
		if(model){
			PropertyRowContainer* container = static_cast<PropertyRowContainer*>(dest);
			PropertyRow* elementRow = model->defaultType(container->elementTypeName());
			YASLI_ESCAPE(elementRow, return false);
			if(strcmp(elementRow->typeName(), source->typeName()) == 0){
				result = true;
				if(!onlyCheck){
					PropertyRow* dest = elementRow;
					if(dest->isPointer() && !source->isPointer()){
						PropertyRowPointer* d = static_cast<PropertyRowPointer*>(dest);
						SharedPtr<PropertyRowPointer> newSourceRoot = static_cast<PropertyRowPointer*>(d->clone(model->constStrings()).get());
						source->swapChildren(newSourceRoot);
						source = newSourceRoot;
					}

					container->add(source.get());
				}
			}
			container->setLabelChanged();
		}
	}
	
	return result;
}

static bool propertyRowFromMimeData(SharedPtr<PropertyRow>& row, const QMimeData* mimeData, ConstStringList* constStrings)
{
	PropertyRow::setConstStrings(constStrings);
	QStringList formats = mimeData->formats();
	QByteArray array = mimeData->data("binary/qpropertytree");
	if (array.isEmpty())
		return 0;
	yasli::BinIArchive ia;
	if (!ia.open(array.data(), array.size()))
		return 0;

	if (!ia(row, "row", "Row"))
		return false;

	PropertyRow::setConstStrings(0);
	return true;

}

bool propertyRowFromClipboard(SharedPtr<PropertyRow>& row, ConstStringList* constStrings)
{
	const QMimeData* mime = QApplication::clipboard()->mimeData();
	if (!mime)
		return false;
	return propertyRowFromMimeData(row, mime, constStrings);
}

void PropertyTreeMenuHandler::onMenuCopy()
{
	QMimeData* mime = propertyRowToMimeData(row, tree->model()->constStrings());
	if (mime)
		QApplication::clipboard()->setMimeData(mime);
}

void PropertyTreeMenuHandler::onMenuPaste()
{
	if(!tree->canBePasted(row))
		return;
	PropertyRow* parent = row->parent();

	tree->model()->rowAboutToBeChanged(row);

	SharedPtr<PropertyRow> source;
	if (!propertyRowFromClipboard(source, tree->model()->constStrings()))
		return;

	if (!smartPaste(row, source, tree->model(), false))
		return;
	
	tree->model()->rowChanged(parent ? parent : tree->model()->root());
}

class FilterEntry : public QLineEdit
{
public:
	FilterEntry(QPropertyTree* tree)
    : QLineEdit(tree)
    , tree_(tree)
	{
	}
protected:

    void keyPressEvent(QKeyEvent * ev)
    {
        if (ev->key() == Qt::Key_Escape || ev->key() == Qt::Key_Return)
        {
            ev->accept();
            tree_->setFocus();
            tree_->keyPressEvent(ev);
        }

        if (ev->key() == Qt::Key_Backspace && text()[0] == '\0')
        {
            tree_->setFilterMode(false);
        }
        QLineEdit::keyPressEvent(ev);
    }
private:
	QPropertyTree* tree_;
};


// ---------------------------------------------------------------------------

DragWindow::DragWindow(QPropertyTree* tree)
: tree_(tree)
, offset_(0, 0)
{
	QWidget::setWindowFlags(Qt::ToolTip);
	QWidget::setWindowOpacity(192.0f/ 256.0f);
}

void DragWindow::set(QPropertyTree* tree, PropertyRow* row, const QRect& rowRect)
{
	QRect rect = tree->rect();
	rect.setTopLeft(tree->mapToGlobal(rect.topLeft()));

	offset_ = rect.topLeft();

	row_ = row;
	rect_ = rowRect;
}

void DragWindow::setWindowPos(bool visible)
{
	QWidget::move(rect_.left() + offset_.x() - 3,  rect_.top() + offset_.y() - 3 + tree_->area_.top());
	QWidget::resize(rect_.width() + 5, rect_.height() + 5);
}

void DragWindow::show()
{
	setWindowPos(true);
	QWidget::show();
}

void DragWindow::move(int deltaX, int deltaY)
{
	offset_ += Point(deltaX, deltaY);
	setWindowPos(isVisible());
}

void DragWindow::hide()
{
	setWindowPos(false);
	QWidget::hide();
}

struct DrawVisitor
{
	DrawVisitor(QPainter& painter, const QRect& area, int scrollOffset, bool selectionPass)
		: area_(area)
		, painter_(painter)
		, offset_(0)
		, scrollOffset_(scrollOffset)
		, lastParent_(0)
		, selectionPass_(selectionPass)
	{}

	ScanResult operator()(PropertyRow* row, PropertyTree* tree, int index)
	{
		if(row->visible(tree) && ((row->parent()->expanded() && !lastParent_) || row->pulledUp())){
			if(row->rect().top() > scrollOffset_ + area_.height())
				lastParent_ = row->parent();

			QDrawContext context((QPropertyTree*)tree, &painter_);
			if(row->rect().bottom() > scrollOffset_ && row->rect().width() > 0)
				row->drawRow(context, tree, index, selectionPass_);

			return SCAN_CHILDREN_SIBLINGS;
		}
		else
			return SCAN_SIBLINGS;
	}

protected:
	QPainter& painter_;
	QRect area_;
	int offset_;
	int scrollOffset_;
	PropertyRow* lastParent_;
	bool selectionPass_;
};

struct DrawRowVisitor
{
	DrawRowVisitor(QPainter& painter) : painter_(painter) {}

	ScanResult operator()(PropertyRow* row, PropertyTree* tree, int index)
	{
		if(row->pulledUp() && row->visible(tree)) {
			QDrawContext context((QPropertyTree*)tree, &painter_);
			row->drawRow(context, tree, index, true);
			row->drawRow(context, tree, index, false);
		}

		return SCAN_CHILDREN_SIBLINGS;
	}

protected:
	QPainter& painter_;
};


void DragWindow::drawRow(QPainter& p)
{
	QRect entireRowRect(0, 0, rect_.width() + 4, rect_.height() + 4);

	p.setBrush(tree_->palette().button());
	p.setPen(QPen(tree_->palette().color(QPalette::WindowText)));
	p.drawRect(entireRowRect);

	QPoint leftTop = QRect(row_->rect()).topLeft();
	int offsetX = -leftTop.x() - tree_->tabSize() + 3;
	int offsetY = -leftTop.y() + 3;
	p.translate(offsetX, offsetY);
	int rowIndex = 0;
	if (row_->parent())
		rowIndex = row_->parent()->childIndex(row_);
	QDrawContext context(tree_, &p);
	row_->drawRow(context, tree_, 0, true);
	row_->drawRow(context, tree_, 0, false);
	DrawRowVisitor visitor(p);
	row_->scanChildren(visitor, tree_);
	p.translate(-offsetX, -offsetY);
}

void DragWindow::paintEvent(QPaintEvent* ev)
{
	QPainter p(this);

	drawRow(p);
}

// ---------------------------------------------------------------------------

class QPropertyTree::DragController
{
public:
	DragController(QPropertyTree* tree)
	: tree_(tree)
	, captured_(false)
	, dragging_(false)
	, before_(false)
	, row_(0)
	, clickedRow_(0)
	, window_(tree)
	, hoveredRow_(0)
	, destinationRow_(0)
	{
	}

	void beginDrag(PropertyRow* clickedRow, PropertyRow* draggedRow, QPoint pt)
	{
		row_ = draggedRow;
		clickedRow_ = clickedRow;
		startPoint_ = pt;
		lastPoint_ = pt;
		captured_ = true;
		dragging_ = false;
	}

	bool dragOn(QPoint screenPoint)
	{
		if (dragging_)
			window_.move(screenPoint.x() - lastPoint_.x(), screenPoint.y() - lastPoint_.y());

		bool needCapture = false;
		if(!dragging_ && (startPoint_ - screenPoint).manhattanLength() >= 5)
			if(row_->canBeDragged()){
				needCapture = true;
				QRect rect = row_->rect();
				rect = QRect(rect.topLeft() - tree_->offset_ + QPoint(tree_->tabSize(), 0), 
							 rect.bottomRight() - tree_->offset_);

				window_.set(tree_, row_, rect);
				window_.move(screenPoint.x() - startPoint_.x(), screenPoint.y() - startPoint_.y());
				window_.show();
				dragging_ = true;
			}

		if(dragging_){
			QPoint point = tree_->mapFromGlobal(screenPoint);
			trackRow(point);
		}
		lastPoint_ = screenPoint;
		return needCapture;
	}

	void interrupt()
	{
		captured_ = false;
		dragging_ = false;
		row_ = 0;
		window_.hide();
	}

	void trackRow(QPoint pt)
	{
		hoveredRow_ = 0;
		destinationRow_ = 0;

		QPoint point = pt;
		PropertyRow* row = tree_->rowByPoint(point);
		if(!row || !row_)
			return;

		row = row->nonPulledParent();
		if(!row->parent() || row->isChildOf(row_) || row == row_)
			return;

		float pos = (point.y() - row->rect().top()) / float(row->rect().height());
		if(row_->canBeDroppedOn(row->parent(), row, tree_)){
			if(pos < 0.25f){
				destinationRow_ = row->parent();
				hoveredRow_ = row;
				before_ = true;
				return;
			}
			if(pos > 0.75f){
				destinationRow_ = row->parent();
				hoveredRow_ = row;
				before_ = false;
				return;
			}
		}
		if(row_->canBeDroppedOn(row, 0, tree_))
			hoveredRow_ = destinationRow_ = row;
	}

	void drawUnder(QPainter& painter)
	{
		if(dragging_ && destinationRow_ == hoveredRow_ && hoveredRow_){
			QRect rowRect = hoveredRow_->rect();
			rowRect.setLeft(rowRect.left() + tree_->tabSize());
			QBrush brush(true ? tree_->palette().highlight() : tree_->palette().shadow());
			QColor brushColor = brush.color();
			QColor borderColor(brushColor.alpha() / 4, brushColor.red(), brushColor.green(), brushColor.blue());
			fillRoundRectangle(painter, brush, rowRect, borderColor, 6);
		}
	}

	void drawOver(QPainter& painter)
	{
		if(!dragging_)
			return;

		QRect rowRect = row_->rect();

		if(destinationRow_ != hoveredRow_ && hoveredRow_){
			const int tickSize = 4;
			QRect hoveredRect = hoveredRow_->rect();
			hoveredRect.setLeft(hoveredRect.left() + tree_->tabSize());

			if(!before_){ // previous
				QRect rect(hoveredRect.left() - 1 , hoveredRect.bottom() - 1, hoveredRect.width(), 2);
				QRect rectLeft(hoveredRect.left() - 1 , hoveredRect.bottom() - tickSize, 2, tickSize * 2);
				QRect rectRight(hoveredRect.right() - 1 , hoveredRect.bottom() - tickSize, 2, tickSize * 2);
				painter.fillRect(rect, tree_->palette().highlight());
				painter.fillRect(rectLeft, tree_->palette().highlight());
				painter.fillRect(rectRight, tree_->palette().highlight());
			}
			else{ // next
				QRect rect(hoveredRect.left() - 1 , hoveredRect.top() - 1, hoveredRect.width(), 2);
				QRect rectLeft(hoveredRect.left() - 1 , hoveredRect.top() - tickSize, 2, tickSize * 2);
				QRect rectRight(hoveredRect.right() - 1 , hoveredRect.top() - tickSize, 2, tickSize * 2);
				painter.fillRect(rect, tree_->palette().highlight());
				painter.fillRect(rectLeft, tree_->palette().highlight());
				painter.fillRect(rectRight, tree_->palette().highlight());
			}
		}
	}

	bool drop(QPoint screenPoint)
	{
		bool rowLayoutChanged = false;
		PropertyTreeModel* model = tree_->model();
		if(row_ && hoveredRow_){
			YASLI_ASSERT(destinationRow_);
			clickedRow_->setSelected(false);
			row_->dropInto(destinationRow_, destinationRow_ == hoveredRow_ ? 0 : hoveredRow_, tree_, before_);
			rowLayoutChanged = true;
		}

		captured_ = false;
		dragging_ = false;
		row_ = 0;
		window_.hide();
		hoveredRow_ = 0;
		destinationRow_ = 0;
		return rowLayoutChanged;
	}

	bool captured() const{ return captured_; }
	bool dragging() const{ return dragging_; }
	PropertyRow* draggedRow() { return row_; }
protected:
	DragWindow window_;
	QPropertyTree* tree_;
	PropertyRow* row_;
	PropertyRow* clickedRow_;
	PropertyRow* hoveredRow_;
	PropertyRow* destinationRow_;
	QPoint startPoint_;
	QPoint lastPoint_;
	bool captured_;
	bool dragging_;
	bool before_;
};

// ---------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable: 4355) //  'this' : used in base member initializer list
QPropertyTree::QPropertyTree(QWidget* parent)
: QWidget(parent)
, PropertyTree(new QUIFacade(this))
, sizeHint_(180, 180)
, dragController_(new DragController(this))

, updateHeightsTime_(0)
, paintTime_(0)
{
	setFocusPolicy(Qt::WheelFocus);
	scrollBar_ = new QScrollBar(Qt::Vertical, this);
	connect(scrollBar_, SIGNAL(valueChanged(int)), this, SLOT(onScroll(int)));

    filterEntry_.reset(new FilterEntry(this));
    QObject::connect(filterEntry_.data(), SIGNAL(textChanged(const QString&)), this, SLOT(onFilterChanged(const QString&)));
    filterEntry_->hide();

	mouseStillTimer_ = new QTimer(this);
	mouseStillTimer_->setSingleShot(true);
	connect(mouseStillTimer_, SIGNAL(timeout()), this, SLOT(onMouseStillTimer()));

	boldFont_ = font();
	boldFont_.setBold(true);
}
#pragma warning(pop)

QPropertyTree::~QPropertyTree()
{
	clearMenuHandlers();
}

void QPropertyTree::interruptDrag()
{
	dragController_->interrupt();
}

void QPropertyTree::updateHeights()
{
	QElapsedTimer timer;
	timer.start();

	model()->root()->updateLabel(this, 0);

	QRect widgetRect = this->rect();

	int scrollBarW = style()->pixelMetric(QStyle::PM_ScrollBarExtent);
	int lb = compact_ ? 0 : 4;
	int rb = widgetRect.right() - lb - scrollBarW - 2;
	bool force = lb != leftBorder_ || rb != rightBorder_;
	leftBorder_ = lb;
	rightBorder_ = rb;
	model()->root()->calculateMinimalSize(this, leftBorder_, force, 0, 0);

	int padding = compact_ ? 4 : 6;

	int totalHeight = 0;
	model()->root()->adjustVerticalPosition(this, totalHeight);
	size_.setY(totalHeight);

	bool hasScrollBar = updateScrollBar();

	area_ = widgetRect.adjusted(2, 2, -2 - scrollBarW, -2);
	size_.setX(area_.width());

	if (filterMode_)
	{
		int filterAreaHeight = filterEntry_ ? filterEntry_->height() : 0;
		area_.setTop(area_.top() + filterAreaHeight + 2 + 2);
	}

	_arrangeChildren();

	update();
	updateHeightsTime_ = timer.elapsed();
}

bool QPropertyTree::updateScrollBar()
{
	int pageSize = rect().height();
	offset_.setX(max(0, min(offset_.x(), max(0, size_.x() - area_.right() - 1))));
	offset_.setY(max(0, min(offset_.y(), max(0, size_.y() - pageSize))));

	if (pageSize < size_.y())
	{
		scrollBar_->setRange(0, size_.y() - pageSize);
		scrollBar_->setSliderPosition(offset_.y());
		scrollBar_->setPageStep(pageSize);
		scrollBar_->show();
		scrollBar_->move(rect().right() - scrollBar_->width(), 0);
		scrollBar_->resize(scrollBar_->width(), height());
		return true;
	}
	else
	{
		scrollBar_->hide();
		return false;
	}
}

const QFont& QPropertyTree::boldFont() const
{
	return boldFont_;
}

void QPropertyTree::onScroll(int pos)
{
	offset_.setY(scrollBar_->sliderPosition());
	_arrangeChildren();
	repaint();
}

void QPropertyTree::serialize(Archive& ar)
{
	model()->serialize(ar, this);

	if(ar.isInput()){
		updateHeights();
		ensureVisible(model()->focusedRow());
		updateAttachedPropertyTree(false);
		updateHeights();
		signalSelected();
	}
}

bool QPropertyTree::canBePasted(PropertyRow* destination)
{
	SharedPtr<PropertyRow> source;
	if (!propertyRowFromClipboard(source, model_->constStrings()))
		return false;

	if (!smartPaste(destination, source, model(), true))
		return false;
	return true;
}

bool QPropertyTree::canBePasted(const char* destinationType)
{
	SharedPtr<PropertyRow> source;
	if (!propertyRowFromClipboard(source, model()->constStrings()))
		return false;

	bool result = strcmp(source->typeName(), destinationType) == 0;
	return result;
}

void QPropertyTree::onModelPushUndo(PropertyTreeOperator* op, bool* handled)
{
	signalPushUndo();
}

bool QPropertyTree::hasFocusOrInplaceHasFocus() const
{
	if (hasFocus())
		return true;

	QWidget* inplaceWidget = 0;
	if (widget_.get() && widget_->actualWidget())
		inplaceWidget = (QWidget*)widget_->actualWidget();

	if (inplaceWidget) {
		if (inplaceWidget->hasFocus())
			return true;
		if (inplaceWidget->isAncestorOf(QWidget::focusWidget()))
			return true;

	}

	return false;
}

void QPropertyTree::setFilterMode(bool inFilterMode)
{
    bool changed = filterMode_ != inFilterMode;
    filterMode_ = inFilterMode;
    
	if (filterMode_)
	{
        filterEntry_->show();
		filterEntry_->setFocus();
        filterEntry_->selectAll();
	}
    else
        filterEntry_->hide();

    if (changed)
    {
        onFilterChanged(QString());
    }
}

void QPropertyTree::startFilter(const char* filter)
{
	setFilterMode(true);
	filterEntry_->setText(filter);
    onFilterChanged(filter);
}

void QPropertyTree::_arrangeChildren()
{
	if(widget_.get()){
		PropertyRow* row = widgetRow_;
		if(row->visible(this)){
			QWidget* w = (QWidget*)widget_->actualWidget();
			YASLI_ASSERT(w);
			if(w){
				QRect rect = row->widgetRect(this);
				rect = QRect(rect.topLeft() - offset_ + area_.topLeft(), 
							 rect.bottomRight() - offset_ + area_.topLeft());
				w->move(rect.topLeft());
				w->resize(rect.size());
				if(!w->isVisible()){
					w->show();
					w->setFocus();
				}
			}
			else{
				//YASLI_ASSERT(w);
			}
		}
		else{
			widget_.reset();
		}
	}

	if (filterEntry_) {
		QSize size = rect().size();
		const int padding = 2;
		QRect pos(padding, padding, size.width() - padding * 2, filterEntry_->height());
		filterEntry_->move(pos.topLeft());
		filterEntry_->resize(pos.size() - QSize(scrollBar_ ? scrollBar_->width() : 0, 0));
	}
}

QPoint QPropertyTree::_toScreen(Point point) const
{
	QPoint pt ( point.x() - offset_.x() + area_.left(), 
				point.y() - offset_.y() + area_.top() );

	return mapToGlobal(pt);
}

void QPropertyTree::attachPropertyTree(PropertyTree* propertyTree) 
{ 
	if(attachedPropertyTree_)
		disconnect((QPropertyTree*)attachedPropertyTree_, SIGNAL(signalChanged()), this, SLOT(onAttachedTreeChanged()));
	PropertyTree::attachPropertyTree(propertyTree);
	connect((QPropertyTree*)attachedPropertyTree_, SIGNAL(signalChanged()), this, SLOT(onAttachedTreeChanged()));
}

struct FilterVisitor
{
	const QPropertyTree::RowFilter& filter_;

	FilterVisitor(const QPropertyTree::RowFilter& filter) 
    : filter_(filter)
    {
    }

	static void markChildrenAsBelonging(PropertyRow* row, bool belongs)
	{
		int count = int(row->count());
		for (int i = 0; i < count; ++i)
		{
			PropertyRow* child = row->childByIndex(i);
			child->setBelongsToFilteredRow(belongs);

			markChildrenAsBelonging(child, belongs);
		}
	}
	
	static bool hasMatchingChildren(PropertyRow* row)
	{
		int numChildren = (int)row->count();
		for (int i = 0; i < numChildren; ++i)
		{
			PropertyRow* child = row->childByIndex(i);
			if (!child)
				continue;
			if (child->matchFilter())
				return true;
			if (hasMatchingChildren(child))
				return true;
		}
		return false;
	}

	ScanResult operator()(PropertyRow* row, PropertyTree* _tree)
	{
		QPropertyTree* tree = (QPropertyTree*)_tree;
		const char* label = row->labelUndecorated();
		yasli::string value = row->valueAsString();

		bool matchFilter = filter_.match(label, filter_.NAME_VALUE, 0, 0) || filter_.match(value.c_str(), filter_.NAME_VALUE, 0, 0);
		if (matchFilter && filter_.typeRelevant(filter_.NAME))
			filter_.match(label, filter_.NAME, 0, 0);
		if (matchFilter && filter_.typeRelevant(filter_.VALUE))
			matchFilter = filter_.match(value.c_str(), filter_.VALUE, 0, 0);
		if (matchFilter && filter_.typeRelevant(filter_.TYPE))
			matchFilter = filter_.match(row->typeNameForFilter(tree), filter_.TYPE, 0, 0);						   
		
		int numChildren = int(row->count());
		if (matchFilter) {
			if (row->pulledBefore() || row->pulledUp()) {
				// treat pulled rows as part of parent
				PropertyRow* parent = row->parent();
				parent->setMatchFilter(true);
				markChildrenAsBelonging(parent, true);
				parent->setBelongsToFilteredRow(false);
			}
			else {
				markChildrenAsBelonging(row, true);
				row->setBelongsToFilteredRow(false);
				row->setLayoutChanged();
				row->setLabelChanged();
			}
		}
		else {
			bool belongs = hasMatchingChildren(row);
			row->setBelongsToFilteredRow(belongs);
			if (belongs) {
				tree->expandRow(row, true, false);
				for (int i = 0; i < numChildren; ++i) {
					PropertyRow* child = row->childByIndex(i);
					if (child->pulledUp())
						child->setBelongsToFilteredRow(true);
				}
			}
			else {
				row->_setExpanded(false);
				row->setLayoutChanged();
			}
		}

		row->setMatchFilter(matchFilter);
		return SCAN_CHILDREN_SIBLINGS;
	}

protected:
	yasli::string labelStart_;
};


void QPropertyTree::onFilterChanged(const QString& text)
{
	QByteArray arr = filterEntry_->text().toLocal8Bit();
	const char* filterStr = filterMode_ ? arr.data() : "";
	rowFilter_.parse(filterStr);
	FilterVisitor visitor(rowFilter_);
	model()->root()->scanChildrenBottomUp(visitor, this);
	updateHeights();
}

void QPropertyTree::drawFilteredString(QPainter& p, const wchar_t* text, RowFilter::Type type, const QFont* font, const QRect& rect, const QColor& textColor, bool pathEllipsis, bool center) const
{
	int textLen = (int)wcslen(text);

	if (textLen == 0)
		return;

	yasli::string textStr(fromWideChar(text));
#ifdef _MSC_VER
	// default qt 4.8 builds have 32-bit wchar_t
	QString str = QString::fromUtf16((const ushort*)text);
#else
	QString str = QString::fromWCharArray(text);
#endif
	QFontMetrics fm(*font);
	QRect textRect = rect;
	int alignment;
	if (center)
		alignment = Qt::AlignHCenter | Qt::AlignVCenter;
	else {
		if (pathEllipsis && textRect.width() < fm.width(str))
			alignment = Qt::AlignRight | Qt::AlignVCenter;
		else
			alignment = Qt::AlignLeft | Qt::AlignVCenter;
	}

	if (filterMode_) {
		size_t hiStart = 0;
		size_t hiEnd = 0;
		bool matched = rowFilter_.match(textStr.c_str(), type, &hiStart, &hiEnd) && hiStart != hiEnd;
		if (!matched && (type == RowFilter::NAME || type == RowFilter::VALUE))
			matched = rowFilter_.match(textStr.c_str(), RowFilter::NAME_VALUE, &hiStart, &hiEnd);
		if (matched && hiStart != hiEnd) {
			QRectF boxFull;
			QRectF boxStart;
			QRectF boxEnd;

			boxFull = fm.boundingRect(textRect, alignment, str);

			if (hiStart > 0)
				boxStart = fm.boundingRect(textRect, alignment, str.left(hiStart));
			else {
				boxStart = fm.boundingRect(textRect, alignment, str);
				boxStart.setWidth(0.0f);
			}
			boxEnd = fm.boundingRect(textRect, alignment, str.left(hiEnd));

			QColor highlightColor, highlightBorderColor;
			{
				highlightColor = palette().color(QPalette::Highlight);
				int h, s, v;
				highlightColor.getHsv(&h, &s, &v);
				h -= 175;
				if (h < 0)
					h += 360;
				highlightColor.setHsv(h, min(255, int(s * 1.33f)), v, 255);
				highlightBorderColor.setHsv(h, s * 0.5f, v, 255);
			}

			int left = int(boxFull.left() + boxStart.width()) - 1;
			int top = int(boxFull.top());
			int right = int(boxFull.left() + boxEnd.width());
			int bottom = int(boxFull.top() + boxEnd.height());
			QRect highlightRect(left, top, right - left, bottom - top);
			QBrush br(highlightColor);
			p.setBrush(br);
			p.setPen(highlightBorderColor);
			bool oldAntialiasing = p.renderHints().testFlag(QPainter::Antialiasing);
			p.setRenderHint(QPainter::Antialiasing, true);

			QRect intersectedHighlightRect = rect.intersected(highlightRect);
			p.drawRoundedRect(intersectedHighlightRect, 4.0, 4.0);
			p.setRenderHint(QPainter::Antialiasing, oldAntialiasing);
		}
	}

	QBrush textBrush(textColor);
	p.setBrush(textBrush);
	p.setPen(textColor);
	QFont previousFont = p.font();
	p.setFont(*font);
	p.drawText(textRect, alignment, str, 0);
	p.setFont(previousFont);
}

void QPropertyTree::_drawRowValue(QPainter& p, const wchar_t* text, const QFont* font, const QRect& rect, const QColor& textColor, bool pathEllipsis, bool center) const
{
	drawFilteredString(p, text, RowFilter::VALUE, font, rect, textColor, pathEllipsis, center);
}

QSize QPropertyTree::sizeHint() const
{
	return sizeHint_;
}

void QPropertyTree::paintEvent(QPaintEvent* ev)
{
	QElapsedTimer timer;
	timer.start();
	QPainter painter(this);
	QRect clientRect = this->rect();

	int clientWidth = clientRect.width();
	int clientHeight = clientRect.height();
	painter.fillRect(clientRect, palette().window());

	painter.translate(-offset_.x(), -offset_.y());

	if(dragController_->captured())
	 	dragController_->drawUnder(painter);

	painter.translate(area_.left(), area_.top());

    if (model()->root()) {
        DrawVisitor selectionOp(painter, area_, offset_.y(), true);
        model()->root()->scanChildren(selectionOp, this);

        DrawVisitor op(painter, area_, offset_.y(), false);
        model()->root()->scanChildren(op, this);
    }

	painter.translate(-area_.left(), -area_.top());
	painter.translate(offset_.x(), offset_.y());

	//painter.setClipRect(rect());

	if (size_.y() > clientHeight)
	{
 	  const int shadowHeight = 10;
		QColor color1(0, 0, 0, 0);
		QColor color2(0, 0, 0, 96);

		QRect upperRect(rect().left(), rect().top(), area_.width(), shadowHeight);
		QLinearGradient upperGradient(upperRect.left(), upperRect.top(), upperRect.left(), upperRect.bottom());
		upperGradient.setColorAt(0.0f, color2);
		upperGradient.setColorAt(1.0f, color1);
		QBrush upperBrush(upperGradient);
		painter.fillRect(upperRect, upperBrush);

		QRect lowerRect(rect().left(), rect().bottom() - shadowHeight / 2, rect().width(), shadowHeight / 2 + 1);
		QLinearGradient lowerGradient(lowerRect.left(), lowerRect.top(), lowerRect.left(), lowerRect.bottom());		
		lowerGradient.setColorAt(0.0f, color1);
		lowerGradient.setColorAt(1.0f, color2);
		QBrush lowerBrush(lowerGradient);
		painter.fillRect(lowerRect, lowerGradient);
	}
	
	if (dragController_->captured()) {
	 	painter.translate(-offset_);
	 	dragController_->drawOver(painter);
	 	painter.translate(offset_);
	}
	else{
	// 	if(model()->focusedRow() != 0 && model()->focusedRow()->isRoot() && tree_->hasFocus()){
	// 		clientRect.left += 2; clientRect.top += 2;
	// 		clientRect.right -= 2; clientRect.bottom -= 2;
	// 		DrawFocusRect(dc, &clientRect);
	// 	}
	}
	paintTime_ = timer.elapsed();
}

void QPropertyTree::moveEvent(QMoveEvent* ev)
{
	QWidget::moveEvent(ev);
}

void QPropertyTree::resizeEvent(QResizeEvent* ev)
{
	QWidget::resizeEvent(ev);

	updateHeights();
}

void QPropertyTree::mousePressEvent(QMouseEvent* ev)
{
	//QWidget::mousePressEvent(ev);
	setFocus(Qt::MouseFocusReason);

	if (ev->button() == Qt::LeftButton)
	{
		int x = ev->x();
		int y = ev->y();

		PropertyRow* row = rowByPoint(ev->pos());
		if(row && !row->isSelectable())
			row = row->parent();
		if(row){
			if(onRowLMBDown(row, row->rect(), _toWidget(pointToRootSpace(ev->pos())), ev->modifiers().testFlag(Qt::ControlModifier)))
				capturedRow_ = row;
			else if (!dragCheckMode_){
				row = rowByPoint(ev->pos());
				PropertyRow* draggedRow = row;
				while (draggedRow && (!draggedRow->isSelectable() || draggedRow->pulledUp() || draggedRow->pulledBefore()))
					draggedRow = draggedRow->parent();
				if(draggedRow && !draggedRow->userReadOnly() && !widget_.get()){
					dragController_->beginDrag(row, draggedRow, ev->globalPos());
				}
			}
		}
		update();
	}
	else if (ev->button() == Qt::RightButton)
	{
		QPoint point = ev->pos();
		PropertyRow* row = rowByPoint(point);
		if(row){
			model()->setFocusedRow(row);
			update();

			onRowRMBDown(row, row->rect(), _toWidget(pointToRootSpace(point)));
		}
		else{
			QRect rect = this->rect();
			onRowRMBDown(model()->root(), rect, _toWidget(pointToRootSpace(point)));
		}
	}
}

void QPropertyTree::mouseReleaseEvent(QMouseEvent* ev)
{
	QWidget::mouseReleaseEvent(ev);

	if (ev->button() == Qt::LeftButton)
	{
		 if(dragController_->captured()){
		 	if (dragController_->drop(QCursor::pos()))
				updateHeights();
			else
				update();
		}
		 if (dragCheckMode_) {
			 dragCheckMode_ = false;
		 }
		 else {
			 QPoint point = ev->pos();
			 PropertyRow* row = rowByPoint(point);
			 if(capturedRow_){
				 QRect rowRect = capturedRow_->rect();
				 onRowLMBUp(capturedRow_, rowRect, _toWidget(pointToRootSpace(ev->pos())));
				 mouseStillTimer_->stop();
				 capturedRow_ = 0;
				 update();
			 }
		 }
	}
	else if (ev->button() == Qt::RightButton)
	{

	}
}

void QPropertyTree::focusInEvent(QFocusEvent* ev)
{
	QWidget::focusInEvent(ev);
	widget_.reset();
}

void QPropertyTree::defocusInplaceEditor()
{
	if (hasFocusOrInplaceHasFocus())
		setFocus();
}

void QPropertyTree::keyPressEvent(QKeyEvent* ev)
{
	if (ev->key() == Qt::Key_F && ev->modifiers() == Qt::CTRL) {
		setFilterMode(true);
	}

	if (ev->key() == Qt::Key_Up){
		int y = model()->root()->verticalIndex(this, model()->focusedRow());
		if (filterMode_ && y == 0) {
			setFilterMode(true);
			update();
			return;
		}
	}
	else if (ev->key() == Qt::Key_Down) {
		if (filterMode_ && filterEntry_->hasFocus()) {
			setFocus();
			update();
			return;
		}
	}

	if (filterMode_) {
		if (ev->key() == Qt::Key_Escape && ev->modifiers() == Qt::NoModifier) {
			setFilterMode(false);
		}
	}

	bool result = false;
	if (!widget_.get()) {
		PropertyRow* row = model()->focusedRow();
		if (row) {
			KeyEvent keyEvent;
			keyEvent.key_ = ev->key();
			keyEvent.modifiers_ = ev->modifiers();
			onRowKeyDown(row, &keyEvent);
		}
	}
	update();
	if(!result)
		QWidget::keyPressEvent(ev);
}


void QPropertyTree::mouseDoubleClickEvent(QMouseEvent* ev)
{
	QWidget::mouseDoubleClickEvent(ev);

	QPoint point = ev->pos();
	PropertyRow* row = rowByPoint(point);
	if(row){
		if(row->widgetRect(this).contains(pointToRootSpace(point))){
			if(!row->onActivate(this, true) &&
				!row->onActivateRelease(this))
				toggleRow(row);	
		}
		else if(!toggleRow(row)) {
			row->onActivate(this, false);
			row->onActivateRelease(this);
		}
	}
}

void QPropertyTree::onMouseStillTimer()
{
	onMouseStill();
}

void QPropertyTree::mouseMoveEvent(QMouseEvent* ev)
{
	if(dragController_->captured() && !ev->buttons().testFlag(Qt::LeftButton))
		dragController_->interrupt();
	if(dragController_->captured()){
		QPoint pos = QCursor::pos();
		if (dragController_->dragOn(pos)) {
			// SetCapture
		}
		update();
	}
	else{
		QPoint point = ev->pos();
		PropertyRow* row = rowByPoint(point);
		if (row && dragCheckMode_ && row->widgetRect(this).contains(pointToRootSpace(point))) {
			row->onMouseDragCheck(this, dragCheckValue_);
		}
		else if(capturedRow_){
			onRowMouseMove(capturedRow_, QRect(), point);
			if (sliderUpdateDelay_ >= 0)
				mouseStillTimer_->start(sliderUpdateDelay_);
		}
	}
}

void QPropertyTree::wheelEvent(QWheelEvent* ev) 
{
	QWidget::wheelEvent(ev);
	
	if (scrollBar_->isVisible() && scrollBar_->isEnabled())
 		scrollBar_->setValue(scrollBar_->value() + -ev->delta());
}

bool QPropertyTree::_isDragged(const PropertyRow* row) const
{
	if (!dragController_->dragging())
		return false;
	if (dragController_->draggedRow() == row)
		return true;
	return false;
}

void QPropertyTree::onAttachedTreeChanged()
{
	revert();
}

// vim:ts=4 sw=4:
