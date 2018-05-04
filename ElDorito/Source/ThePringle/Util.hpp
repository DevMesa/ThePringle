#pragma once

#ifndef PRINGLE_UTILH
#define PRINGLE_UTILH

#include <sstream>
#include <string>
#include <iostream>

namespace Pringle
{
	struct Util
	{
	public:
		static void StringReplace(std::string& str, const std::string& from, const std::string& to) 
		{
			if (from.empty())
				return;
			size_t start_pos = 0;
			while ((start_pos = str.find(from, start_pos)) != std::string::npos) 
			{
				str.replace(start_pos, from.length(), to);
				start_pos += to.length(); // In case 'to' contains 'from', like replacing 'x' with 'yx'
			}
		}

		template<typename... Args>
		static std::string Format(const std::string& format, const Args&... args)
		{
			std::string ret = format;
			FormatInternal(ret, 0, args...);
			return ret;
		}

		template<typename... Args>
		static void WriteLine(const std::string& format, const Args&... args)
		{
			std::cout << Format(format, args...) << std::endl;
		}

	private:
		template<typename T, typename... Args>
		static void FormatInternal(std::string& output, size_t n, const T& first, Args&... rest)
		{
			std::stringstream n_ss;
			n_ss  << '{' << n << '}';

			std::stringstream first_ss;
			first_ss << first;

			StringReplace(output, n_ss.str(), first_ss.str());

			FormatInternal(output, n + 1, rest...);
		}

		// the terminating condition
		static void FormatInternal(std::string& output, size_t n)
		{
		}
	};
}


#endif
