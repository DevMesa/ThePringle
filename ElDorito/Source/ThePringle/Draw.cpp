#include "Draw.hpp"
#include "FontManager.hpp"

namespace
{
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

	typedef void(__thiscall* WorldToScreen_t)(project_t*, float);
	auto WorldToScreen_ptr = reinterpret_cast<WorldToScreen_t>(0xAD2360);

	void WorldToScreen(project_t *project, float clamp)
	{
		static const uint32_t current_view_addr = 0x50DEDF0;

		uint8_t** view_ptr = (uint8_t**)current_view_addr;
		*view_ptr = (uint8_t*)0x050DEE10;

		WorldToScreen_ptr(project, clamp); // clamp <= 0.f will not clamp projection
	}
}

namespace Pringle
{
	Draw::Draw(LPDIRECT3DDEVICE9 _device) : device(_device) 
	{
		D3DVIEWPORT9 viewport;
		this->device->GetViewport(&viewport);
		this->screenWidth = static_cast<int>(viewport.Width);
		this->screenHeight = static_cast<int>(viewport.Height);
		this->state = nullptr;
	}

	Draw::~Draw()
	{
		this->ReleaseState();
	}

	LPDIRECT3DDEVICE9 Draw::GetDevice() const
	{
		return this->device;
	}

	ID3DXFont* Draw::GetFont() const
	{
		return !this->font ? FontManager::Instance().GetFont(device) : this->font;
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

	void Draw::CaptureState()
	{
		if (this->state != nullptr)
			return;

		device->CreateStateBlock(D3DSBT_ALL, &this->state);
		this->state->Capture();
	}

	void Draw::ReleaseState()
	{
		if (this->state != nullptr)
		{
			this->state->Apply();
			this->state->Release();
			this->state = nullptr;
		}
	}

	void Draw::Line(int sx, int sy, int ex, int ey, uint32_t color)
	{
		DrawVertex verts[] = 
		{
			DrawVertex(static_cast<float>(sx), static_cast<float>(sy), color),
			DrawVertex(static_cast<float>(ex), static_cast<float>(ey), color)
		};

		device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);

		device->SetTexture(0, 0);
		device->SetVertexShader(0);
		device->SetPixelShader(0);

		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);

		device->DrawPrimitiveUP(D3DPT_LINELIST, 1, &verts, sizeof(DrawVertex));
	}

	void Draw::Rect(int x, int y, int w, int h, uint32_t color)
	{
		DrawVertex verts[] =
		{
			// left
			DrawVertex(static_cast<float>(x), static_cast<float>(y), color),
			DrawVertex(static_cast<float>(x), static_cast<float>(y + h), color),
			DrawVertex(static_cast<float>(x + w), static_cast<float>(y + h), color),
			// right
			DrawVertex(static_cast<float>(x + w), static_cast<float>(y + h), color),
			DrawVertex(static_cast<float>(x + w), static_cast<float>(y), color),
			DrawVertex(static_cast<float>(x), static_cast<float>(y), color)
		};

		device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);

		device->SetTexture(0, 0);
		device->SetVertexShader(0);
		device->SetPixelShader(0);

		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);

		device->DrawPrimitiveUP(D3DPT_TRIANGLELIST, 2, &verts, sizeof(DrawVertex));
	}

	void Draw::OutlinedRect(int x, int y, int w, int h, uint32_t color)
	{
		float fx = static_cast<float>(x); 
		float fy = static_cast<float>(y);
		float fw = static_cast<float>(w);
		float fh = static_cast<float>(h);

		DrawVertex verts[] =
		{
			DrawVertex(fx, fy, color),
			DrawVertex(fx + fw, fy, color),
			DrawVertex(fx + fw, fy + fh, color),
			DrawVertex(fx, fy + fh, color),
			DrawVertex(fx, fy, color)
		};

		device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);

		device->SetTexture(0, 0);
		device->SetVertexShader(0);
		device->SetPixelShader(0);

		device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
		device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		device->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);

		device->DrawPrimitiveUP(D3DPT_LINESTRIP, 4, &verts, sizeof(DrawVertex));
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

	bool Draw::ToScreen(float x, float y, float z, int & screenX, int & screenY, float clamp)
	{
		project_t projection;
		projection.should_project = true;
		projection.x = x;
		projection.y = y;
		projection.z = z;
		WorldToScreen(&projection, clamp);

		screenX = projection.projected_x;
		screenY = projection.projected_y;

		return projection.projected_x > -1 && projection.projected_x <= screenWidth && projection.projected_y > -1 && projection.projected_y <= screenHeight;
	}
}