#include "ESP.hpp"
#include "../ElDorito.hpp"
#include "../Pointer.hpp"
#include "../Patches/Core.hpp"
#include "../Blam/BlamData.hpp"
#include "../Blam/BlamPlayers.hpp"
#include "../Blam/BlamObjects.hpp"
#include "../Blam/Math/RealVector3D.hpp"
#include "../Forge/ForgeUtil.hpp"
#include "../Console.hpp"
#include "../Modules/ModuleSettings.hpp"

#include "Vector.hpp"
#include "QAngle.hpp"

#include <d3dx9math.h>

using namespace Pringle;
using namespace Pringle::Hooks;
using namespace Modules;
using namespace Blam;

namespace Halo
{
	class rect_t
	{
	public:
		uint16_t draw_start_x; //0x0000
		uint16_t draw_start_y; //0x0002
		uint16_t draw_end_x; //0x0004
		uint16_t draw_end_y; //0x0006
	}; //Size: 0x0008

	class color_t
	{
	public:
		float r; //0x0000
		float g; //0x0004
		float b; //0x0008
	}; //Size: 0x000C

	class project_t
	{
	public:
		uint8_t should_project; //0x0000
		char pad_0001[3]; //0x0001
		float x; //0x0004
		float y; //0x0008
		float z; //0x000C
		float projected_x; //0x0010
		float projected_y; //0x0014
		float projected_x2; //0x0018
		float projected_y2; //0x001C
		char pad_0020[16]; //0x0020
	}; //Size: 0x0030

	void WorldToScreen(project_t *project)
	{

		static const uint32_t current_view_addr = 0x50DEDF0;
		static const uint32_t world2screen_addr = 0xAD2360;

		uint8_t** view_ptr = (uint8_t**)current_view_addr;
		*view_ptr = (uint8_t*)0x050DEE10;

		typedef void(__thiscall *world2screen_func)(project_t*, float);
		world2screen_func world2screen = reinterpret_cast<world2screen_func>(world2screen_addr);
		world2screen(project, 1.0f);
	}

	class c_font_cache_mt_safe
	{
	private:
		static const uint32_t constructor_addr = 0x659650;
		static const uint32_t deconstructor_addr = 0x659720;

	public:
		c_font_cache_mt_safe()
		{
			typedef c_font_cache_mt_safe* (__thiscall *c_font_cache_mt_safe_constructor)(c_font_cache_mt_safe*);
			c_font_cache_mt_safe_constructor constructor = reinterpret_cast<c_font_cache_mt_safe_constructor>(constructor_addr);
			constructor(this);
		};

		~c_font_cache_mt_safe()
		{
			typedef void(__thiscall *c_font_cache_mt_safe_deconstructor)(c_font_cache_mt_safe*);
			c_font_cache_mt_safe_deconstructor deconstructor = reinterpret_cast<c_font_cache_mt_safe_deconstructor>(deconstructor_addr);
			deconstructor(this);
		};

	private:
		uint8_t junk[0x4000]; //TODO
	};

	class c_rasterizer_draw_string 
	{
	private:
		static const uint32_t constructor_addr = 0xA25F00;
		static const uint32_t deconstructor_addr = 0x6571D0;
		static const uint32_t set_draw_area_addr = 0x658d20;
		static const uint32_t set_color_addr = 0x658df0;
		static const uint32_t set_style_addr = 0x659070;
		static const uint32_t draw_addr = 0xA26270;

	public:
		c_rasterizer_draw_string()
		{
			typedef c_rasterizer_draw_string* (__thiscall *c_rasterizer_draw_string_constructor)(c_rasterizer_draw_string*);
			auto constructor = reinterpret_cast<c_rasterizer_draw_string_constructor>(constructor_addr);
			constructor(this);
		};

		~c_rasterizer_draw_string()
		{
			typedef void (__thiscall *c_rasterizer_draw_string_deconstructor)(c_rasterizer_draw_string*);
			auto deconstructor = reinterpret_cast<c_rasterizer_draw_string_deconstructor>(deconstructor_addr);
			deconstructor(this);
		};

		void set_draw_area(rect_t* rect)
		{
			typedef void(__thiscall *c_raster_draw_string__set_draw_area)(c_rasterizer_draw_string*, rect_t*);
			auto string_set_draw_area = reinterpret_cast<c_raster_draw_string__set_draw_area>(set_draw_area_addr);
			string_set_draw_area(this, rect);
		}

		void set_color(color_t* color)
		{
			typedef void(__thiscall *c_raster_draw_string__set_color)(c_rasterizer_draw_string*, color_t*);
			auto string_set_color = reinterpret_cast<c_raster_draw_string__set_color>(set_color_addr);
			string_set_color(this, color);
		}

		void set_style(uint32_t style)
		{
			typedef void(__thiscall *c_raster_draw_string__set_style)(c_rasterizer_draw_string*, uint32_t);
			auto string_set_style = reinterpret_cast<c_raster_draw_string__set_style>(set_style_addr);
			string_set_style(this, style);
		}

		void draw()
		{
			typedef void(__thiscall *c_raster_draw_string__draw)(c_rasterizer_draw_string*);
			auto string_draw = reinterpret_cast<c_raster_draw_string__draw>(draw_addr);
			string_draw(this);
		}
	
	private:
		uint8_t junk[0x4000]; //TODO
	};
}

namespace Pringle
{
	static bool isMainMenu = true;

	const static auto Unit_GetHeadPosition = (void(__cdecl*)(uint32_t unitObjectIndex, Math::RealVector3D *position))(0x00B439D0);

	ESP::ESP() : ModuleBase("pringle")
	{
		Enabled = this->AddVariableInt("esp.enabled", "esp.enabled", "Enables ESP", eCommandFlagsArchived, 0);

		Hook::SubscribeMember<DirectX::EndScene>(this, &ESP::OnEndScene);
		Hook::SubscribeMember<Hooks::PreTick>(this, &ESP::OnPreTick);

		Patches::Core::OnMapLoaded([](auto map) {
			isMainMenu = !(std::string(map).find("mainmenu") == std::string::npos);
		});
	}

	static void DrawRect(LPDIRECT3DDEVICE9 device, int x, int y, int w, int h, D3DCOLOR color)
	{
		D3DRECT rect;
		rect.x1 = x;
		rect.y1 = y;
		rect.x2 = x + w;
		rect.y2 = y + h;

		device->Clear(1, &rect, D3DCLEAR_TARGET, color, 0, 0);
	}

	static void GetCameraPosition(D3DXVECTOR3* pos)
	{
		Pointer directorGlobalsPtr(ElDorito::GetMainTls(GameGlobals::Director::TLSOffset)[0]);

		pos->x = directorGlobalsPtr(0x834).Read<float>();
		pos->y = directorGlobalsPtr(0x838).Read<float>();
		pos->z = directorGlobalsPtr(0x83C).Read<float>();
	}

	static void GetCameraUp(D3DXVECTOR3* up)
	{
		Pointer directorGlobalsPtr(ElDorito::GetMainTls(GameGlobals::Director::TLSOffset)[0]);

		up->x = directorGlobalsPtr(0x868).Read<float>();
		up->y = directorGlobalsPtr(0x86C).Read<float>();
		up->z = directorGlobalsPtr(0x870).Read<float>();
	}

	static void GetLocalPlayerViewAngles(D3DXVECTOR3* angs)
	{
		Pointer playerControlGlobalsPtr(ElDorito::GetMainTls(GameGlobals::Input::TLSOffset)[0]);

		angs->x = playerControlGlobalsPtr(0x30C).Read<float>();
		angs->y = playerControlGlobalsPtr(0x310).Read<float>();
		angs->z = playerControlGlobalsPtr(0x314).Read<float>();
	}

	static uint32_t GetLocalPlayerHandle()
	{
		auto handle = Players::GetLocalPlayer(0); // get local player handle
		if (!handle || handle == DatumHandle::Null)
			return DatumHandle::Null;

		return handle.Handle;
	}

	static void GetLocalPlayerHeadPosition(D3DXVECTOR3* headPos)
	{
		auto lph = Players::GetLocalPlayer(0); // get local player handle
		if (!lph || lph == DatumHandle::Null)
			return;

		auto pl = Players::GetPlayers().Get(lph); // get player handle
		if (!pl)
			return;

		auto uh = pl->SlaveUnit.Handle;
		if (uh == -1)
			return;

		auto unit = Objects::Get(uh);
		if (!unit)
			return;

		Math::RealVector3D pos;
		Unit_GetHeadPosition(pl->SlaveUnit, &pos);

		headPos->x = pos.I;
		headPos->y = pos.J;
		headPos->z = pos.K;
	}

	void ToScreen(Vector& pos, Vector& out)
	{
		Halo::project_t project;
		project.x = pos.X;
		project.y = pos.Y;
		project.z = pos.Z;
		Halo::WorldToScreen(&project);
		out.X = project.projected_x;
		out.Y = project.projected_y;
	}

	void ESP::Draw(const DirectX::EndScene & msg, Blam::Objects::ObjectBase* unit, uint32_t color)
	{
		auto& players = Players::GetPlayers();
		auto& localPlayerIndex = Players::GetLocalPlayer(0);
		auto localPlayer = players.Get(localPlayerIndex);
		
		Vector pos = unit->Position, screen;
		ToScreen(pos, screen);

		DrawRect(msg.Device, screen.X, screen.Y, 5, 5, color);
	}

	void ESP::DrawPlayers(const DirectX::EndScene & msg)
	{
		for (auto it = ally_player_units.begin(); it != ally_player_units.end(); ++it) {
			auto unit = *it;
			uint32_t color = D3DCOLOR_RGBA(100, 100, 255, 255);
			this->Draw(msg, unit, color);
		}

		for (auto it = enemy_player_units.begin(); it != enemy_player_units.end(); ++it) {
			auto unit = *it;
			uint32_t color = D3DCOLOR_RGBA(255, 100, 100, 255);
			this->Draw(msg, unit, color);
		}
	}

	void ESP::OnEndScene(const DirectX::EndScene & msg)
	{
		if (isMainMenu || this->Enabled->ValueInt == 0)
			return;

		int width, height;
		ModuleSettings::Instance().GetScreenResolution(&width, &height);
		float fwidth = (float)width, fheight = (float)height;

		this->DrawPlayers(msg);
	}

	void ESP::OnPreTick(const Hooks::PreTick& msg)
	{
		ally_player_units.clear();
		enemy_player_units.clear();

		auto& players = Players::GetPlayers();
		auto& localPlayerIndex = Players::GetLocalPlayer(0);
		auto localPlayer = players.Get(localPlayerIndex);

		if (localPlayer == nullptr)
			return;

		for (auto it = players.begin(); it != players.end(); ++it) {
			const auto& unitObjectIndex = it->SlaveUnit.Handle;
			if (unitObjectIndex == -1)
				continue;

			if (localPlayer->SlaveUnit.Handle == unitObjectIndex)
				continue;

			const auto& unit = Objects::Get(unitObjectIndex);
			if (!unit)
				continue;

			if (it->DeadSlaveUnit)
				continue;

			if (it->Properties.TeamIndex != localPlayer->Properties.TeamIndex)
				enemy_player_units.push_back(unit);
			else
				ally_player_units.push_back(unit);
		}
	}
}
