#include "Draw.hpp"

namespace
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

	inline ID3DXFont* get_default_font(LPDIRECT3DDEVICE9 device)
	{
		static ID3DXFont* defaultFont = nullptr;
		if (!defaultFont) D3DXCreateFont(device, 16, 8, 0, 0, false, DEFAULT_CHARSET, OUT_CHARACTER_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, "Verdana", &defaultFont);
		return defaultFont;
	}
}

namespace Pringle
{
	Draw::Draw(LPDIRECT3DDEVICE9 _device) : device(_device) 
	{
		D3DVIEWPORT9 viewport;
		device->GetViewport(&viewport);
		screenWidth = static_cast<int>(viewport.Width);
		screenHeight = static_cast<int>(viewport.Height);
	}

	LPDIRECT3DDEVICE9 Draw::GetDevice() const
	{
		return this->device;
	}

	ID3DXFont* Draw::GetFont() const
	{
		return !this->font ? get_default_font(this->device) : this->font;
	}

	void Draw::SetFont(ID3DXFont* font)
	{
		this->font = font;
	}

	int Draw::GetScreenWidth()
	{
		return this->screenWidth;
	}

	int Draw::GetScreenHeight()
	{
		return this->screenHeight;
	}

	void Draw::Line(int sx, int sy, int ex, int ey, uint32_t color, bool antialias, int width)
	{
		LPD3DXLINE line;
		D3DXCreateLine(device, &line);
		D3DXVECTOR2 verts[] = 
		{
			D3DXVECTOR2(static_cast<float>(sx), static_cast<float>(sy)),
			D3DXVECTOR2(static_cast<float>(ex), static_cast<float>(ey))
		};

		line->SetAntialias(antialias);
		line->SetWidth(static_cast<float>(width));

		line->Begin();
		line->Draw(verts, _countof(verts), color);
		line->End();
		line->Release();
	}

	void Draw::Rect(int x, int y, int w, int h, uint32_t color)
	{
		this->Line(x + (w / 2), y, x + (w / 2), y + h, color, false, w);
	}

	void Draw::OutlinedRect(int x, int y, int w, int h, uint32_t color, int lineWidth)
	{
		if (lineWidth < 1) return;

		for (int i = 0; i < lineWidth; i++)
		{
			LPD3DXLINE line;
			D3DXCreateLine(device, &line);

			float px = static_cast<float>(x - i);
			float pxw = px + static_cast<float>(w + (i * 2));

			float py = static_cast<float>(y - i);
			float pyh = py + static_cast<float>(h + (i * 2));

			D3DXVECTOR2 verts[] =
			{
				D3DXVECTOR2(px, py),
				D3DXVECTOR2(pxw, py),
				D3DXVECTOR2(pxw, pyh),
				D3DXVECTOR2(px, pyh),
				D3DXVECTOR2(px, py)
			};

			line->SetWidth(1.f);

			line->Begin();
			line->Draw(verts, _countof(verts), color);
			line->End();
			line->Release();
		}
	}

	void Draw::Text(const char* text, int x, int y, uint32_t color, uint32_t alignment)
	{
		RECT rect;
		SetRect(&rect, x, y, x, y);

		GetFont()->DrawTextA(0, text, -1, &rect, DT_NOCLIP | alignment, color);
	}

	void Draw::Text(const wchar_t* text, int x, int y, uint32_t color, uint32_t alignment)
	{
		RECT rect;
		SetRect(&rect, x, y, x, y);

		GetFont()->DrawTextW(0, text, -1, &rect, DT_NOCLIP | alignment, color);
	}
	bool Draw::ToScreen(float x, float y, float
		
		
		
		z, int & screenX, int & screenY)
	{
		project_t projection;
		projection.should_project = true;
		projection.x = x;
		projection.y = y;
		projection.z = z;
		WorldToScreen(&projection);

		screenX = projection.projected_x;
		screenY = projection.projected_y;

		//return projection.projected_x2 > -1 && projection.projected_x2 <= screenWidth && projection.projected_y2 > -1 && projection.projected_y2 <= screenHeight;
		return true;
	}
}