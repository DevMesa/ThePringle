#include "NoRender.hpp"
#include <Windows.h>

using namespace Pringle;
using namespace Pringle::Hooks;
using namespace Modules;


NoRenderMod::NoRenderMod() : ModuleBase("Pringle")
{
	this->Enabled = this->AddVariableInt("NoRender.Enabled", "norender.enabled", "Disable screen rendering effects", eCommandFlagsArchived, 0);

	Hook::SubscribeMember<RenderEffectEvent>(this, &NoRenderMod::OnRenderEffect);
}

void NoRenderMod::OnRenderEffect(const Pringle::Hooks::RenderEffectEvent& event)
{
	if (this->Enabled->ValueInt)
		event.Canceled = true;
}