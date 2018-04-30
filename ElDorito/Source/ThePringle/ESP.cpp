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
		//uint8_t* old_view = *view_ptr;
		*view_ptr = (uint8_t*)0x050DEE10;

		typedef void(__thiscall *world2screen_func)(project_t*, float);
		world2screen_func world2screen = reinterpret_cast<world2screen_func>(world2screen_addr);
		world2screen(project, 1.0f);

		//*view_ptr = old_view;
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

		/*uint32_t vtable; //0x0000
		char pad_0004[4]; //0x0004
		uint32_t style_id; //0x0008 text style id, can be 0..10
		uint32_t N00000004; //0x000C
		uint32_t N00000005; //0x0010
		uint32_t N00000006; //0x0014
		char pad_0018[4]; //0x0018
		float color_r; //0x001C
		float color_g; //0x0020
		float color_b; //0x0024
		float N0000000B; //0x0028
		float N0000000C; //0x002C
		char pad_0030[12]; //0x0030
		float N00000010; //0x003C
		float N00000011; //0x0040
		char pad_0044[36]; //0x0044
		float N0000001B; //0x0068
		float N00000024; //0x006C
		float N0000001C; //0x0070
		float N00000027; //0x0074
		char pad_0078[16]; //0x0078*/
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

	static void GetLocalPlayerUp(D3DXVECTOR3* up)
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

		up->x = unit->Up.I;
		up->y = unit->Up.J;
		up->z = unit->Up.K;
	}

	static void VecTransformCoordinate(D3DXVECTOR4* out, const D3DXVECTOR3* target, const D3DXMATRIX* matrix)
	{
		out->x = (target->x * matrix->m[0][0]) + (target->y * matrix->m[1][0]) + (target->z * matrix->m[2][0]) + matrix->m[3][0];
		out->y = (target->x * matrix->m[0][1]) + (target->y * matrix->m[1][1]) + (target->z * matrix->m[2][1]) + matrix->m[3][1];
		out->z = (target->x * matrix->m[0][2]) + (target->y * matrix->m[1][2]) + (target->z * matrix->m[2][2]) + matrix->m[3][2];
		out->w = (target->x * matrix->m[0][3]) + (target->y * matrix->m[1][3]) + (target->z * matrix->m[2][3]) + matrix->m[3][3];
	}

	static void ToScreen(LPDIRECT3DDEVICE9 device, const D3DXVECTOR3& target, D3DXVECTOR2& screen, bool& visible)
	{
		D3DXVECTOR4 output;

		D3DVIEWPORT9 viewport;

		device->GetViewport(&viewport);

		const float fwidth = (float)viewport.Width;
		const float fheight = (float)viewport.Height;

		D3DXMATRIX view, proj, world, wvm;

		D3DXVECTOR3 head, cam, up, angs;
		GetLocalPlayerHeadPosition(&head);
		GetCameraPosition(&cam);
		GetCameraUp(&up);
		GetLocalPlayerViewAngles(&angs);

		D3DXMatrixLookAtLH(&view, &cam, &angs, &D3DXVECTOR3(0, 0, 1));
		D3DXMatrixPerspectiveFovLH(&proj, (float)(D3DX_PI / 4.f), fwidth / fheight, 1.f, 1000.f);

		wvm = view * proj;

		VecTransformCoordinate(&output, &target, &wvm);

		output.x /= output.w;
		output.y /= output.w;

		output.x = fwidth * (output.x + 1.f) / 2.f;
		output.y = fheight * (1.f - ((output.y + 1.f) / 2.f));

		screen.x = output.x;
		screen.y = output.y;

		visible = !(output.x < 0 || output.y < 0 || output.x > fwidth || output.y > fheight);
	}

	static void ToScreenAshleigh(LPDIRECT3DDEVICE9 device, const float x, const float y, const float z, float& screenX, float& screenY, bool& visible)
	{
		D3DVIEWPORT9 viewport;

		device->GetViewport(&viewport);

		D3DXVECTOR3 head, cam, up, angs;
		GetLocalPlayerHeadPosition(&head);
		GetCameraPosition(&cam);
		GetCameraUp(&up);
		GetLocalPlayerViewAngles(&angs);

		const float width = (float)viewport.Width;
		const float height = (float)viewport.Height;

		Angle yaw = Angle::Radians(-angs.x * 0.5f) + 135_deg;
		Angle pitch = Angle::Radians(-angs.y * 0.5f);

		Vector viewpos = cam;
		QAngle viewang(pitch, yaw, Angle::Radians(0));
		Vector pos = Vector(x, y, z);

		visible = pos.ToScreen(viewpos, viewang, 100_deg, width, height, screenX, screenY);
	}

	static void ToScreen(LPDIRECT3DDEVICE9 device, const float x, const float y, const float z, float& screenX, float& screenY, bool& visible)

	{
		D3DXVECTOR2 d3dscreen;
		D3DXVECTOR3 d3dtarget = D3DXVECTOR3(x, y, z);


		ToScreen(device, d3dtarget, d3dscreen, visible);

		screenX = d3dscreen.x;
		screenY = d3dscreen.y;
	}

	void ESP::Draw(const DirectX::EndScene & msg, Blam::Objects::ObjectBase* unit, uint32_t color)
	{
		auto& players = Players::GetPlayers();
		auto& localPlayerIndex = Players::GetLocalPlayer(0);
		auto localPlayer = players.Get(localPlayerIndex);
		
		float screenX = .0f, screenY = .0f;
		{
			auto pos = unit->Position;
			Halo::project_t project;
			project.x = pos.I;
			project.y = pos.J;
			project.z = pos.K;
			Halo::WorldToScreen(&project);
			screenX = project.projected_x;
			screenY = project.projected_y;
		}

		{
			DrawRect(msg.Device, screenX, screenY, 5, 5, color);

			{
				static Halo::color_t white = { 1.0f, 1.0f, 1.0f };
				Halo::rect_t area = { screenX, screenY, screenX + 100, screenY + 100 };
				//Halo::c_rasterizer_draw_string draw_string;
				//Halo::c_font_cache_mt_safe font_cache;

				//draw_string.set_color(&white);
				//draw_string.set_style(0);
				//draw_string.set_draw_area(&area);
				//draw_string.draw();
			}
		}
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
		if (isMainMenu)
			return;

		int width, height;
		ModuleSettings::Instance().GetScreenResolution(&width, &height);
		float fwidth = (float)width, fheight = (float)height;

		this->DrawPlayers(msg);
	}

	void ESP::OnPreTick(const Hooks::PreTick & msg)
	{
		{
			ally_player_units.clear();
			enemy_player_units.clear();

			auto& players = Players::GetPlayers();
			auto& localPlayerIndex = Players::GetLocalPlayer(0);
			auto localPlayer = players.Get(localPlayerIndex);

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
}
