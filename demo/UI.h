#pragma once

#include <list>
#include <Windows.h>
#include <string>
#include <sstream>
#include "AntTweakBar.h"

class D3D;
class UIBar;

// Initializes UI stuff
class UI
{
	public:

		friend class UIBar;

		// Base constructor.
		explicit UI(D3D* D3D, HWND hWnd);

		// Destructor.
		virtual ~UI();

		LRESULT MessageProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);

		// Initializes the UI stuff
		bool Init();

		// Renders the UI.
		void RenderUI();

		// Creates a floating bar.
		UIBar* CreateBar(const std::string& Title, int Width, int Height);

		// Creates an enumeration type, e.g. "one,two,three".
		int CreateEnumType(const std::string& Name, const std::string& Enumeration);

		// Hides the UI.
		void Hide();
		// Shows the UI.
		void Show();

		// Gets whether the UI is visible.
		bool IsVisible() const;

		// Checks whether the mouse hovers above any bar of the UI.
		bool IsMouseOver() const;

		// Checks whether the user interacts.
		bool IsUserInteracting() const;

	private:

		bool _UserInteracts;

		D3D* _D3D;

		// Flag that determines whether the UI is visible.
		bool _Visible;

		// List of bars in the UI.
		std::list<UIBar*> _Bars;

		HWND _hWnd;
};

#ifndef UI_CALL
#define UI_CALL __stdcall
#endif
typedef void (UI_CALL * UISetVarCallback)(const void *value, void *clientData);
typedef void (UI_CALL * UIGetVarCallback)(void *value, void *clientData);
typedef void (UI_CALL * UIButtonCallback)(void *clientData);

template <typename T>
static std::string ToStr ( T Number )
{
	std::stringstream ss;
	ss << Number;
	return ss.str();
}

class UIBar
{
	friend class UI;

	public:		

		~UIBar();

		void AddVarBool(const std::string& Name, const std::string& Label, bool* Variable, const std::string& Group, const std::string& Help, const std::string& Key = "") { MakeVarRW(Name, Label, TW_TYPE_BOOLCPP, Variable, Help, Group, "", "", "", Key, "", ""); }
		void AddVarBool(const std::string& Name, const std::string& Label, const bool* Variable, const std::string& Group, const std::string& Help, const std::string& Key = "") { MakeVarRO(Name, Label, TW_TYPE_BOOLCPP, Variable, Help, Group, "", "", "", Key, "", ""); }		
		void AddVarBool(const std::string& Name, const std::string& Label, UISetVarCallback SetCallback, UIGetVarCallback GetCallback, void* ClientData, const std::string& Group, const std::string& Help, const std::string& Key="") { MakeVarCB(Name, Label, TW_TYPE_BOOLCPP, SetCallback, GetCallback, ClientData, Help, Group, "", "", "", Key, "", ""); }

		void AddVarInt(const std::string& Name, const std::string& Label, int* Variable, const int& MinValue, const int& MaxValue, const std::string& Group, const std::string& Help, const int& Step=1, const std::string& KeyDecr = "", const std::string& KeyIncr = "") { MakeVarRW(Name, Label, TW_TYPE_INT32, Variable, Help, Group, ToStr(Step), ToStr(MinValue), ToStr(MaxValue), "", KeyIncr, KeyDecr); }
		void AddVarInt(const std::string& Name, const std::string& Label, const int* Variable, const int& MinValue, const int& MaxValue, const std::string& Group, const std::string& Help, const int& Step=1, const std::string& KeyDecr = "", const std::string& KeyIncr = "") { MakeVarRO(Name, Label, TW_TYPE_INT32, Variable, Help, Group, ToStr(Step), ToStr(MinValue), ToStr(MaxValue), "", KeyIncr, KeyDecr); }		
		void AddVarInt(const std::string& Name, const std::string& Label, UISetVarCallback SetCallback, UIGetVarCallback GetCallback, void* ClientData, const int& MinValue, const int& MaxValue, const std::string& Group, const std::string& Help, const int& Step=1, const std::string& KeyDecr = "", const std::string& KeyIncr = "") { MakeVarCB(Name, Label, TW_TYPE_INT32, SetCallback, GetCallback, ClientData, Help, Group, ToStr(Step), ToStr(MinValue), ToStr(MaxValue), "", KeyIncr, KeyDecr); }

		void AddVarFloat(const std::string& Name, const std::string& Label, float* Variable, const float& MinValue, const float& MaxValue, const std::string& Group, const std::string& Help, const float& Step=1, const std::string& KeyDecr = "", const std::string& KeyIncr = "") { MakeVarRW(Name, Label, TW_TYPE_FLOAT, Variable, Help, Group, ToStr(Step), ToStr(MinValue), ToStr(MaxValue), "", KeyIncr, KeyDecr); }
		void AddVarFloat(const std::string& Name, const std::string& Label, const float* Variable, const float& MinValue, const float& MaxValue, const std::string& Group, const std::string& Help, const float& Step=1, const std::string& KeyDecr = "", const std::string& KeyIncr = "") { MakeVarRO(Name, Label, TW_TYPE_FLOAT, Variable, Help, Group, ToStr(Step), ToStr(MinValue), ToStr(MaxValue), "", KeyIncr, KeyDecr); }		
		void AddVarFloat(const std::string& Name, const std::string& Label, UISetVarCallback SetCallback, UIGetVarCallback GetCallback, void* ClientData, const float& MinValue, const float& MaxValue, const std::string& Group, const std::string& Help, const float& Step=1, const std::string& KeyDecr = "", const std::string& KeyIncr = "") { MakeVarCB(Name, Label, TW_TYPE_FLOAT, SetCallback, GetCallback, ClientData, Help, Group, ToStr(Step), ToStr(MinValue), ToStr(MaxValue), "", KeyIncr, KeyDecr); }

		void AddVarDouble(const std::string& Name, const std::string& Label, double* Variable, const double& MinValue, const double& MaxValue, const std::string& Group, const std::string& Help, const double& Step=1, const std::string& KeyDecr = "", const std::string& KeyIncr = "") { MakeVarRW(Name, Label, TW_TYPE_DOUBLE, Variable, Help, Group, ToStr(Step), ToStr(MinValue), ToStr(MaxValue), "", KeyIncr, KeyDecr); }
		void AddVarDouble(const std::string& Name, const std::string& Label, const double* Variable, const double& MinValue, const double& MaxValue, const std::string& Group, const std::string& Help, const double& Step=1, const std::string& KeyDecr = "", const std::string& KeyIncr = "") { MakeVarRO(Name, Label, TW_TYPE_DOUBLE, Variable, Help, Group, ToStr(Step), ToStr(MinValue), ToStr(MaxValue), "", KeyIncr, KeyDecr); }		
		void AddVarDouble(const std::string& Name, const std::string& Label, UISetVarCallback SetCallback, UIGetVarCallback GetCallback, void* ClientData, const double& MinValue, const double& MaxValue, const std::string& Group, const std::string& Help, const double& Step=1, const std::string& KeyDecr = "", const std::string& KeyIncr = "") { MakeVarCB(Name, Label, TW_TYPE_DOUBLE, SetCallback, GetCallback, ClientData, Help, Group, ToStr(Step), ToStr(MinValue), ToStr(MaxValue), "", KeyIncr, KeyDecr); }

		void AddVarColor3(const std::string& Name, const std::string& Label, float* Variable, const std::string& Group, const std::string& Help) { MakeVarRW(Name, Label, TW_TYPE_COLOR3F, Variable, Help, Group, "", "", "", "", "", ""); }
		void AddVarColor3(const std::string& Name, const std::string& Label, const float* Variable, const std::string& Group, const std::string& Help) { MakeVarRO(Name, Label, TW_TYPE_COLOR3F, Variable, Help, Group, "", "", "", "", "", ""); }		
		void AddVarColor3(const std::string& Name, const std::string& Label, UISetVarCallback SetCallback, UIGetVarCallback GetCallback, void* ClientData, const std::string& Group, const std::string& Help) { MakeVarCB(Name, Label, TW_TYPE_COLOR3F, SetCallback, GetCallback, ClientData, Help, Group, "", "", "", "", "", ""); }

		void AddVarEnum(const std::string& Name, const std::string& Label, void* Variable, const std::string& Group, const std::string& Help, int EnumType) { MakeVarRW(Name, Label, (ETwType)EnumType, Variable, Help, Group, "","","","","",""); }
		void AddVarEnum(const std::string& Name, const std::string& Label, const void* Variable, const std::string& Group, const std::string& Help, int EnumType) { MakeVarRO(Name, Label, (ETwType)EnumType, Variable, Help, Group, "","","","","",""); }
		void AddVarEnum(const std::string& Name, const std::string& Label, UISetVarCallback SetCallback, UIGetVarCallback GetCallback, void* ClientData, const std::string& Group, const std::string& Help, int EnumType) { MakeVarCB(Name, Label, (ETwType)EnumType, SetCallback, GetCallback, ClientData, Help, Group, "", "", "", "", "", ""); }

		void AddButton(const std::string& Name, const std::string& Label, UIButtonCallback ButtonCallback, void* ClientData, const std::string& Help, const std::string& Group, const std::string& Key="");
		
		// Removes variables, buttons, comments, enums, ...
		void Remove(const std::string& Name);
		// Removes all variables, buttons, ...
		void RemoveAll();
		
		// Checks whether the mouse hovers above the bar.
		bool IsMouseOver() const;

		void Show();
		void Hide();
		bool IsVisible() const;

		void Minimize();
		void Unminimize();
		bool isMinimized() const;

		UI* GetUI() { return _UI; }

	private:

		bool _IsVisible;		

		std::string _BarTitle;

		UIBar(UI* _UI, const std::string& _Title, int Width, int Height);

		UI* _UI;

		TwBar* _Bar;

		void MakeVarRW(const std::string& Name, const std::string& Label, ETwType Type, void* Variable, const std::string& Help, const std::string& Group, const std::string& Step, const std::string& Min, const std::string& Max, const std::string& Key, const std::string& KeyIncr, const std::string& KeyDesc);
		void MakeVarRO(const std::string& Name, const std::string& Label, ETwType Type, const void* Variable, const std::string& Help, const std::string& Group, const std::string& Step, const std::string& Min, const std::string& Max, const std::string& Key, const std::string& KeyIncr, const std::string& KeyDesc);
		void MakeVarCB(const std::string& Name, const std::string& Label, ETwType Type, UISetVarCallback SetCallback, UIGetVarCallback GetCallback, void* ClientData, const std::string& Help, const std::string& Group, const std::string& Step, const std::string& Min, const std::string& Max, const std::string& Key, const std::string& KeyIncr, const std::string& KeyDesc);
};