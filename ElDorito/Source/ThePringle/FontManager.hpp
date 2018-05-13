#pragma once

#ifndef __PRINGLE_FONTMANAGER_
#define  __PRINGLE_FONTMANAGER_

#include "Hooks.hpp"
#include "../Utils/Singleton.hpp"

using namespace Pringle::Hooks;

namespace Pringle
{
	class FontManager 
		: public Utils::Singleton<FontManager>
	{
	public:
		FontManager();
		~FontManager();

		ID3DXFont* GetFont();
		void SetFont(ID3DXFont* font);
		
		void OnInitialize(const DirectX::Initialize& msg);
		void OnPreReset(const DirectX::PreReset& msg);
		void OnPostReset(const DirectX::PostReset& msg);

	private:
		ID3DXFont *font, *default_font;
	};
};

#endif // !__PRINGLE_FONTMANAGER_