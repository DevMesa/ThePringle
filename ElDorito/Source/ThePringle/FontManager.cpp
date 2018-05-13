#include "FontManager.hpp"
#include "Hooks.hpp"

namespace Pringle
{
	FontManager::FontManager()
	{
		Hook::SubscribeMember<Hooks::DirectX::Initialize>(this, &FontManager::OnInitialize);
		Hook::SubscribeMember<Hooks::DirectX::PreReset>(this, &FontManager::OnPreReset);
		Hook::SubscribeMember<Hooks::DirectX::PostReset>(this, &FontManager::OnPostReset);

		this->font = this->default_font = nullptr; // do this or it will equal 0xCCCCCCCC
	}

	FontManager::~FontManager()
	{
		if (this->default_font) this->default_font->Release();
	}

	ID3DXFont* FontManager::GetFont()
	{
		return this->font == nullptr ? this->default_font : this->font;
	}

	void FontManager::SetFont(ID3DXFont * font)
	{
		this->font = font;
	}

	void FontManager::OnInitialize(const DirectX::Initialize & msg)
	{
		if (this->default_font)
		{
			this->default_font->Release();
			this->default_font = nullptr;
		}

		D3DXCreateFont(msg.Device, 16, 8, 0, 0, false, DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Verdana", &this->default_font);
	}

	void FontManager::OnPreReset(const Hooks::DirectX::PreReset& msg)
	{
		if (this->default_font) this->default_font->OnLostDevice();
		if (this->font) this->font->OnLostDevice();
	}

	void FontManager::OnPostReset(const Hooks::DirectX::PostReset& msg)
	{
		if (this->default_font) this->default_font->OnResetDevice();
		if (this->font) this->font->OnResetDevice();
	}
}
