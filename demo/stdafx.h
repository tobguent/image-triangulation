#pragma once

#include <Windows.h>


#include <dxgi.h>
#include <d3d11.h>
#include <d3d10.h>
#pragma warning( push )
#pragma warning( disable: 4838 )
#include <effect-framework/xnamath.h>
#include <effect-framework/D3DX11.h>
#include <effect-framework/d3dx11effect.h>
#pragma warning( pop )
#pragma warning (push)
#pragma warning (disable: 4996)
#include <string>
#pragma warning (pop)
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <list>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include "math.hpp"
#include <random>

#ifndef SAFE_DELETE
	#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#endif

#ifndef SAFE_RELEASE
	#define SAFE_RELEASE(p)       { if(p) { (p)->Release();     (p)=NULL; } }
#endif

#ifndef SAFE_DELETE_ARRAY
	#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#endif

inline bool IsKeyDown(char key)
{
	return GetAsyncKeyState(key) < 0;
}

// Checks whether a key is pressed.
inline bool IsKeyPressed(char key)
{
    static bool keys[256];  // One flag for each key... local scope but global lifetime!
    bool currState = IsKeyDown(key);        // get current state.
    if (currState && !keys[key])    // compare to last state -> is pressed?
    {
            keys[key] = currState;  // memorize state for next time
            return true;                    // report: yes!
    }
    keys[key] = currState;          // memorize state for next time
    return false;                           // report: no!
}