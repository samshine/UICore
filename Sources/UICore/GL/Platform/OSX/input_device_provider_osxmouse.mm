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
**    James Lammlein
**    Magnus Norddahl
*/

#include "UICore/precomp.h"
#include "input_device_provider_osxmouse.h"

#include "UICore/Core/Math/point.h"
#include "UICore/Core/Text/string_format.h"
#include "UICore/Display/Window/display_window.h"
#include "UICore/Display/Window/input_event.h"
#include "UICore/Display/Window/keys.h"

#import <CoreGraphics/CoreGraphics.h>

namespace uicore
{

/////////////////////////////////////////////////////////////////////////////
// InputDeviceProvider_OSXMouse construction:

InputDeviceProvider_OSXMouse::InputDeviceProvider_OSXMouse(OpenGLWindowProvider *window)
    : mouse_position(0.0f, 0.0f), sig_provider_event(nullptr), window(window)
{
    for (int i = 0; i < uicore::mouse_count; ++i)
    {
        mouse_down_map[i] = false;
    }
}

InputDeviceProvider_OSXMouse::~InputDeviceProvider_OSXMouse()
{
	dispose();
}

/////////////////////////////////////////////////////////////////////////////
// InputDeviceProvider_OSXMouse attributes:

float InputDeviceProvider_OSXMouse::x() const
{
	throw_if_disposed();
	return mouse_position.x;
}

float InputDeviceProvider_OSXMouse::y() const
{
	throw_if_disposed();
	return mouse_position.y;
}

bool InputDeviceProvider_OSXMouse::keycode(int keycode) const
{
    throw_if_disposed();

    bool result = false;

    if (keycode < uicore::mouse_count)
    {
        result = mouse_down_map[keycode];
    }

    return result;
}

std::string InputDeviceProvider_OSXMouse::key_name(int id) const
{
	throw_if_disposed();

	switch (id)
	{
	case 0: return "Mouse left";
	case 1: return "Mouse right";
	case 2: return "Mouse middle";
	case 3: return "Mouse wheel up";
	case 4: return "Mouse wheel down";
	}

	return string_format("Mouse button %1", id);
}

float InputDeviceProvider_OSXMouse::axis(int index) const
{
	throw_if_disposed();
	return 0.0f;
}

std::string InputDeviceProvider_OSXMouse::name() const
{
	throw_if_disposed();
	return "System Mouse";
}

std::string InputDeviceProvider_OSXMouse::device_name() const
{
	throw_if_disposed();
	return "System Mouse";
}

std::vector<int> InputDeviceProvider_OSXMouse::axis_ids() const
{
	throw_if_disposed();
	return std::vector<int>();
}

int InputDeviceProvider_OSXMouse::button_count() const
{
	throw_if_disposed();
	return -1;
}

/////////////////////////////////////////////////////////////////////////////
// InputDeviceProvider_OSXMouse operations:

void InputDeviceProvider_OSXMouse::set_position(float x, float y)
{
    throw_if_disposed();
	CGWarpMouseCursorPosition(CGPointMake(x, y));
}

/////////////////////////////////////////////////////////////////////////////
// InputDeviceProvider_OSXMouse implementation:

void InputDeviceProvider_OSXMouse::on_dispose()
{
}

void InputDeviceProvider_OSXMouse::on_mouse_event(const uicore::InputCode& keycode, const uicore::InputEvent::Type& type,
                                                  const uicore::Pointf& position)
{
    if (keycode < uicore::mouse_count)
    {
        if (type == InputEvent::doubleclick || type == uicore::InputEvent::pressed)
        {
            mouse_down_map[keycode] = true;
        }
        else if (type == uicore::InputEvent::released)
        {
            mouse_down_map[keycode] = false;
        }
    }
}

}
