#include "stdafx.h"
#include "AntTweakBar.h"
#include "UI.h"
#include "D3D.h"

UI::UI(D3D* d3d, HWND hWnd) :
_D3D(d3d),
_Visible(true),
_UserInteracts(false),
_hWnd(hWnd)
{

}

UI::~UI()
{
	for (std::list<UIBar*>::iterator it = _Bars.begin(); it != _Bars.end(); ++it)
		SAFE_DELETE( *it );
	_Bars.clear();
	TwTerminate();
}

LRESULT UI::MessageProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{	
	if (this) // One of the first messages (before this instance even exists) must be catched! Thus this method is called even though the UI is not yet created.
	{
		if ((msg & WM_MOUSEMOVE) == WM_MOUSEMOVE)
		{
			bool mouseOverUI = IsMouseOver();
			if (mouseOverUI && (wParam & MK_LBUTTON) == MK_LBUTTON) 
				_UserInteracts = true;
			if (!mouseOverUI && (wParam & MK_LBUTTON) != MK_LBUTTON)
				_UserInteracts = false;
		}
	}
    return TwEventWin(wnd, msg, wParam, lParam);        
}

bool UI::Init()
{
	if (!TwInit(TW_DIRECT3D11, _D3D->GetDevice()))
        return false;
	
	TwDefine(" GLOBAL help='Stylized Image Triangulation \nETH Zurich, University of Koblenz. (c) 2019' ");

	return true;
}

void UI::RenderUI()
{
	if (this && _Visible)
		TwDraw();
}

UIBar* UI::CreateBar(const std::string& Title, int Width, int Height)
{
	UIBar* bar = new UIBar(this, Title, Width, Height);
	_Bars.push_back(bar);
	return bar;
}

int UI::CreateEnumType(const std::string& Name, const std::string& Enumeration)
{
	return TwDefineEnumFromString(Name.c_str(), Enumeration.c_str());
}

void UI::Hide()
{
	_Visible = false;
}

void UI::Show()
{
	_Visible = true;
}

bool UI::IsVisible() const
{
	return _Visible;
}

bool UI::IsMouseOver() const
{
	for (std::list<UIBar*>::const_iterator it = _Bars.begin(); it != _Bars.end(); ++it)
		if ((*it)->IsMouseOver()) return true;
	return false;
}

bool UI::IsUserInteracting() const
{
	return _UserInteracts;
}


UIBar::UIBar(UI* UI, const std::string& Title, int Width, int Height) :
_UI(UI),
_BarTitle(Title),
_IsVisible(true)
{	
	_Bar = TwNewBar(Title.c_str());    
    int barSize[2] = {Width, Height};
    TwSetParam(_Bar, NULL, "size", TW_PARAM_INT32, 2, barSize);
}

void UIBar::MakeVarRW(const std::string& Name, const std::string& Label, ETwType Type, void* Variable, const std::string& Help, const std::string& Group, const std::string& Step, const std::string& Min, const std::string& Max, const std::string& Key, const std::string& KeyIncr, const std::string& KeyDesc)
{
	std::string def = "label='" + Label + "' ";
	if (Group != "")
		def += "group='" + Group + "' ";
	if (Key != "")
		def += "key=" + Key + " ";
	if (KeyIncr != "")
		def += "keyincr=" + KeyIncr + " ";
	if (KeyDesc != "")
		def += "keydecr=" + KeyDesc + " ";
	if (Help != "")
		def += "help='" + Help + "' ";
	if (Min != "")
		def += "min=" + Min + " ";
	if (Max != "")
		def += "max=" + Max + " ";
	if (Step != "")
		def += "step=" + Step + " ";
	TwAddVarRW(_Bar, Name.c_str(), Type, Variable, def.c_str());
}

void UIBar::MakeVarRO(const std::string& Name, const std::string& Label, ETwType Type, const void* Variable, const std::string& Help, const std::string& Group, const std::string& Step, const std::string& Min, const std::string& Max, const std::string& Key, const std::string& KeyIncr, const std::string& KeyDesc)
{
	std::string def = "label='" + Label + "' ";
	if (Group != "")
		def += "group='" + Group + "' ";
	if (Key != "")
		def += "key=" + Key + " ";
	if (KeyIncr != "")
		def += "keyincr=" + KeyIncr + " ";
	if (KeyDesc != "")
		def += "keydecr=" + KeyDesc + " ";
	if (Help != "")
		def += "help='" + Help + "' ";
	if (Min != "")
		def += "min=" + Min + " ";
	if (Max != "")
		def += "max=" + Max + " ";
	if (Step != "")
		def += "step=" + Step + " ";
	TwAddVarRO(_Bar, Name.c_str(), Type, Variable, def.c_str());
}

void UIBar::MakeVarCB(const std::string& Name, const std::string& Label, ETwType Type, UISetVarCallback SetCallback, UIGetVarCallback GetCallback, void* ClientData, const std::string& Help, const std::string& Group, const std::string& Step, const std::string& Min, const std::string& Max, const std::string& Key, const std::string& KeyIncr, const std::string& KeyDesc)
{	
	std::string def = "label='" + Label + "' ";
	if (Group != "")
		def += "group='" + Group + "' ";
	if (Key != "")
		def += "key=" + Key + " ";
	if (KeyIncr != "")
		def += "keyincr=" + KeyIncr + " ";
	if (KeyDesc != "")
		def += "keydecr=" + KeyDesc + " ";
	if (Help != "")
		def += "help='" + Help + "' ";
	if (Min != "")
		def += "min=" + Min + " ";
	if (Max != "")
		def += "max=" + Max + " ";
	if (Step != "")
		def += "step=" + Step + " ";
	TwAddVarCB(_Bar, Name.c_str(), Type, SetCallback, GetCallback, ClientData, def.c_str());
}

void UIBar::AddButton(const std::string& Name, const std::string& Label, UIButtonCallback ButtonCallback, void* ClientData, const std::string& Help, const std::string& Group, const std::string& Key)
{
	std::string def = "label='" + Label + "' ";
	if (Group != "")
		def += "group='" + Group + "' ";
	if (Key != "")
		def += "key=" + Key + " ";
	if (Help != "")
		def += "help='" + Help + "' ";
	TwAddButton(_Bar, Name.c_str(), ButtonCallback, ClientData, def.c_str());	
}

void UIBar::Remove(const std::string& Name)
{
	TwRemoveVar(_Bar, Name.c_str());
}

void UIBar::RemoveAll()
{
	TwRemoveAllVars(_Bar);
}

UIBar::~UIBar()
{
	_UI->_Bars.remove(this);
	TwDeleteBar(_Bar);	
}

void UIBar::Show()
{
	_IsVisible = true;
	TwDefine((_BarTitle + " visible=true ").c_str());
}

void UIBar::Hide()
{
	_IsVisible = false;
	TwDefine((_BarTitle + " visible=false ").c_str());
}

bool UIBar::IsVisible() const
{
	return _IsVisible;
}

void UIBar::Minimize()
{	
	TwDefine((_BarTitle + " iconified=true ").c_str());
}

void UIBar::Unminimize()
{
	TwDefine((_BarTitle + " iconified=false ").c_str());
}

bool UIBar::isMinimized() const
{
	int ismin;
	TwGetParam(_Bar, NULL, "iconified", TW_PARAM_INT32, 1, &ismin);
	return ismin != 0;
}

bool UIBar::IsMouseOver() const
{
	if (isMinimized()) return false;

	int pos[2];	// position in client space
	TwGetParam(_Bar, NULL, "position", TW_PARAM_INT32, 2, pos);

	int size[2]; // get bar size
	TwGetParam(_Bar, NULL, "size", TW_PARAM_INT32, 2, size);

	POINT arg; // mouse coordinate in screen space.
	if (!GetCursorPos(&arg)) return false;
	
	if (!ScreenToClient(_UI->_hWnd, &arg)) return false;

	// is in?
	return arg.x >= pos[0] && arg.y >= pos[1] && arg.x < pos[0] + size[0] && arg.y < pos[1] + size[1];		
}