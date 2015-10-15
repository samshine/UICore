/*
**  UICore
**  Copyright (c) 1997-2015 The UICore Team
**
**  This software is provided 'as-is', without any express or implied
**  warranty.  In no event will the authors be held liable for any damages
**  arising from the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be
**     misrepresented as being the original software.
**  3. This notice may not be removed or altered from any source distribution.
**
**  Note: Some of the libraries UICore may link to may have additional
**  requirements or restrictions.
**
**  File Author(s):
**
**    Magnus Norddahl
*/

#pragma once

#include "UICore/Core/System/exception.h"

namespace uicore
{

#ifdef WIN32

	class ExceptionDialog_Impl
	{
	public:
		static void show(const std::string &message_and_stack_trace);

	private:
		ExceptionDialog_Impl(const std::string &message_and_stack_trace, HWND owner);
		ExceptionDialog_Impl(const ExceptionDialog_Impl &);
		ExceptionDialog_Impl &operator=(const ExceptionDialog_Impl &);
		~ExceptionDialog_Impl();
		LRESULT window_proc(HWND window_handle, UINT message_id, WPARAM wparam, LPARAM lparam);
		static LRESULT CALLBACK static_window_proc(HWND window_handle, UINT message_id, WPARAM wparam, LPARAM lparam);

		HWND window_handle;
		HWND frame;
		HWND text_label;
		HWND ok_button;
		HFONT font;
	};

#else

	class ExceptionDialog_Impl
	{
	public:
		static void show(const std::string &message_and_stack_trace);
	};

#endif

}
