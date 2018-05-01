#include "Hooks.hpp"
#include "../Modules/ModuleBase.hpp"

#include "../Blam/BlamPlayers.hpp"
#include "../Patch.hpp"

//#define __PRINGLE_MARKERS__
#ifdef __PRINGLE_MARKERS__
uint32_t marker_vis_hook(uint8_t* thisptr) 
{
	bool visibility = false;
	Pringle::Hook::Call<Pringle::Hooks::ModifyMarkerVisibility>(visibility);

	if (visibility)
	{
		uint32_t saved_ebx;
		uint32_t saved_edi;

		__asm
		{
			mov saved_ebx, ebx
			mov saved_edi, edi //TODO: this might be player index
		}

		return saved_ebx;
	}

	return *(uint32_t*)(thisptr + 8);
}

using namespace Pringle::Hooks;
using namespace Modules;

class Markers :
	public ModuleBase
{
public:
	Markers() :
		ModuleBase("pringle")
	{
		Enabled = this->AddVariableInt("espmarkers", "espmarkers", "ESP Markers", eCommandFlagsNone, 1);

		//.text:0074A6EE E8 2D D1 DE FF                          call    sub_537820
		//.text : 0074A6F3 3B C3                                 cmp     eax, ebx
		//.text : 0074A6F5 75 3D                                 jnz     short loc_74A734
		Hook(0x34A6EE, marker_vis_hook, HookFlags::IsCall).Apply();

		Pringle::Hook::SubscribeMember<ModifyMarkerVisibility>(this, &Markers::OnModifyMarkerVisibility);
	}

protected:
	Command * Enabled;

	void OnModifyMarkerVisibility(const ModifyMarkerVisibility& msg) 
	{
		msg.Visibility = Enabled->ValueInt > 0;
	}
};

static Markers MarkersInstance;
#endif