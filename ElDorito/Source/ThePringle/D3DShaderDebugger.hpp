#pragma once

#ifndef __D3D_SHADER_DEBUGGER_
#define __D3D_SHADER_DEBUGGER_

#define FAILURE -1

#include <string>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <map>

#include <stdint.h>

#include <d3d9.h>
#include <d3dx9shader.h>

#include <boost\filesystem.hpp>

#include "../ThirdParty/rapidjson/document.h"
#include "../ThirdParty/rapidjson/prettywriter.h"

#define ARRAY_LENGTH(a) sizeof(a)/sizeof(*a);

using namespace boost;
using namespace rapidjson;

namespace DirectXDebugger
{
	class Shaders
	{
	private:
		static const constexpr char* const D3D_Classes[] =
		{ 
			"D3DXPC_SCALAR",
			"D3DXPC_VECTOR",
			"D3DXPC_MATRIX_ROWS",
			"D3DXPC_MATRIX_COLUMNS",
			"D3DXPC_OBJECT",
			"D3DXPC_STRUCT" 
		};

		static const constexpr char* const D3D_Registers[] =
		{ 
			"D3DXRS_BOOL",
			"D3DXRS_INT4",
			"D3DXRS_FLOAT4",
			"D3DXRS_SAMPLER" 
		};

		static const constexpr char* const D3D_Types[] =
		{
			"D3DXPT_VOID",
			"D3DXPT_BOOL",
			"D3DXPT_INT",
			"D3DXPT_FLOAT",
			"D3DXPT_STRING",
			"D3DXPT_TEXTURE",
			"D3DXPT_TEXTURE1D",
			"D3DXPT_TEXTURE2D",
			"D3DXPT_TEXTURE3D",
			"D3DXPT_TEXTURECUBE",
			"D3DXPT_SAMPLER",
			"D3DXPT_SAMPLER1D",
			"D3DXPT_SAMPLER2D",
			"D3DXPT_SAMPLER3D",
			"D3DXPT_SAMPLERCUBE",
			"D3DXPT_PIXELSHADER",
			"D3DXPT_VERTEXSHADER",
			"D3DXPT_PIXELFRAGMENT",
			"D3DXPT_VERTEXFRAGMENT" 
		};

		static uint32_t crc32c(uint32_t crc, const unsigned char *buf, size_t len)
		{
			int k;

			crc = ~crc;
			while (len--) {
				crc ^= *buf++;
				for (k = 0; k < 8; k++)
					crc = crc & 1 ? (crc >> 1) ^ 0x82f63b78 : crc >> 1;
			}
			return ~crc;
		}

		static inline std::string getparentdir(const std::string& file)
		{
			uint32_t pos = file.find_last_of("/\\");
			return pos == std::string::npos ? "" : file.substr(0, pos);
		}

		static std::string getexepath()
		{
			static std::string dir = "";
			if (dir == "")
			{
				char result[MAX_PATH];
				dir = std::string(result, GetModuleFileName(NULL, result, MAX_PATH));
				dir = getparentdir(dir);
			}
			return dir;
		}

		static void string_replace_all(std::string& str, const std::string& what, const std::string& with)
		{
			uint32_t pos = 0;
			while ((pos = str.find(what, pos)) != std::string::npos)
			{
				str.replace(pos, what.size(), with);
				pos += with.size();
			}
		}

		static bool open_file(const std::string& dir, std::ofstream& stream, std::ios_base::openmode mode = std::ios_base::out)
		{
			auto parent = getparentdir(getexepath() + "\\" + dir);
			if (parent != "" && !filesystem::exists(parent))
				filesystem::create_directories(parent);

			stream.open(dir, mode);

			return stream.is_open();
		}

		static HRESULT BuildShaderJson(StringBuffer& strbuff, const DWORD* func, std::vector<uint32_t>* filter = nullptr)
		{
			if (!func)
				return FAILURE;

			ID3DXBuffer* pbuff;

			auto err = S_OK;
			if (FAILED(err = D3DXDisassembleShader(func, 0, 0, &pbuff)))
				return err;

			auto buffer = std::unique_ptr<ID3DXBuffer, void(*)(ID3DXBuffer*)>(pbuff, [](ID3DXBuffer* ptr) { ptr->Release(); }); // now i don't have to remember to release at every return
			auto code = static_cast<uint8_t*>(buffer->GetBufferPointer());

			// get crc32
			auto crc32 = crc32c(0, code, strlen(reinterpret_cast<const char*>(code)));

			// if there is a filter provided, check if it already contains the crc32 for this
			if (filter && std::find(filter->begin(), filter->end(), crc32) != filter->end())
				return FAILURE;

			// if the item doesn't exist in the filter, and one is present, place it in now to prevent duplicates
			if (filter)
				filter->emplace_back(crc32);

			// now look up the constant table data
			LPD3DXCONSTANTTABLE ptable;
			if (FAILED(err = D3DXGetShaderConstantTable(func, &ptable)))
				return err;

			auto ctable = std::unique_ptr<ID3DXConstantTable, void(*)(ID3DXConstantTable*)>(ptable, [](ID3DXConstantTable* ptr) { ptr->Release(); });

			D3DXCONSTANTTABLE_DESC tdesc;
			D3DXCONSTANT_DESC desc[32];

			if (FAILED(err = ctable->GetDesc(&tdesc)))
				return err;

			// create new document writer
			PrettyWriter<StringBuffer> writer(strbuff);
			writer.StartObject(); // : {

			//
			// crc
			//

			std::stringstream ss;
			ss << std::hex << std::setfill('0') << std::setw(sizeof(uint32_t) * 2) << crc32;
			auto scrc32 = ss.str();

			writer.String("crc32");
			writer.String(scrc32.c_str());

			//
			// constants
			//

			writer.String("constants"); // constants
			writer.StartArray(); // [
			{
				for (uint32_t i = 0; i < tdesc.Constants; ++i)
				{
					auto handle = ctable->GetConstant(0, i);

					if (!handle)
						continue;

					uint32_t count = 32;
					if (FAILED(ctable->GetConstantDesc(handle, desc, &count)))
						continue;
					
					writer.StartObject(); // {
					for (uint32_t j = 0; j < count; ++j)
					{
						auto at = desc[j];

						writer.String("Name");
						writer.String(at.Name);

						writer.String("RegisterSet");
						writer.String(D3D_Registers[at.RegisterSet]);

						writer.String("RegisterIndex");
						writer.Uint(at.RegisterIndex);

						writer.String("RegisterCount");
						writer.Uint(at.RegisterCount);

						writer.String("Class");
						writer.String(D3D_Classes[at.Class]);

						writer.String("Type");
						writer.String(D3D_Types[at.Type]);

						writer.String("Rows");
						writer.Uint(at.Rows);

						writer.String("Columns");
						writer.Uint(at.Columns);

						writer.String("Elements");
						writer.Uint(at.Elements);

						writer.String("StructMembers");
						writer.Uint(at.StructMembers);

						writer.String("Bytes");
						writer.Uint(at.Bytes);

						writer.String("DefaultValue");
						writer.Uint(reinterpret_cast<DWORD>(at.DefaultValue));

						/* Useless information
						writer.String("StartRegister");
						writer.Uint(i);

						writer.String("CurrentIndex");
						writer.Uint(j);*/
					}
					writer.EndObject(); // }
				}
			}
			writer.EndArray(); // ]

			// 
			// shader asm writer section
			// 

			writer.String("asm"); // asm
			writer.StartArray(); // [
			{
				auto scode = std::string(reinterpret_cast<const char*>(code));

				string_replace_all(scode, "\r", "\n");
				string_replace_all(scode, "\n\n", "\n");
				string_replace_all(scode, "\t", "   ");

				// write out each line
				auto start = 0U, end = scode.find('\n');
				while (end != std::string::npos)
				{
					std::string at = scode.substr(start, end - start);
					writer.String(at.c_str());
					start = end + 1;
					end = scode.find('\n', start);
				}
			}
			writer.EndArray(); // ]

			writer.EndObject(); // }

			return err; // will return S_OK if everything is good;
		}

	public:
		static HRESULT DumpShaderToJson(const std::string& to, const DWORD* func, std::vector<uint32_t>* filter = nullptr)
		{
			StringBuffer buffer;

			auto err = S_OK;
			if (FAILED(err = BuildShaderJson(buffer, func, filter)))
				return err;

			std::ofstream file;
			if (!open_file(to, file, std::ios::app))
				return FAILURE;

			file << std::string(buffer.GetString(), buffer.GetSize()) + '\n';

			file.close();

			return err;
		}

		static HRESULT DumpCurrentVertexShader(const std::string& dir, LPDIRECT3DDEVICE9 device, std::vector<uint32_t>* filter = nullptr)
		{
			LPDIRECT3DVERTEXSHADER9 shader;

			auto err = S_OK;
			if (FAILED(err = device->GetVertexShader(&shader)))
				return err;

			uint32_t size;
			shader->GetFunction(0, &size);

			uint8_t* data = new uint8_t[size];
			shader->GetFunction(data, &size);

			err = DumpShaderToJson(dir, reinterpret_cast<DWORD*>(data), filter);

			delete[] data;

			return err;
		}

		static HRESULT DumpCurrentPixelShader(const std::string& dir, LPDIRECT3DDEVICE9 device, std::vector<uint32_t>* filter = nullptr)
		{
			LPDIRECT3DPIXELSHADER9 shader;

			auto err = S_OK;
			if (FAILED(err = device->GetPixelShader(&shader)))
				return err;

			uint32_t size;
			shader->GetFunction(0, &size);

			uint8_t* data = new uint8_t[size];
			shader->GetFunction(data, &size);

			err = DumpShaderToJson(dir, reinterpret_cast<DWORD*>(data), filter);

			delete[] data;

			return err;
		}
	};

	class RenderState
	{
	private:
		static const constexpr char* const D3D_RENDERSTATETYPE[] =
		{
			nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
			"D3DRS_ZENABLE",
			"D3DRS_FILLMODE",
			"D3DRS_SHADEMODE",
			nullptr, nullptr, nullptr, nullptr,
			"D3DRS_ZWRITEENABLE",
			"D3DRS_ALPHATESTENABLE",
			"D3DRS_LASTPIXEL",
			nullptr, nullptr,
			"D3DRS_SRCBLEND",
			"D3DRS_DESTBLEND",
			nullptr,
			"D3DRS_CULLMODE",
			"D3DRS_ZFUNC",
			"D3DRS_ALPHAREF",
			"D3DRS_ALPHAFUNC",
			"D3DRS_DITHERENABLE",
			"D3DRS_ALPHABLENDENABLE",
			"D3DRS_FOGENABLE",
			"D3DRS_SPECULARENABLE",
			nullptr, nullptr, nullptr, nullptr,
			"D3DRS_FOGCOLOR",
			"D3DRS_FOGTABLEMODE",
			"D3DRS_FOGSTART",
			"D3DRS_FOGEND",
			"D3DRS_FOGDENSITY",
			nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
			"D3DRS_RANGEFOGENABLE",
			nullptr, nullptr, nullptr,
			"D3DRS_STENCILENABLE",
			"D3DRS_STENCILFAIL",
			"D3DRS_STENCILZFAIL",
			"D3DRS_STENCILPASS",
			"D3DRS_STENCILFUNC",
			"D3DRS_STENCILREF",
			"D3DRS_STENCILMASK",
			"D3DRS_STENCILWRITEMASK",
			"D3DRS_TEXTUREFACTOR",
			nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
			"D3DRS_WRAP0",
			"D3DRS_WRAP1",
			"D3DRS_WRAP2",
			"D3DRS_WRAP3",
			"D3DRS_WRAP4",
			"D3DRS_WRAP5",
			"D3DRS_WRAP6",
			"D3DRS_WRAP7",
			"D3DRS_CLIPPING",
			"D3DRS_LIGHTING",
			nullptr,
			"D3DRS_AMBIENT",
			"D3DRS_FOGVERTEXMODE",
			"D3DRS_COLORVERTEX",
			"D3DRS_LOCALVIEWER",
			"D3DRS_NORMALIZENORMALS",
			nullptr,
			"D3DRS_DIFFUSEMATERIALSOURCE",
			"D3DRS_SPECULARMATERIALSOURCE",
			"D3DRS_AMBIENTMATERIALSOURCE",
			"D3DRS_EMISSIVEMATERIALSOURCE",
			nullptr, nullptr,
			"D3DRS_VERTEXBLEND",
			"D3DRS_CLIPPLANEENABLE",
			nullptr,
			"D3DRS_POINTSIZE",
			"D3DRS_POINTSIZE_MIN",
			"D3DRS_POINTSPRITEENABLE",
			"D3DRS_POINTSCALEENABLE",
			"D3DRS_POINTSCALE_A",
			"D3DRS_POINTSCALE_B",
			"D3DRS_POINTSCALE_C",
			"D3DRS_MULTISAMPLEANTIALIAS",
			"D3DRS_MULTISAMPLEMASK",
			"D3DRS_PATCHEDGESTYLE",
			nullptr,
			"D3DRS_DEBUGMONITORTOKEN",
			"D3DRS_POINTSIZE_MAX",
			"D3DRS_INDEXEDVERTEXBLENDENABLE",
			"D3DRS_COLORWRITEENABLE",
			nullptr,
			"D3DRS_TWEENFACTOR",
			"D3DRS_BLENDOP",
			"D3DRS_POSITIONDEGREE",
			"D3DRS_NORMALDEGREE",
			"D3DRS_SCISSORTESTENABLE",
			"D3DRS_SLOPESCALEDEPTHBIAS",
			"D3DRS_ANTIALIASEDLINEENABLE",
			nullptr,
			"D3DRS_MINTESSELLATIONLEVEL",
			"D3DRS_MAXTESSELLATIONLEVEL",
			"D3DRS_ADAPTIVETESS_X",
			"D3DRS_ADAPTIVETESS_Y",
			"D3DRS_ADAPTIVETESS_Z",
			"D3DRS_ADAPTIVETESS_W",
			"D3DRS_ENABLEADAPTIVETESSELLATION",
			"D3DRS_TWOSIDEDSTENCILMODE",
			"D3DRS_CCW_STENCILFAIL",
			"D3DRS_CCW_STENCILZFAIL",
			"D3DRS_CCW_STENCILPASS",
			"D3DRS_CCW_STENCILFUNC",
			"D3DRS_COLORWRITEENABLE1",
			"D3DRS_COLORWRITEENABLE2",
			"D3DRS_COLORWRITEENABLE3",
			"D3DRS_BLENDFACTOR",
			"D3DRS_SRGBWRITEENABLE",
			"D3DRS_DEPTHBIAS",
			nullptr, nullptr,
			"D3DRS_WRAP8",
			"D3DRS_WRAP9",
			"D3DRS_WRAP10",
			"D3DRS_WRAP11",
			"D3DRS_WRAP12",
			"D3DRS_WRAP13",
			"D3DRS_WRAP14",
			"D3DRS_WRAP15",
			"D3DRS_SEPARATEALPHABLENDENABLE",
			"D3DRS_SRCBLENDALPHA",
			"D3DRS_DESTBLENDALPHA",
			"D3DRS_BLENDOPALPHA",
		};
		static const uint32_t COUNT_RENDERSTATES = 210;

		static const constexpr char* const D3D_SHADEMODE[] =
		{
			nullptr,
			"D3DSHADE_FLAT",
			"D3DSHADE_GOURAUD",
			"D3DSHADE_PHONG",
		};

		static const constexpr char* const D3D_FILLMODE[] =
		{
			nullptr,
			"D3DFILL_POINT",
			"D3DFILL_WIREFRAME",
			"D3DFILL_SOLID",
		};

		static const constexpr char* const D3D_BLEND[] =
		{
			nullptr,
			"D3DBLEND_ZERO",
			"D3DBLEND_ONE",
			"D3DBLEND_SRCCOLOR",
			"D3DBLEND_INVSRCCOLOR",
			"D3DBLEND_SRCALPHA",
			"D3DBLEND_INVSRCALPHA",
			"D3DBLEND_DESTALPHA",
			"D3DBLEND_INVDESTALPHA",
			"D3DBLEND_DESTCOLOR",
			"D3DBLEND_INVDESTCOLOR",
			"D3DBLEND_SRCALPHASAT",
			"D3DBLEND_BOTHSRCALPHA",
			"D3DBLEND_BOTHINVSRCALPHA",
			"D3DBLEND_BLENDFACTOR",
			"D3DBLEND_INVBLENDFACTOR",
			"D3DBLEND_SRCCOLOR2",
			"D3DBLEND_INVSRCCOLOR2",
		};

		static const constexpr char* const D3D_BLENDOP[] =
		{
			nullptr,
			"D3DBLENDOP_ADD",
			"D3DBLENDOP_SUBTRACT",
			"D3DBLENDOP_REVSUBTRACT",
			"D3DBLENDOP_MIN",
			"D3DBLENDOP_MAX",
		};

		static const constexpr char* const D3D_TEXTUREADDRESS[] =
		{
			nullptr,
			"D3DTADDRESS_WRAP",
			"D3DTADDRESS_MIRROR",
			"D3DTADDRESS_CLAMP",
			"D3DTADDRESS_BORDER",
			"D3DTADDRESS_MIRRORONCE",
		};

		static const constexpr char* const D3D_CULL[] =
		{
			nullptr,
			"D3DCULL_NONE",
			"D3DCULL_CW",
			"D3DCULL_CCW",
		};

		static const constexpr char* const D3D_CMPFUNC[] =
		{
			nullptr,
			"D3DCMP_NEVER",
			"D3DCMP_LESS",
			"D3DCMP_EQUAL",
			"D3DCMP_LESSEQUAL",
			"D3DCMP_GREATER",
			"D3DCMP_NOTEQUAL",
			"D3DCMP_GREATEREQUAL",
			"D3DCMP_ALWAYS",
		};

		static const constexpr char* const D3D_STENCILOP[] =
		{
			nullptr,
			"D3DSTENCILOP_KEEP",
			"D3DSTENCILOP_ZERO",
			"D3DSTENCILOP_REPLACE",
			"D3DSTENCILOP_INCRSAT",
			"D3DSTENCILOP_DECRSAT",
			"D3DSTENCILOP_INVERT",
			"D3DSTENCILOP_INCR",
			"D3DSTENCILOP_DECR",
		};

		static const constexpr char* const D3D_FOGMODE[] =
		{
			"D3DFOG_NONE",
			"D3DFOG_EXP",
			"D3DFOG_EXP2",
			"D3DFOG_LINEAR",
		};

		static const constexpr char* const D3D_ZBUFFERTYPE[] =
		{
			"D3DZB_FALSE",
			"D3DZB_TRUE",
			"D3DZB_USEW",
		};

		static const constexpr char* const D3D_PRIMITIVETYPE[] =
		{
			nullptr,
			"D3DPT_POINTLIST",
			"D3DPT_LINELIST",
			"D3DPT_LINESTRIP",
			"D3DPT_TRIANGLELIST",
			"D3DPT_TRIANGLESTRIP",
			"D3DPT_TRIANGLEFAN",
		};

		static const constexpr char* const D3D_TRANSFORMSTATETYPE[] =
		{
			nullptr, nullptr,
			"D3DTS_VIEW",
			"D3DTS_PROJECTION",
			nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
			"D3DTS_TEXTURE0",
			"D3DTS_TEXTURE1",
			"D3DTS_TEXTURE2",
			"D3DTS_TEXTURE3",
			"D3DTS_TEXTURE4",
			"D3DTS_TEXTURE5",
			"D3DTS_TEXTURE6",
			"D3DTS_TEXTURE7",
		};

		static inline const char* to_bool_string(DWORD value)
		{
			return value == 0 ? "false" : "true";
		}

		static inline const char* to_string(DWORD value)
		{
			std::stringstream ss;
			ss << std::hex << std::setfill('0') << std::setw(sizeof(DWORD) * 2) << value;
			return ss.str().c_str();
		}

		static const char* state_value_to_string(D3DRENDERSTATETYPE state, DWORD value)
		{
			if (state == 0x7fffffff || value == 0x7fffffff)
				return '\0';

			switch (state)
			{
			case D3DRS_ZENABLE:
				return D3D_ZBUFFERTYPE[value];
			case D3DRS_FILLMODE:
				return D3D_FILLMODE[value];
			case D3DRS_SHADEMODE:
				return D3D_SHADEMODE[value];
			case D3DRS_SRCBLEND:
			case D3DRS_DESTBLEND:
			case D3DRS_SRCBLENDALPHA:
			case D3DRS_DESTBLENDALPHA:
			case D3DRS_BLENDOPALPHA:
				return D3D_BLEND[value];
			case D3DRS_CULLMODE:
				return D3D_CULL[value];
			case D3DRS_ZFUNC:
			case D3DRS_ALPHAFUNC:
			case D3DRS_STENCILFUNC:
			case D3DRS_CCW_STENCILFUNC:
				return D3D_CMPFUNC[value];
			case D3DRS_FOGTABLEMODE:
				return D3D_FOGMODE[value];
			case D3DRS_STENCILFAIL:
			case D3DRS_STENCILZFAIL:
			case D3DRS_STENCILPASS:
			case D3DRS_CCW_STENCILFAIL:
			case D3DRS_CCW_STENCILZFAIL:
			case D3DRS_CCW_STENCILPASS:
				return D3D_STENCILOP[value];
			case D3DRS_BLENDOP:
				return D3D_BLENDOP[value];
			case D3DRS_ZWRITEENABLE:
			case D3DRS_ALPHATESTENABLE:
			case D3DRS_LASTPIXEL:
			case D3DRS_DITHERENABLE:
			case D3DRS_ALPHABLENDENABLE:
			case D3DRS_FOGENABLE:
			case D3DRS_SPECULARENABLE:
			case D3DRS_RANGEFOGENABLE:
			case D3DRS_STENCILENABLE:
			case D3DRS_CLIPPING:
			case D3DRS_LIGHTING:
			case D3DRS_POINTSPRITEENABLE:
			case D3DRS_POINTSCALEENABLE:
			case D3DRS_MULTISAMPLEANTIALIAS:
			case D3DRS_TWOSIDEDSTENCILMODE:
			case D3DRS_CLIPPLANEENABLE:
				return to_bool_string(value);
			}
			return to_string(value); // fucks up for some reason
		}

	public:
		static void GatherRenderStates(LPDIRECT3DDEVICE9 device, std::map<std::string, std::string>& details) // TODO: finish
		{
			details.clear(); // remove all previous data

			for (uint32_t i = 0; i < COUNT_RENDERSTATES; ++i)
			{
				auto name = D3D_RENDERSTATETYPE[i];

				if (name)
				{
					auto state = static_cast<D3DRENDERSTATETYPE>(i);

					DWORD value;
					if (FAILED(device->GetRenderState(state, &value)))
						continue;

					auto strv = state_value_to_string(state, value);
					details.insert({ name, strv ? strv : "null" });
				}
			}
		}
	};
}

#endif // !__D3D_SHADER_DEBUGGER_
