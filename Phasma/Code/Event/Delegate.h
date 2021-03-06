/*
Copyright (c) 2018-2021 Christos Karamoustos

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include <vector>
#include <functional>
#include <utility>

namespace pe
{
	template<typename ... T>
	class Delegate
	{
	public:
		using Func_type = std::function<void(const T& ...)>;
		
		inline void operator+=(Func_type&& func)
		{
			const size_t funcAddress = *reinterpret_cast<long*>(reinterpret_cast<char*>(&func));
			
			bool alreadyExist = false;
			for (auto& function : m_functions)
			{
				const size_t functionAddress = *reinterpret_cast<long*>(reinterpret_cast<char*>(&function));
				
				// This equality seems to work, holding back though since it might not be accurate
				if (funcAddress == functionAddress)
					alreadyExist = true;
			}
			
			if (!alreadyExist)
				m_functions.push_back(std::forward<Func_type>(func));
		}
		
		inline void operator-=(Func_type&& func)
		{
			const size_t funcAddress = *reinterpret_cast<long*>(reinterpret_cast<char*>(&func));
			
			int index = 0;
			for (auto& function : m_functions)
			{
				const size_t functionAddress = *reinterpret_cast<long*>(reinterpret_cast<char*>(&function));
				
				// This equality seems to work, holding back though since it might not be accurate
				if (funcAddress == functionAddress)
					m_functions.erase(m_functions.begin() + index);
				else
					index++;
			}
		}
		
		inline void Invoke(const T& ... args)
		{
			for (auto& function : m_functions)
				function(args...);
		}
	
	private:
		std::vector<Func_type> m_functions {};
	};
}