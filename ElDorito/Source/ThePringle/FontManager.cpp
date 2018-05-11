#include "FontManager.hpp"
#include "Hooks.hpp"

namespace Pringle
{
	FontManager::FontManager()
	{
		Hook::SubscribeMember<Hooks::DirectX::PreReset>(this, &FontManager::OnPreReset);
		Hook::SubscribeMember<Hooks::DirectX::PostReset>(this, &FontManager::OnPostReset);
	}

	ID3DXFont* FontManager::GetFont(LPDIRECT3DDEVICE9 device)
	{
		if (!this->font) D3DXCreateFont(device, 16, 8, 0, 0, false, DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Verdana", &this->font);
		return this->font;
	}

	void FontManager::OnPreReset(const Hooks::DirectX::PreReset& msg)
	{
		if (this->font) this->font->OnLostDevice();
	}

	void FontManager::OnPostReset(const Hooks::DirectX::PostReset& msg)
	{
		if (this->font) this->font->OnResetDevice();
	}
}
