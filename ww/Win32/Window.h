#pragma once

#include "ww/API.h"
#include "ww/Win32/Types.h"
#include "ww/Rect.h"

#include "yasli/Pointers.h"
#include "yasli/Assert.h"
#include "XMath/XMath.h"


struct tagDRAWITEMSTRUCT;
typedef tagDRAWITEMSTRUCT DRAWITEMSTRUCT;

struct tagMEASUREITEMSTRUCT;
typedef tagMEASUREITEMSTRUCT MEASUREITEMSTRUCT;

namespace Win32{
using namespace yasli;

void WW_API _setGlobalInstance(HINSTANCE instance);
HINSTANCE WW_API _globalInstance();
/// ��������� ���������������� �� ����� ���
bool WW_API isClassRegistered(const wchar_t* className);
bool WW_API isKeyPressed(UINT keyCode);

LRESULT CALLBACK universalWindowProcedure(HWND handle, UINT message, WPARAM wparam, LPARAM lparam);

/// ������������ ����� ���� ��������� ���������� ��������� ���������� (�������� ����� onMessage)
//bool WW_API registerClass(const char* className);

/// ���������� � ��������� ��������� ������ �� ��������� WM_QUIT 
int WW_API basicMessageLoop(HACCEL acceleratorTable = 0);

//Vect2i calculateTextSize(HWND window, HFONT font, const char* text);
Vect2i calculateTextSize(HWND window, HFONT font, const wchar_t* text);
HFONT defaultFont();
HFONT defaultBoldFont();
void initializeCommonControls();

class Window32;

class TimerInterface : public ww::RefCounter{
public:
	TimerInterface()
	: window_(0)
	{}
	virtual ~TimerInterface();

	virtual void onTimer(){}
	void setWindow(Win32::Window32* window){ 
		ASSERT(window_ == 0 || window == 0);
		window_ = window;
	}
	Window32* window(){ return window_; }
protected:
	Window32* window_;
};

/// ������������ ���������� BeginDeferWindowPos, DeferWindowPos, EndDeferWindowPos
class WindowPositionDeferer : public ww::RefCounter{
public:
	WindowPositionDeferer(Window32* parent, int numWindows = 1);
	~WindowPositionDeferer();

	void defer(Window32* window, const ww::Rect& position);
protected:
	HDWP handle_;
	Window32* parent_;
};

/// ������������� Windows-����
class WW_API Window32 : public ww::RefCounter{
public:
	Window32(HWND handle = 0);
	virtual ~Window32();
	HWND attach(HWND handle);
	HWND detach();

	virtual const wchar_t* className() const{ return L"ww.Win32.Window"; }
	virtual bool registerClass(const wchar_t* className);
		
	virtual LRESULT onMessage(UINT message, WPARAM wparam, LPARAM lparam);
	
	virtual LRESULT defaultWindowProcedure(UINT message, WPARAM wparam, LPARAM lparam);

	virtual BOOL onMessageMeasureItem(UINT id, MEASUREITEMSTRUCT* drawItemStruct);
	virtual BOOL onMessageDrawItem(UINT id, DRAWITEMSTRUCT* drawItemStruct);

	virtual int onMessageDestroy();
	virtual int onMessageChar(UINT code, USHORT count, USHORT flags);
	virtual int onMessageKeyDown(UINT keyCode, USHORT count, USHORT flags);
	
	virtual int onMessageKeyUp(UINT keyCode, USHORT count, USHORT flags);
	virtual int onMessageSysKeyDown(UINT keyCode, USHORT count, USHORT flags);
	virtual int onMessageSysKeyUp(UINT keyCode, USHORT count, USHORT flags);

	virtual BOOL onMessageEraseBkgnd(HDC dc);
	virtual void onMessageShowWindow(BOOL show, int status);
	virtual BOOL onMessageSize(UINT type, USHORT width, USHORT height);
    virtual BOOL onMessageWindowPosChanged(WINDOWPOS* windowPos);
	virtual int onMessageCommand(USHORT command, USHORT id, HWND wnd);
	virtual int onMessageKillFocus(HWND focusedWindow);
	virtual int onMessageSetFocus(HWND lastFocusedWindow);

	virtual BOOL onMessageSetCursor(HWND window, USHORT hitTest, USHORT message);
	virtual void onMessageMouseMove(UINT button, int x, int y);
	virtual void onMessageMouseWheel(SHORT delta);
	
	virtual void onMessageLButtonDblClk(int x, int y);

	virtual void onMessageLButtonDown(UINT button, int x, int y);
	virtual void onMessageLButtonUp(UINT button, int x, int y);

	virtual void onMessageMButtonDown(UINT button, int x, int y);
	virtual void onMessageMButtonUp(UINT button, int x, int y);

	virtual void onMessageRButtonDown(UINT button, int x, int y);
	virtual void onMessageRButtonUp(UINT button, int x, int y);

	virtual void onMessagePaint();
	virtual void onMessageTimer(int id);

	bool isWindow() const;

	Window32* parent() const;

	// api functions
	bool create(const wchar_t* windowName, UINT style, const ww::Rect& position, HWND parentWnd, UINT exStyle = 0);
	bool creating() const{ return creating_; }
	void destroy();

	void setWindowText(const wchar_t* windowText);
	void enable(bool enabled);
	void show(int showCommand);
	ww::Rect getRect() const;
	void move(const ww::Rect& position, bool redraw = false);
	void update();
	bool isVisible() const;

	void setStyle(UINT style);
	UINT getStyle();
	void setParent(Window32* newParent);

	LONG_PTR setUserDataLongPtr(LONG_PTR newLongPtr);
	LONG_PTR getUserDataLongPtr();

	LONG_PTR getLongPtr(int index);
	LONG_PTR setLongPtr(int index, LONG_PTR newLongPtr);

	typedef ww::SharedPtr<WindowPositionDeferer> PositionDeferer;
	PositionDeferer positionDeferer(int numWindows = 1);	

	void clientToScreen(RECT& rect);
	void screenToClient(RECT& rect);
	//

	HWND get() const{ return handle_; }
	operator HWND() const{ return get(); }

	void attachTimer(TimerInterface* timer, int interval);
	void detachTimer(TimerInterface* timer);
	typedef std::vector<ww::SharedPtr< TimerInterface> > Timers;
protected:
	Timers timers_;
	friend class WindowPositionDeferer;
	WindowPositionDeferer* positionDeferer_;
	bool handleOwner_;
	bool creating_;
	HWND handle_;
};

extern Win32::Window32* _globalDummyWindow;

inline TimerInterface::~TimerInterface()
{
	if(window_)
		window_->detachTimer(this);
}

}

