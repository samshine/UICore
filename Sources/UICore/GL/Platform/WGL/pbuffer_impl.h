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
**    Mark Page
*/

#pragma once

#include "UICore/GL/opengl_wrap.h"
#include "../../opengl_graphic_context_provider.h"

namespace uicore
{
	class GL1GraphicContext;
	class OpenGLWindowProvider;

	class PBuffer_GL1_Impl : public OpenGLContextProvider
	{
	public:
		PBuffer_GL1_Impl(GL1GraphicContext *gc_provider);
		~PBuffer_GL1_Impl();

		void make_current() const;

		void get_opengl_version(int &version_major, int &version_minor) const;
		void get_opengl_version(int &version_major, int &version_minor, int &version_release) const;

		void create(OpenGLWindowProvider &window_provider, const Size &size);
		ProcAddress *get_proc_address(const std::string& function_name) const;

	private:
		void reset();

		GL1GraphicContext *gc_provider;
		GLFunctions::HPBUFFERARB pbuffer;
		HGLRC pbuffer_context;
		HDC pbuffer_dc;

		Size pbuffer_size;
	};
}
