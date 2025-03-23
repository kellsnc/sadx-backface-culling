#include "pch.h"
#include <d3d8.h>
#include "SADXModLoader.h"
#include "FastFunctionHook.hpp"
#include "polybuff.h"

DataPointer(IDirect3DDevice8*, _st_d3d_device_, 0x03D128B0);
DataPointer(int, _nj_basic_cull_, 0x3D08490); 

FastThiscallHook<void, NJS_MATERIAL*> _njSetMaterial_h(0x784850);

// Parse the doubled sided flag
void _njSetMaterial_r(NJS_MATERIAL* material)
{
	_njSetMaterial_h.Original(material);

	int attrflags = material->attrflags;

	if (_nj_control_3d_flag_ & NJD_CONTROL_3D_CONSTANT_ATTR)
	{
		attrflags = _nj_constant_attr_or_ | _nj_constant_attr_and_ & attrflags;
	}

	if (attrflags & NJD_FLAG_DOUBLE_SIDE)
	{
		_nj_basic_cull_ = D3DCULL_NONE;
	}
	else
	{
		_nj_basic_cull_ = D3DCULL_CW;
	}

	// The game does not set the culling mode render state for static rendering so we do it here
	IDirect3DDevice8_SetRenderState(_st_d3d_device_, D3DRS_CULLMODE, _nj_basic_cull_);
}

extern "C"
{
	__declspec(dllexport) void __cdecl Init(const char* path, const HelperFunctions& helperFunctions)
	{
		_njSetMaterial_h.Hook(_njSetMaterial_r);
		WriteData<6>((void*)0x43954B, 0x90); // Prevent the game from forcing the double sided flag arbitrarly.
		WriteData<6>((void*)0x408819, 0x90); // Prevent the game from forcing the double sided flag for late rendering

		PatchPolybuff(); // Patch quad and strips parser to have correct winding order
	}

	__declspec(dllexport) ModInfo SADXModInfo = { ModLoaderVer };
}