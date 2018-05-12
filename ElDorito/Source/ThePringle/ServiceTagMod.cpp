#include "ServiceTagMod.hpp"
#include "../Modules/ModulePlayer.hpp"
#include "../Patches/PlayerRepresentation.hpp"
#include <sstream>
#include <algorithm> 

using namespace Pringle;
using namespace Pringle::Hooks;
using namespace Modules;

std::vector<std::string>& Tags = *new std::vector<std::string>();

void FilterArg(std::string& input)
{
	std::transform(input.begin(), input.end(), input.begin(), ::toupper);
	if (input.length() > 4) {
		input.resize(4);
	}
}

bool AcceptArgs(const std::vector<std::string>& Arguments, std::string& returnInfo) 
{
	Tags = Arguments;
	std::for_each(Tags.begin(), Tags.end(), FilterArg);
	std::stringstream ss;
	ss << "Set Tag List (" << Arguments.size() << ")";
	returnInfo = ss.str();
	return true;
}

ServiceTagMod::ServiceTagMod() : ModuleBase("Pringle")
{
	this->Enabled = this->AddVariableInt("TagMod.Enabled", "tagmod.enabled", "Enable the hack", eCommandFlagsArchived, 0);
	this->Delay = this->AddVariableInt("TagMod.Delay", "tagmod.delay", "How fast to change", eCommandFlagsArchived, 100);
	this->Text = this->AddCommand("TagMod.Tags", "tagmod.tags", "Set the list of tags", eCommandFlagsArchived, AcceptArgs);

	Hook::SubscribeMember<PostTick>(this, &ServiceTagMod::OnPostTick);
}


size_t ticks = 0, state = 0;

std::string GetCurrentTag()
{
	if (state >= Tags.size())
		state = 0;

	std::string tag = Tags[state];

	state++;
	return tag;
}

void ServiceTagMod::OnPostTick(const PostTick & msg)
{
	if (this->Enabled->ValueInt != 0)
	{
		if (Tags.size() == 0) return;
		const int delay = Delay->ValueInt;
		if (delay > 0 && ticks % delay == 0)
		{
			auto &modulePlayer = Modules::ModulePlayer::Instance();
			modulePlayer.VarPlayerServiceTag->ValueString = GetCurrentTag();

			Patches::PlayerRepresentation::UpdateLocalRepresentation();
		}

		ticks++;
	}
}
