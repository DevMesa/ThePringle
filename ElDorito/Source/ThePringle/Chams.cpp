#include "Chams.hpp"

namespace Pringle
{
	Chams::Chams() : Modules::ModuleBase("pringle")
	{
		Upper = this->AddVariableInt("chams.upper", "chams.upper", "debug", eCommandFlagsArchived, 0);
		Lower = this->AddVariableInt("chams.lower", "chams.lower", "debug", eCommandFlagsArchived, 0);
		Only = this->AddVariableInt("chams.only", "chams.only", "debug", eCommandFlagsArchived, 0);
	}
}