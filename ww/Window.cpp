#include "StdAfx.h"
#include "ww/Window.h"
#include "ww/Widget.h"

#include "ww/Win32/Window.h"
#include "ww/Win32/Rectangle.h"
#include "ww/Win32/Handle.h"
#include "ww/HotkeyContext.h"


#include "ww/Serialization.h"
#include "ww/Unicode.h"
#include "yasli/TypesFactory.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace ww{
YASLI_CLASS(Container, Window, "Application Window")

class WindowImpl : public Win32::Window32{
public:
	WindowImpl(ww::Window* window);
	~WindowImpl();
	// virtuals:
	const wchar_t* className() const{ return L"ww.WindowImpl"; }
	LRESULT onMessage(UINT message, WPARAM wparam, LPARAM lparam);
	int onMessageKeyDown(UINT keyCode, USHORT count, USHORT flags);
	// ^^^
protected:
	ww::Window* owner_;
};

WindowImpl::WindowImpl(ww::Window* owner)
: Win32::Window32()
, owner_(owner)
{

}


WindowImpl::~WindowImpl()
{
}

LRESULT WindowImpl::onMessage(UINT message, WPARAM wparam, LPARAM lparam)
{
	switch(message){
	case WM_CLOSE:
		owner_->onClose();
		return 0;
    case WM_ACTIVATE:
        {
            UINT activationMethod = LOWORD(wparam);
            if(activationMethod != WA_INACTIVE)
                owner_->signalActivate_.emit();
        }
        return 0;
	case WM_SETFOCUS:
        if(owner_->_focusedWidget()){
            Widget* focusedOne = owner_->_focusedWidget();
            SetFocus(*focusedOne->_window());
        }
		owner_->onSetFocus();
		break;
	case WM_WINDOWPOSCHANGED:
		{
			WINDOWPOS* windowPos = (WINDOWPOS*)(lparam);
			if(!(windowPos->flags & SWP_NOSIZE)){
				UINT mode = UINT(wparam);
				UINT width = LOWORD(lparam);
				UINT height = HIWORD(lparam);
				if(mode != SIZE_MINIMIZED){
					owner_->_arrangeChildren();

					RedrawWindow(*this, 0, 0, RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE);
				}
			}
			return TRUE;
		}
	case WM_SIZING: 
		{
			// здесь будем проверять чтобы окно не сделали меньше размера
			UINT edge = (UINT)wparam;   // угол/ребро за которое тянет пользователь
			RECT* rect = (RECT*)lparam; // прямоугольник содержащий координаты окна
										// (не клиентской области - а внешней рамки), меняем ниже
			RECT windowRect;
			RECT clientRect;
			::GetWindowRect(*this, &windowRect);
			::GetClientRect(*this, &clientRect);

			POINT borderSize; // размер рамок и TitleBar-а с обоих сторон
							  // (точнее разница в полном размере окна и клиетнской области)
			borderSize.x = windowRect.right - windowRect.left - (clientRect.right - clientRect.left);
			borderSize.y = windowRect.bottom - windowRect.top - (clientRect.bottom - clientRect.top);

			if(rect->right - rect->left < owner_->_minimalSize().x + borderSize.x){
				if(edge == WMSZ_BOTTOMLEFT || edge == WMSZ_TOPLEFT || edge == WMSZ_LEFT)
					rect->left = rect->right - owner_->_minimalSize().x - borderSize.x;
				else if(edge == WMSZ_BOTTOMRIGHT || edge == WMSZ_TOPRIGHT || edge == WMSZ_RIGHT)
					rect->right = rect->left + owner_->_minimalSize().x + borderSize.x;
			}
			if(rect->bottom - rect->top < owner_->_minimalSize().y + borderSize.y){
				if(edge == WMSZ_BOTTOMLEFT || edge == WMSZ_BOTTOMRIGHT || edge == WMSZ_BOTTOM)
					rect->bottom = rect->top + owner_->_minimalSize().y + borderSize.y;
				else if(edge == WMSZ_TOPLEFT || edge == WMSZ_TOPRIGHT || edge == WMSZ_TOP)
					rect->top = rect->bottom - owner_->_minimalSize().y - borderSize.y;
			}
			return TRUE;
		}
	}
	return Win32::Window32::onMessage(message, wparam, lparam);
}

int WindowImpl::onMessageKeyDown(UINT keyCode, USHORT count, USHORT flags)
{
	HotkeyContext* hotkeyContext = owner_->_hotkeyContext();
	if(hotkeyContext && !((flags >> 14) & 1))
		hotkeyContext->injectPress(KeyPress::addModifiers(Key(keyCode)));
	return __super::onMessageKeyDown(keyCode, count, flags);
}

// -------------------------------------------------------------------------------------
Window::Window(Application* app, int border)
{
    style_ = 0;
    init(0, border, app);
}

Window::Window(int border, int style)
{
	style_ = style;
	init(0, border, 0);
}

Window::Window(HWND parent, int border)
{
	style_ = 0;
	init(parent, border, 0);
}

Window::~Window()
{
    if(hotkeyContext_ && app_)
        hotkeyContext_->uninstallFilter(app_);
	defWidget_ = 0;
	if(child_)
		child_->_setParent(0);
	child_ = 0;
	delete window_;
	window_ = 0;
}

void Window::init(HWND owner, int border, Application* app)
{
	resizeable_ = true;
	minimizeable_ = true;
	showTitleBar_ = true;
	windowPosition_ = POSITION_DEFAULT;
	positioned_ = false;
	defaultSize_.set(0, 0);
	defWidget_ = 0;
    focusedWidget_ = 0;
    app_ = app;
	hotkeyContext_ = new HotkeyContext;
    if(app_)
        hotkeyContext_->installFilter(app_);

	window_ = new WindowImpl(this);
	VERIFY(window_->create(toWideChar(title_.c_str()).c_str(), calculateStyle(), Rect(0, 0, 400, 400), owner));
	setBorder(border);
	
	hotkeyContext_->signalPressed(KeyPress(KEY_TAB)).connect(this, &Window::onHotkeyFocusNext);
	hotkeyContext_->signalPressed(KeyPress(KEY_TAB, KEY_MOD_SHIFT)).connect(this, &Window::onHotkeyFocusPrev);
}

unsigned int Window::calculateStyle()
{
    if(style_)
        return style_;  

    int style = WS_CLIPCHILDREN/* | WS_SYSMENU*/;
    if(showTitleBar_)
        style |= WS_CAPTION | WS_SYSMENU;
    else
        if(resizeable_)
            style |= WS_DLGFRAME;

    if(resizeable_)
        style |= WS_MAXIMIZEBOX | WS_THICKFRAME; 
    else{
        style |= WS_POPUP;
        if(!showTitleBar_)
            style |= WS_DLGFRAME;
    }
    if(minimizeable_)
        style |= WS_MINIMIZEBOX;

    return style;
}

void Window::setResizeable(bool allowResize)
{
	resizeable_ = allowResize;
	window_->setStyle(calculateStyle()); 
}

void Window::setRestoredPosition(const Rect& position)
{
	WINDOWPLACEMENT placement;
	placement.length = sizeof(placement);
	VERIFY(GetWindowPlacement(*_window(), &placement));
	placement.rcNormalPosition = Win32::Rect(position);
	VERIFY(SetWindowPlacement(*_window(), &placement));
}

Rect Window::restoredPosition() const
{
	WINDOWPLACEMENT placement;
	placement.length = sizeof(placement);
	VERIFY(GetWindowPlacement(*_window(), &placement));
	return Win32::Rect(placement.rcNormalPosition).recti();
}

void Window::setMinimizeable(bool allowMinimize)
{
	minimizeable_ = allowMinimize;
	window_->setStyle(calculateStyle()); 
}

void Window::setMaximized(bool maximized)
{
	if(this->maximized() != maximized){
		ShowWindow(*_window(), maximized ? SW_MAXIMIZE : SW_RESTORE);
		RedrawWindow(*_window(), 0, 0, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE);
	}	
}

bool Window::maximized() const
{
	return (_window()->getStyle() & WS_MAXIMIZE) != 0;
}

bool Window::isVisible() const
{
	return window_->isVisible();
}

void Window::_updateVisibility()
{
	Widget::_updateVisibility();
	if(_visible()){
		if(!positioned_){
			reposition();
		}
		_arrangeChildren(); // FIXME: hack
		window_->show(SW_SHOW);
	}
	else
		window_->show(SW_HIDE);
}

void Window::setDefaultWidget(Widget* widget)
{
	if(widget && !widget->canBeDefault())
		return;

	if(defWidget_ && defWidget_ != widget)
		defWidget_->setDefaultFrame(false);

	if(widget)
		widget->setDefaultFrame(true);

	defWidget_ = widget;
}

void Window::showAll()
{
	Widget::showAll();	
	if(!focusedWidget_)
		_setFocus();
}


void Window::setTitle(std::string title)
{
	title_ = title;
	window_->setWindowText(toWideChar(title.c_str()).c_str());
}

void Window::setShowTitleBar(bool showTitleBar)
{
	showTitleBar_ = showTitleBar;
	window_->setStyle(calculateStyle()); 
}


void Window::setIconFromResource(const char* resourceName)
{
	HICON icon = LoadIcon(Win32::_globalInstance(), toWideChar(resourceName).c_str());
	ASSERT(icon);
 
	SetClassLong(*window_, GCL_HICON, (LONG)icon);
}

void Window::setIconFromFile(const char* resourceName)
{
	HANDLE icon = LoadImage(0, toWideChar(resourceName).c_str(), IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
	ASSERT(icon);

	SetClassLong(*window_, GCL_HICON, (LONG)icon);
}

void Window::add(Widget* widget)
{
	child_ = widget;
	widget->_setParent(this);
	_arrangeChildren();
}

void Window::remove()
{
	if(child_)
		child_->_setParent(0);
	child_ = 0;
}

void Window::reposition()
{
	int screenWidth = int(GetSystemMetrics(SM_CXSCREEN));
	int screenHeight = int(GetSystemMetrics(SM_CYSCREEN));
	
	int width = std::max(std::max(GetSystemMetrics(SM_CXMINSPACING), _minimalSize().x), defaultSize_.x);
	int height = std::max(_minimalSize().y, defaultSize_.y);
	if(resizeable_){
		width += GetSystemMetrics(SM_CXBORDER);
		height += GetSystemMetrics(SM_CYBORDER);
	}
	if(showTitleBar_){
		height += GetSystemMetrics(SM_CYCAPTION);
	}

    int left = 0;
    int top = 0;
    switch(windowPosition_) {
    case POSITION_LEFT_TOP:
        left = 0;
        top = 0;
        break;
    case POSITION_LEFT:
        left = 0;
        top = (screenHeight - height) / 2;
        break;
    case POSITION_LEFT_BOTTOM:
        left = 0;
        top = screenHeight - height;
        break;
    case POSITION_RIGHT_TOP:
        left = screenWidth - width;
        top = 0;
        break;
    case POSITION_RIGHT:
        left = screenWidth - width;
        top = (screenHeight - height) / 2;
        break;
    case POSITION_RIGHT_BOTTOM:
        left = screenWidth - width;
        top = screenHeight - height;
        break;
    case POSITION_TOP:
        left = (screenWidth - width) / 2;
        top = 0;
        break;
    case POSITION_BOTTOM:
        left = (screenWidth - width) / 2;
        top = screenHeight - height;
        break;
    case POSITION_CENTER:
        left = (screenWidth - width) / 2;
        top = (screenHeight - height) / 2;
        break;
    case POSITION_DEFAULT:
        left = 0;
        top = 0;
        break;
    case POSITION_CURSOR:
    {
        POINT point;
        GetCursorPos(&point);
		left = point.x;
		top = point.y;
        break;
    } 
    }
    Rect rect(left, top, left + width, top + height);

	::SetWindowPos(*window_, 0, rect.left(), rect.top(), rect.width(), rect.height(), SWP_NOZORDER | SWP_NOACTIVATE | (visible_ ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
	positioned_ = true;
}

void Window::setDefaultPosition(WindowPosition position)
{
	windowPosition_ = position;
	positioned_ = false;
	reposition();
}

void Window::setDefaultSize(Vect2 size)
{
	defaultSize_ = size;
	positioned_ = false;
	reposition();
}

void Window::setDefaultSize(int w, int h)
{
	setDefaultSize(Vect2(w, h));
}

void Window::visitChildren(WidgetVisitor& visitor) const
{
	if(child_)
		visitor(*child_);
}

void Window::_arrangeChildren()
{
	if(child_){
		RECT rect;
		VERIFY(::GetClientRect(*window_, &rect));
		VERIFY(::InflateRect(&rect, -border_, -border_));
		child_->_setPosition(Rect(rect.left, rect.top, rect.right, rect.bottom));
	}
}

void Window::_relayoutParents()
{
	if(child_)
		_setMinimalSize(child_->_minimalSize() + Vect2(border_ * 2, border_ * 2));
	else
		_setMinimalSize(border_ * 2, border_ * 2);

	RECT clientRect;
	::GetClientRect(*window_, &clientRect);
	bool move = false;
	RECT windowRect;
	::GetWindowRect(*window_, &windowRect);
	SIZE borderSize = { windowRect.right - windowRect.left - (clientRect.right - clientRect.left),
						windowRect.bottom - windowRect.top - (clientRect.bottom - clientRect.top) };

	if(clientRect.right - clientRect.left < _minimalSize().x){
		windowRect.right = windowRect.left + _minimalSize().x + borderSize.cx;
		move = true;
	}
	if(clientRect.bottom - clientRect.top < _minimalSize().y){
		windowRect.bottom = windowRect.top + _minimalSize().y + borderSize.cy;
        move = true;
	}
	if(move){
		window_->move(Rect(windowRect.left, windowRect.top, windowRect.right, windowRect.bottom));
		window_->update();
	}
	else
		::RedrawWindow(*window_, 0, 0, RDW_INVALIDATE | RDW_ALLCHILDREN);

	__super::_relayoutParents();
}

void Window::_setFocus()
{
    if(_focusable())
    	__super::_setFocus();
}

void Window::onClose()
{
	hide();
	signalClose().emit();
}

void Window::serialize(Archive& ar)
{
	if(ar.filter(SERIALIZE_DESIGN)){
		ar(_property(title_, this, &Window::setTitle), "title", "Title");
		ar(_property(defaultSize_, this, &Window::setDefaultSize), "defaultSize", "Default Size");
		ar(resizeable_, "resizeable", "Resizeable");
		if(resizeable_){
			ar(minimizeable_, "resizeable", "Minimizeable");
			ar(showTitleBar_, "showTitleBar", "Show Title Bar");
		}
		ar(child_, "widget", "Widget");
	}
    else if(ar.filter(SERIALIZE_STATE)){
		Rect position = restoredPosition();
		bool maximized = this->maximized();

		ar.serialize(position, "restoredPosition", "Restored Position");
		ar.serialize(maximized, "maximized", "Maximized");

		if(ar.isInput()){
			setRestoredPosition(position);
			setMaximized(maximized);
		}
        if(child_)
            ar(*child_, "widget", "Widget");
	}
}

void Window::onHotkeyFocusNext()
{
	passFocus(ww::FOCUS_NEXT);
}

void Window::onHotkeyFocusPrev()
{
	passFocus(ww::FOCUS_PREVIOUS);
}

void Window::_setFocusedWidget(Widget* widget)
{
	focusedWidget_ = widget;
	setDefaultWidget(widget);
}

}
