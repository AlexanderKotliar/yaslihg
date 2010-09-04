#include "StdAfx.h"

#include "ww/CheckBox.h"
#include "ww/_WidgetWindow.h"
#include "ww/Serialization.h"
#include "ww/Unicode.h"
#include "yasli/TypesFactory.h"
#define WIN32_LEAN_AND_MEAN
#include <windowsx.h>

#pragma warning(push)
#pragma warning(disable: 4355) // 'this' : used in base member initializer list

namespace ww{

YASLI_CLASS(Widget, CheckBox, "CheckBox");

class CheckBoxImpl: public _WidgetWindow{
public:
	CheckBoxImpl(ww::CheckBox* owner);
	const wchar_t* className() const{ return L"BUTTON"; }
	LRESULT onMessage(UINT message, WPARAM wparam, LPARAM lparam);
	LRESULT defaultWindowProcedure(UINT message, WPARAM wparam, LPARAM lparam);

	void setCheckBoxText(const wchar_t* text);
	void setCheckBoxStatus(bool status);
	void updateMinimalSize();
	void updateStyle();
	int generateWin32Style();
protected:

	ww::CheckBox* owner_;
	static WNDPROC controlWindowProc_;
};

WNDPROC CheckBoxImpl::controlWindowProc_;

#pragma warning(push)
#pragma warning(disable: 4312) // 'type cast' : conversion from 'LONG' to 'HINSTANCE' of greater size

CheckBoxImpl::CheckBoxImpl(ww::CheckBox* owner)
: _WidgetWindow(owner)
, owner_(0)
{
//	VERIFY(create(L"", WS_CHILD | WS_TABSTOP , Recti(0, 0, 42, 42), Win32::_globalDummyWindow));
	VERIFY(create(L"", generateWin32Style(), Recti(0, 0, 42, 42), *Win32::_globalDummyWindow));

	controlWindowProc_ = reinterpret_cast<WNDPROC>(::GetWindowLongPtr(handle_, GWLP_WNDPROC));
	::SetWindowLongPtr(handle_, GWLP_WNDPROC, reinterpret_cast<LONG>(&Win32::universalWindowProcedure));
	::SetWindowLongPtr(handle_, GWLP_USERDATA, reinterpret_cast<LONG>(this));

	SetWindowFont(handle_, Win32::defaultFont(), FALSE);
	owner_ = owner;
}

#pragma warning(pop)

LRESULT CheckBoxImpl::defaultWindowProcedure(UINT message, WPARAM wparam, LPARAM lparam)
{
	return ::CallWindowProc(controlWindowProc_, handle_, message, wparam, lparam);
}

void CheckBoxImpl::setCheckBoxStatus(bool status)
{
	ASSERT(::IsWindow(handle_));
	::SendMessage(handle_, BM_SETCHECK, status ? BST_CHECKED : BST_UNCHECKED, 0);
}

void CheckBoxImpl::updateMinimalSize()
{
	HFONT font = GetWindowFont(handle_);
	Vect2i textSize = Win32::calculateTextSize(handle_, font, toWideChar(owner_->text_.c_str()).c_str());

	if(owner_->buttonLike()){
		owner_->_setMinimalSize(textSize + Vect2i(GetSystemMetrics(SM_CXBORDER) * 2 + 6 + 2,
												  GetSystemMetrics(SM_CYBORDER) * 2 + 6 + 2) + Vect2i(owner_->border(), owner_->border()));
	}
	else{
		owner_->_setMinimalSize(textSize + Vect2i(24, 0) + Vect2i(owner_->border(), owner_->border()));
	}
}

int CheckBoxImpl::generateWin32Style()
{
	int style = WS_CHILD | WS_VISIBLE | BS_CHECKBOX | BS_NOTIFY;
	if(owner_ && owner_->buttonLike_)
		style |= BS_PUSHLIKE;
	return style;
}

void CheckBoxImpl::updateStyle()
{
	setStyle(generateWin32Style());
}

void CheckBoxImpl::setCheckBoxText(const wchar_t* text)
{
	ASSERT(::IsWindow(handle_));
	VERIFY(::SetWindowText(handle_, text));

	updateMinimalSize();
	owner_->_queueRelayout();
}

LRESULT CheckBoxImpl::onMessage(UINT message, WPARAM wparam, LPARAM lparam)
{
	switch(message){
		/*
	case WM_SIZE:
	{
		if(handle_){
			UINT width = LOWORD(lparam);
			UINT height = HIWORD(lparam);
			::MoveWindow(handle_, 0, 0, width, height, TRUE);
		}
		return TRUE;
	}
	case WM_ENABLE:
	{
		BOOL enabled = BOOL(wparam);
		if(handle_){
			::EnableWindow(handle_, enabled);
			ASSERT(::IsWindowEnabled(handle_) == enabled);
		}
		return TRUE;
	}
	*/
	case WM_SETFOCUS:
	{
        int result = __super::onMessage(message, wparam, lparam);
        owner_->_setFocus();
        return result;
	}
	case WM_ERASEBKGND:
	{
		HDC dc = (HDC)(wparam);
		return FALSE;
	}
	case WM_COMMAND:
		if(HWND(lparam) == handle_){
			switch(HIWORD(wparam)){
			case BN_CLICKED:
				owner_->onChanged();	
				setCheckBoxStatus(owner_->status());
				return TRUE;
			case BN_SETFOCUS:
				owner_->_setFocus();
				return TRUE;
			}
		}
	}
	return __super::onMessage(message, wparam, lparam);
}



// --------------------------------------------------------------------------------------------

CheckBox::CheckBox(const char* text, int border)
: _WidgetWithWindow(new CheckBoxImpl(this), border), status_(false)
, buttonLike_(false)
{
	_setMinimalSize(Vect2i(24, 24));
	setText(text);
}

void CheckBox::setText(const char* text)
{
	text_ = text;
	window()->setCheckBoxText(toWideChar(text).c_str());
}

void CheckBox::onChanged()
{
	status_ = !status_; 
	signalChanged_.emit();
}

void CheckBox::serialize(Archive& ar)
{
	if(ar.filter(SERIALIZE_DESIGN)){
		ar.serialize(text_, "text", "&Текст");
		ar.serialize(status_, "status", "&Статус");
	}
	Widget::serialize(ar);
}

void CheckBox::setStatus(bool status)
{
	status_ = status;
	window()->setCheckBoxStatus(status_);
}

void CheckBox::setButtonLike(bool buttonLike)
{
	buttonLike_ = buttonLike;
	window()->updateStyle();
}

};


#pragma warning(pop)

