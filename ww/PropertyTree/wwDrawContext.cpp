/**
 *  wWidgets - Lightweight UI Toolkit.
 *  Copyright (C) 2009-2011 Evgeny Andreeshchev <eugene.andreeshchev@gmail.com>
 *                          Alexander Kotliar <alexander.kotliar@gmail.com>
 * 
 *  This code is distributed under the MIT License:
 *                          http://www.opensource.org/licenses/MIT
 */

#include "stdafx.h"
#include "wwDrawContext.h"
#include "Win32/CommonControls.h"
#include <windows.h>
#include <memory>
#include "gdiplusUtils.h"
#include "yasli/Assert.h"
#include "Win32/Drawing.h"
#include "Win32/Window32.h"
#include "PropertyTree.h"
#include "PropertyTree/Color.h"
#include "Color.h"
#include "ww/Icon.h"

#include <uxtheme.h>
#include <vssym32.h>

using namespace Gdiplus;

enum { ICON_SIZE = 16 };

static RECT toRECT(const property_tree::Rect& r)
{
	RECT result = { r.x, r.y, r.x + r.w, r.y + r.h };
	return result;
}

static Gdiplus::Rect gdiplusRect(const property_tree::Rect& r)
{
	return Gdiplus::Rect(r.x, r.y, r.w, r.h);
}

// uxtheme.dll provides an API for drawing XP-themes
// Linking at runtime to stay compatible with Windows 2000.
struct DynamicUxTheme
{
	typedef BOOL(__stdcall *PFNISAPPTHEMED)();
	typedef HRESULT(__stdcall *PFNCLOSETHEMEDATA)(HTHEME hTheme);
	typedef HRESULT(__stdcall *PFNDRAWTHEMEBACKGROUND)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pRect, const RECT *pClipRect);
	typedef HTHEME(__stdcall *PFNOPENTHEMEDATA)(HWND hwnd, LPCWSTR pszClassList);
	typedef HRESULT (__stdcall *PFNDRAWTHEMETEXT)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, LPCWSTR pszText, int iCharCount, DWORD dwTextFlags, DWORD dwTextFlags2, const RECT *pRect);
	typedef HRESULT (__stdcall *PFNGETTHEMEBACKGROUNDCONTENTRECT)(HTHEME hTheme, HDC hdc, int iPartId, int iStateId, const RECT *pBoundingRect, RECT *pContentRect);

	HMODULE module_;
	PFNISAPPTHEMED IsAppThemed;
	PFNOPENTHEMEDATA OpenThemeData;
	PFNDRAWTHEMEBACKGROUND DrawThemeBackground;
	PFNCLOSETHEMEDATA CloseThemeData;
	PFNDRAWTHEMETEXT DrawThemeText;
	PFNGETTHEMEBACKGROUNDCONTENTRECT GetThemeBackgroundContentRect;

	DynamicUxTheme()
	: module_(0)
	, OpenThemeData(0)
	, DrawThemeBackground(0)
	, CloseThemeData(0)
	, DrawThemeText(0)
	, GetThemeBackgroundContentRect(0)
	{
	}

	~DynamicUxTheme()
	{
		freeLibrary();
	}

	bool isLoaded() const{ return module_ != 0; }

	void loadLibrary()
	{
		module_ = LoadLibraryW(L"uxtheme.dll");
		if (!module_)
			return;

		IsAppThemed = (PFNISAPPTHEMED)GetProcAddress(module_, "IsAppThemed");
		OpenThemeData = (PFNOPENTHEMEDATA)GetProcAddress(module_, "OpenThemeData");
		DrawThemeBackground = (PFNDRAWTHEMEBACKGROUND)GetProcAddress(module_, "DrawThemeBackground");
		CloseThemeData = (PFNCLOSETHEMEDATA)GetProcAddress(module_, "CloseThemeData");
		DrawThemeText = (PFNDRAWTHEMETEXT)GetProcAddress(module_, "DrawThemeText");
		GetThemeBackgroundContentRect = (PFNGETTHEMEBACKGROUNDCONTENTRECT)GetProcAddress(module_, "GetThemeBackgroundContentRect");

		if (!OpenThemeData ||
			!DrawThemeBackground ||
			!CloseThemeData ||
			!DrawThemeText ||
			!GetThemeBackgroundContentRect)
		{
			FreeLibrary(module_);
			module_ = 0;
			return;
		}	 
	}

	void freeLibrary()
	{
		if (!module_)
			return;

		FreeLibrary(module_);
		module_ = 0;

		IsAppThemed = 0;
		OpenThemeData = 0;
		DrawThemeBackground = 0;
		CloseThemeData = 0;
		DrawThemeText = 0;
		GetThemeBackgroundContentRect = 0;
	}

} static uxTheme;

namespace Win32 {
	bool isAppThemed()
	{
		return uxTheme.isLoaded() && uxTheme.IsAppThemed();
	}
}

// ---------------------------------------------------------------------------

enum{
	OBM_CHECK = 32760
};
static HBITMAP checkBitmap = ::LoadBitmap(0, (LPCTSTR)OBM_CHECK);
static std::auto_ptr<Gdiplus::Font> defaultFont;
static std::auto_ptr<Gdiplus::Font> defaultBoldFont;

namespace property_tree{

static DrawingCache drawingCache;

DrawingCache* DrawingCache::get()
{
	return &drawingCache;
}

DrawingCache::~DrawingCache()
{
}

void DrawingCache::initialize()
{
	Gdiplus::GdiplusStartupInput startup;
	startup.DebugEventCallback = 0;
	startup.GdiplusVersion = 1;
	startup.SuppressBackgroundThread = FALSE;

	if(users_ == 0){
		uxTheme.loadLibrary();
		theme_ = uxTheme.OpenThemeData(Win32::getDefaultWindowHandle(), L"Button");

		YASLI_ESCAPE(GdiplusStartup(&token_, &startup, 0) == Ok, return);

		NONCLIENTMETRICS nonClientMetrics;
		nonClientMetrics.cbSize = sizeof(nonClientMetrics);
		SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(nonClientMetrics), (PVOID)&nonClientMetrics, 0);
		defaultFont.reset(new Gdiplus::Font(GetDC(GetDesktopWindow()), &nonClientMetrics.lfMessageFont));

		nonClientMetrics.lfMessageFont.lfWeight = FW_BOLD;
		defaultBoldFont.reset(new Gdiplus::Font(GetDC(GetDesktopWindow()), &nonClientMetrics.lfMessageFont));
	}

	++users_;
}

void DrawingCache::flush()
{
	IconToBitmap::iterator it;
	for (it = iconToBitmapMap_.begin(); it != iconToBitmapMap_.end(); ++it)
		delete it->second.bitmap;
	iconToBitmapMap_.clear();
}

void DrawingCache::finalize()
{
	YASLI_ESCAPE(users_ > 0, return);
	--users_;
	if(users_ == 0)
	{
		defaultFont.reset();
		defaultBoldFont.reset();

		flush();

		GdiplusShutdown(token_);
		if (theme_) {
			uxTheme.CloseThemeData(theme_);
			theme_ = 0;
		}
		uxTheme.freeLibrary();
	}
}

Gdiplus::Bitmap* DrawingCache::getBitmapForIcon(const ww::Icon& icon)
{
	IconToBitmap::iterator it = iconToBitmapMap_.find(icon);
	if (it != iconToBitmapMap_.end())
		return it->second.bitmap;

	ww::RGBAImage image;
	YASLI_ESCAPE(icon.getImage(&image), return 0);

	BitmapCache& cache = iconToBitmapMap_[icon];
	cache.pixels.swap(image.pixels_);
	cache.bitmap = new Gdiplus::Bitmap(image.width_, image.height_, image.width_ * 4, PixelFormat32bppARGB, (BYTE*)&cache.pixels[0]);
	return cache.bitmap;
}

// ---------------------------------------------------------------------------

Gdiplus::Font* propertyTreeDefaultFont()
{
	return defaultFont.get();
}

Gdiplus::Font* propertyTreeDefaultBoldFont()
{
	return defaultBoldFont.get();
}


void createRoundRectanglePath(GraphicsPath* path, const Gdiplus::Rect &_rect, int dia)
{
	using Gdiplus::Rect;
    Rect r = _rect;
    if(dia > r.Width)
        dia = r.Width;
    if(dia > r.Height)  
        dia = r.Height;

    Rect corner(r.X, r.Y, dia, dia);
    path->Reset();

    path->AddArc(corner, 180, 90);    

    if(dia == 20) {
        corner.Width += 1; 
        corner.Height += 1; 
        r.Width -=1; r.Height -= 1;
    }

    corner.X += (r.Width - dia - 1);
    path->AddArc(corner, 270, 90);    
    
    corner.Y += (r.Height - dia - 1);
    path->AddArc(corner,   0, 90);    
    
    corner.X -= (r.Width - dia - 1);
    path->AddArc(corner,  90, 90);
    path->CloseFigure();
}

void drawRoundRectangle(Gdiplus::Graphics* graphics, const Gdiplus::Rect &_r, unsigned int color, int radius, int width)
{
	Gdiplus::Rect r = _r;
    int dia = 2*radius;

    int oldPageUnit = graphics->SetPageUnit(UnitPixel);

	Pen pen(color, 1);
    pen.SetAlignment(PenAlignmentCenter);

    GraphicsPath path;

    createRoundRectanglePath(&path, r, dia);
    graphics->DrawPath(&pen, &path);

    for(int i=1; i<width; i++)
    {
		r.Inflate(-1, 0);
        createRoundRectanglePath(&path, r, dia);
            
        graphics->DrawPath(&pen, &path);

		r.Inflate(0, -1);

        createRoundRectanglePath(&path, r, dia);
        graphics->DrawPath(&pen, &path);
    }

    graphics->SetPageUnit((Unit)oldPageUnit);
}

void fillRoundRectangle(Gdiplus::Graphics* gr, Gdiplus::Brush* brush, const Gdiplus::Rect& r, const Gdiplus::Color& border, int radius)
{
    int dia = 2*radius;

    int oldPageUnit = gr->SetPageUnit(UnitPixel);

    Pen pen(border, 1);    
    pen.SetAlignment(PenAlignmentCenter);

    GraphicsPath path;
    createRoundRectanglePath(&path, r, dia);

    gr->FillPath(brush, &path);
    gr->DrawPath(&pen, &path);
    gr->SetPageUnit((Unit)oldPageUnit);
}


void drawEdit(Gdiplus::Graphics* gr, const Gdiplus::Rect& rect, const wchar_t* text, const Gdiplus::Font *font, bool pathEllipsis, bool grayBackground)
{
	using Gdiplus::Color;
    Gdiplus::Rect rt = rect;
    rt.Inflate( -1, -1 );
	
	Color lightColor = gdiplusSysColor(grayBackground ? COLOR_BTNFACE : COLOR_WINDOW);
	Color darkColor = gdiplusSysColor(grayBackground ? COLOR_3DSHADOW : COLOR_BTNFACE);
	LinearGradientBrush brush(Gdiplus::Rect(rt.X, rt.Y, rt.Width, rt.Height), Color(), Color(), LinearGradientModeVertical);

	Color colors[3] = { darkColor, lightColor, lightColor };
	Gdiplus::REAL positions[3] = { 0.0f, 0.4f, 1.0f };
	brush.SetInterpolationColors(colors, positions, 3);

	Color penColor;
	penColor.SetFromCOLORREF(GetSysColor(COLOR_3DSHADOW));
    fillRoundRectangle(gr, &brush, rt, penColor, 5);

	rt.Inflate(-3, -1);
	rt.Height -= 1;
	ww::Rect textRect( rt.GetLeft(), rt.GetTop(), rt.GetRight(), rt.GetBottom() );
	StringFormat format;
	format.SetAlignment(StringAlignmentNear);
	format.SetLineAlignment(StringAlignmentCenter);
	format.SetFormatFlags(StringFormatFlagsNoWrap);
	format.SetTrimming(pathEllipsis ? StringTrimmingEllipsisPath : StringTrimmingEllipsisCharacter);
	
	SolidBrush textBrush(gdiplusSysColor(COLOR_WINDOWTEXT));
    gr->DrawString(text, (int)wcslen(text), font, gdiplusRectF(textRect), &format, &textBrush );
}

// ---------------------------------------------------------------------------

/*
void wwDrawContext::drawIcon(const Rect& rect, const ww::Icon& icon) const
{
	Gdiplus::Bitmap* bitmap = DrawingCache::get()->getBitmapForIcon(icon);
	YASLI_ESCAPE(bitmap != 0, return);
	int x = rect.left() + (rect.width() - icon.width()) / 2;
	int y = rect.top() + (rect.height() - icon.height()) / 2;
	graphics->DrawImage(bitmap, x, y);
}
*/

void wwDrawContext::drawCheck(const Rect& rect, bool disabled, CheckState checked)
{
	bool enabled = !disabled;
	using Gdiplus::Color;
	int size = 17;

	int offsetY = ((rect.width()) - size) / 2;
	int offsetX = ((rect.height()) - size) / 2;

	Color brushColor;
	Color penColor;
	if (enabled) {
		brushColor.SetFromCOLORREF(GetSysColor(COLOR_WINDOW));
		penColor.SetFromCOLORREF(GetSysColor(COLOR_3DSHADOW));
	}
	else {
		ww::Color shadow, face, mix;
		shadow.setGDI(GetSysColor(COLOR_3DSHADOW));
		face.setGDI(GetSysColor(COLOR_BTNFACE));
		mix = shadow.interpolate(face, 0.5f);
		
		penColor.SetFromCOLORREF(mix.rgba());
		brushColor.SetFromCOLORREF(face.rgba());
	}

	SolidBrush brush(brushColor);
	Gdiplus::Rect checkRect(rect.left() + offsetX, rect.top() + offsetY, size, size);
	fillRoundRectangle(graphics, &brush, checkRect, penColor, 4);

	if(checked == CHECK_SET){
		#include "check.xpm"
		static ww::Icon checkIcon(check_xpm);
		Gdiplus::Bitmap* bitmap = drawingCache.getBitmapForIcon(checkIcon);
		graphics->DrawImage(bitmap, checkRect);
	}
	else if (checked == CHECK_IN_BETWEEN)
	{
		#include "check_tristate.xpm"
		static ww::Icon checkIcon(check_tristate_xpm);
		Gdiplus::Bitmap* bitmap = drawingCache.getBitmapForIcon(checkIcon);
		graphics->DrawImage(bitmap, checkRect);
	}
}

void wwDrawContext::drawButton(const Rect& rect, const wchar_t* text, bool pressed, bool focused, bool enabled, bool center, bool dropDownArrow, property_tree::Font font)
{
	using Gdiplus::Color;
	using Gdiplus::Rect;

	if (uxTheme.isLoaded() && uxTheme.IsAppThemed() && drawingCache.theme_)
	{
		HDC dc = graphics->GetHDC();
		RECT buttonRect = toRECT(rect);
		int state = (pressed ? PBS_PRESSED : PBS_NORMAL) | (focused ? PBS_HOT : 0);
		uxTheme.DrawThemeBackground(DrawingCache::get()->theme_, dc, BP_PUSHBUTTON, state, &buttonRect, 0);
		graphics->ReleaseHDC(dc);
	}
	else
	{
		Color brushColor;
		brushColor.SetFromCOLORREF(GetSysColor(COLOR_BTNFACE));
		Gdiplus::SolidBrush brush(brushColor);

		Rect buttonRect = gdiplusRect(rect);
		graphics->FillRectangle(&brush, buttonRect);

		HDC dc =  graphics->GetHDC();
		RECT rt = toRECT(rect);
		DrawFrameControl(dc, &rt, DFC_BUTTON, (pressed ? DFCS_PUSHED : 0) | DFCS_BUTTONPUSH);
		graphics->ReleaseHDC(dc);
	}
	
	StringFormat format;
	format.SetAlignment(StringAlignmentCenter);
	format.SetLineAlignment(StringAlignmentCenter);
	format.SetFormatFlags(StringFormatFlagsNoWrap);
	format.SetTrimming(StringTrimmingEllipsisCharacter);
	Color textColor;
	textColor.SetFromCOLORREF(GetSysColor(COLOR_WINDOWTEXT));
	SolidBrush textBrush(textColor);

	Rect textRect = gdiplusRect(rect);
	textRect.X += 2;
	textRect.Y += 1;
	textRect.Width -= 4;
	if(pressed){
		textRect.X += 1;
		textRect.Y += 1;
	}

	graphics->DrawString( text, (int)wcslen(text), propertyTreeDefaultFont(), RectF(Gdiplus::REAL(textRect.X), Gdiplus::REAL(textRect.Y), Gdiplus::REAL(textRect.Width), Gdiplus::REAL(textRect.Height)), &format, &textBrush );
}


void wwDrawContext::drawValueText(bool highlighted, const char* text)
{
	ww::Color textColor;
	textColor.setGDI(GetSysColor(highlighted ? COLOR_HIGHLIGHTTEXT : COLOR_BTNTEXT));

	ww::Rect textRect(widgetRect.left() + 3, widgetRect.top() + 2, widgetRect.right() - 3, widgetRect.bottom() - 2);

	tree_->_drawRowValue(graphics, text, propertyTreeDefaultFont(), textRect, textColor, false, false);
}

void wwDrawContext::drawEntry(const char* text, bool pathEllipsis, bool grayBackground, int trailingOffset)
{
	using Gdiplus::Color;
    Gdiplus::Rect rt = gdiplusRect(widgetRect.adjusted(0, 0, -trailingOffset, 0));
	Gdiplus::Font* font = propertyTreeDefaultFont();
    rt.Inflate( -1, -1 );
	
	Color lightColor = gdiplusSysColor(grayBackground ? COLOR_BTNFACE : COLOR_WINDOW);
	Color darkColor = gdiplusSysColor(grayBackground ? COLOR_3DSHADOW : COLOR_BTNFACE);
	LinearGradientBrush brush(Gdiplus::Rect(rt.X, rt.Y, rt.Width, rt.Height), Color(), Color(), LinearGradientModeVertical);

	Color colors[3] = { darkColor, lightColor, lightColor };
	Gdiplus::REAL positions[3] = { 0.0f, 0.4f, 1.0f };
	brush.SetInterpolationColors(colors, positions, 3);

	Color penColor;
	penColor.SetFromCOLORREF(GetSysColor(COLOR_3DSHADOW));
    fillRoundRectangle(graphics, &brush, rt, penColor, 5);

	rt.Inflate(-3, -1);
	rt.Height -= 1;
	ww::Rect textRect( rt.GetLeft(), rt.GetTop(), rt.GetRight(), rt.GetBottom() );
	
	ww::Color color;
	color.setGDI(GetSysColor(COLOR_WINDOWTEXT));
	tree_->_drawRowValue(graphics, text, font, textRect, color, pathEllipsis, false);
}

void wwDrawContext::drawColor(const Rect& _rect, const Color& _color)
{
	using Gdiplus::Color;
	Rect rect = _rect.adjusted(1, 0, -1, 0);

	Color color(_color.r, _color.g, _color.b, _color.a);
	SolidBrush brush(color);
	Color penColor;
	penColor.SetFromCOLORREF(GetSysColor(COLOR_3DSHADOW));

	HatchBrush hatchBrush(HatchStyleSmallCheckerBoard, Color::Black, Color::White);

	if (rect.width() > ICON_SIZE * 2 + 5){
		Rect rectb(rect.right() - ICON_SIZE - 3, rect.top(), rect.right(), rect.bottom());
		fillRoundRectangle(graphics, &hatchBrush, gdiplusRect(rectb), Color(0, 0, 0, 0), 6);
		fillRoundRectangle(graphics, &brush, gdiplusRect(rectb), penColor, 6);

		SolidBrush brushb(Color(255, color.GetR(), color.GetG(), color.GetB()));
		Rect recta(rect.left(), rect.top(), rect.right() - ICON_SIZE - 5, rect.bottom());
		fillRoundRectangle(graphics, &brushb, gdiplusRect(recta), penColor, 6);

	}
	else{
		fillRoundRectangle(graphics, &hatchBrush, gdiplusRect(rect), Color(0, 0, 0, 0), 6);
		fillRoundRectangle(graphics, &brush, gdiplusRect(rect), penColor, 6);
	}
}

}