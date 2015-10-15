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

#include "UICore/precomp.h"
#include "UICore/UI/TopLevel/top_level_window.h"
#include "UICore/UI/Events/key_event.h"
#include "UICore/UI/Events/pointer_event.h"
#include "UICore/UI/Events/close_event.h"
#include "UICore/UI/Events/activation_change_event.h"
#include "UICore/Display/Window/input_event.h"
#include "UICore/Display/2D/canvas.h"
#include "top_level_window_impl.h"

namespace uicore
{
	TopLevelWindow_Impl::TopLevelWindow_Impl(TopLevelWindow *view, const DisplayWindowDescription &desc) : window_view(view), window(DisplayWindow::create(desc))
	{
		canvas = Canvas::create(window);

		slots.connect(window->sig_lost_focus(), uicore::bind_member(this, &TopLevelWindow_Impl::on_lost_focus));
		slots.connect(window->sig_got_focus(), uicore::bind_member(this, &TopLevelWindow_Impl::on_got_focus));
		slots.connect(window->sig_resize(), uicore::bind_member(this, &TopLevelWindow_Impl::on_resize));
		slots.connect(window->sig_paint(), uicore::bind_member(this, &TopLevelWindow_Impl::on_paint));
		slots.connect(window->sig_window_close(), uicore::bind_member(this, &TopLevelWindow_Impl::on_window_close));
		slots.connect(window->keyboard()->sig_key_down(), uicore::bind_member(this, &TopLevelWindow_Impl::on_key_down));
		slots.connect(window->keyboard()->sig_key_up(), uicore::bind_member(this, &TopLevelWindow_Impl::on_key_up));
		slots.connect(window->mouse()->sig_key_down(), uicore::bind_member(this, &TopLevelWindow_Impl::on_mouse_down));
		slots.connect(window->mouse()->sig_key_dblclk(), uicore::bind_member(this, &TopLevelWindow_Impl::on_mouse_dblclk));
		slots.connect(window->mouse()->sig_key_up(), uicore::bind_member(this, &TopLevelWindow_Impl::on_mouse_up));
		slots.connect(window->mouse()->sig_pointer_move(), uicore::bind_member(this, &TopLevelWindow_Impl::on_mouse_move));
	}

	void TopLevelWindow_Impl::on_lost_focus()
	{
		release_capture();
		window_view->dispatch_activation_change(ActivationChangeType::deactivated);
	}

	void TopLevelWindow_Impl::on_got_focus()
	{
		release_capture();
		window_view->dispatch_activation_change(ActivationChangeType::activated);
	}

	void TopLevelWindow_Impl::on_resize(float, float)
	{
		window->request_repaint();
	}

	void TopLevelWindow_Impl::on_paint()
	{
		canvas->begin();
		canvas->clear(uicore::Colorf::transparent);
		window_view->render(canvas, window->viewport());
		canvas->end();
		window->flip();
	}

	void TopLevelWindow_Impl::on_window_close()
	{
		CloseEvent e;
		View::dispatch_event(window_view->root_view().get(), &e);
	}

	void TopLevelWindow_Impl::window_key_event(KeyEvent &e)
	{
		View *view = window_view->focus_view();
		if (view)
		{
			View::dispatch_event(view, &e);
		}

		if (!e.default_prevented() && e.type() == KeyEventType::press && e.shift_down() && e.key() == Key::tab)
		{
			window_view->root_view()->prev_focus();
		}
		else if (!e.default_prevented() && e.type() == KeyEventType::press && e.key() == Key::tab)
		{
			window_view->root_view()->next_focus();
		}
	}

	void TopLevelWindow_Impl::dispatch_hot_event(std::shared_ptr<View> &view, PointerEvent &e)
	{
		if (view != hot_view)
		{
			if (hot_view)
			{
				PointerEvent e_exit(PointerEventType::leave, PointerButton::none, e.pos(window_view->root_view()), e.alt_down(), e.shift_down(), e.ctrl_down(), e.cmd_down());
				View::dispatch_event(hot_view.get(), &e_exit, true);
			}

			hot_view = view;

			if (hot_view)
			{
				PointerEvent e_enter(PointerEventType::enter, PointerButton::none, e.pos(window_view->root_view()), e.alt_down(), e.shift_down(), e.ctrl_down(), e.cmd_down());
				View::dispatch_event(hot_view.get(), &e_enter, true);
			}
		}

		if (hot_view)
			hot_view->update_cursor(window);

	}

	void TopLevelWindow_Impl::release_capture()
	{
		if (captured_view)
		{
			window->capture_mouse(false);
			captured_view.reset();
			capture_down_counter = 0;
		}
	}

	std::shared_ptr<View> TopLevelWindow_Impl::get_capture_view(PointerEvent &e, std::shared_ptr<View> &view_above_cursor)
	{
		if (e.type() == PointerEventType::press || e.type() == PointerEventType::double_click)
		{
			capture_down_counter++;
			if (capture_down_counter == 1)
			{
				captured_view = view_above_cursor;
				if (captured_view)
					window->capture_mouse(true);
			}
		}

		std::shared_ptr<View> view = captured_view;

		if (e.type() == PointerEventType::release)
		{
			if (capture_down_counter > 0)
			{
				capture_down_counter--;
				if (capture_down_counter == 0)
				{
					if (captured_view)
						release_capture();
				}
			}
		}
		return view;
	}

	void TopLevelWindow_Impl::window_pointer_event(PointerEvent &e)
	{
		std::shared_ptr<View> view_above_cursor = window_view->root_view()->find_view_at(e.pos(window_view->root_view()));
		auto view = get_capture_view(e, view_above_cursor);
		if (!view)
			view = view_above_cursor;

		dispatch_hot_event(view, e);

		if (e.type() == PointerEventType::enter || e.type() == PointerEventType::leave)		// Ignore window enter/leave events
			return;

		if (view)
			View::dispatch_event(view.get(), &e);
		else
			View::dispatch_event(window_view->root_view().get(), &e);
	}

	Pointf TopLevelWindow_Impl::to_root_pos(const Pointf &client_pos) const
	{
		return window_view->root_view()->to_root_pos(client_pos - window_view->root_view()->geometry().content_pos());
	}

	void TopLevelWindow_Impl::on_key_down(const uicore::InputEvent &e)
	{
		KeyEventType type = KeyEventType::press;
		Key key = decode_ic(e.id);
		int repeat_count = e.repeat_count;
		const std::string text = e.str;
		const Pointf pointer_pos = to_root_pos(e.mouse_pos);
		bool alt_down = e.alt;
		bool shift_down = e.shift;
		bool ctrl_down = e.ctrl;
		bool cmd_down = false;
		KeyEvent key_event(type, key, repeat_count, text, pointer_pos, alt_down, shift_down, ctrl_down, cmd_down);
		window_key_event(key_event);
	}

	void TopLevelWindow_Impl::on_key_up(const uicore::InputEvent &e)
	{
		KeyEventType type = KeyEventType::release;
		Key key = decode_ic(e.id);
		int repeat_count = e.repeat_count;
		const std::string text = e.str;
		const Pointf pointer_pos = to_root_pos(e.mouse_pos);
		bool alt_down = e.alt;
		bool shift_down = e.shift;
		bool ctrl_down = e.ctrl;
		bool cmd_down = false;
		KeyEvent key_event(type, key, repeat_count, text, pointer_pos, alt_down, shift_down, ctrl_down, cmd_down);
		window_key_event(key_event);
	}

	void TopLevelWindow_Impl::on_mouse_down(const uicore::InputEvent &e)
	{
		PointerEventType type = PointerEventType::press;
		PointerButton button = decode_id(e.id);
		const Pointf pos = to_root_pos(e.mouse_pos);
		bool alt_down = e.alt;
		bool shift_down = e.shift;
		bool ctrl_down = e.ctrl;
		bool cmd_down = false;
		PointerEvent pointer_event(type, button, pos, alt_down, shift_down, ctrl_down, cmd_down);
		window_pointer_event(pointer_event);
	}

	void TopLevelWindow_Impl::on_mouse_dblclk(const uicore::InputEvent &e)
	{
		PointerEventType type = PointerEventType::double_click;
		PointerButton button = decode_id(e.id);
		const Pointf pos = to_root_pos(e.mouse_pos);
		bool alt_down = e.alt;
		bool shift_down = e.shift;
		bool ctrl_down = e.ctrl;
		bool cmd_down = false;
		PointerEvent pointer_event(type, button, pos, alt_down, shift_down, ctrl_down, cmd_down);
		window_pointer_event(pointer_event);
	}

	void TopLevelWindow_Impl::on_mouse_up(const uicore::InputEvent &e)
	{
		PointerEventType type = PointerEventType::release;
		PointerButton button = decode_id(e.id);
		const Pointf pos = to_root_pos(e.mouse_pos);
		bool alt_down = e.alt;
		bool shift_down = e.shift;
		bool ctrl_down = e.ctrl;
		bool cmd_down = false;
		PointerEvent pointer_event(type, button, pos, alt_down, shift_down, ctrl_down, cmd_down);
		window_pointer_event(pointer_event);
	}

	void TopLevelWindow_Impl::on_mouse_move(const uicore::InputEvent &uicore_event)
	{
		PointerEvent e(PointerEventType::move, PointerButton::none, to_root_pos(uicore_event.mouse_pos), uicore_event.alt, uicore_event.shift, uicore_event.ctrl, false/*uicore_event.cmd*/);
		window_pointer_event(e);
	}

	PointerButton TopLevelWindow_Impl::decode_id(uicore::InputCode ic) const
	{
		switch (ic)
		{
		default:
			return PointerButton::none;
		case uicore::InputCode::mouse_left:
			return PointerButton::left;
		case uicore::InputCode::mouse_right:
			return PointerButton::right;
		case uicore::InputCode::mouse_middle:
			return PointerButton::middle;
		case uicore::InputCode::mouse_wheel_up:
			return PointerButton::wheel_up;
		case uicore::InputCode::mouse_wheel_down:
			return PointerButton::wheel_down;
		case uicore::InputCode::mouse_xbutton1:
			return PointerButton::xbutton1;
		case uicore::InputCode::mouse_xbutton2:
			return PointerButton::xbutton2;
			//case uicore::InputCode::mouse_xbutton3:
			// return PointerButton::xbutton3;
			//case uicore::InputCode::mouse_xbutton4:
			// return PointerButton::xbutton4;
			//case uicore::InputCode::mouse_xbutton5:
			// return PointerButton::xbutton5;
			//case uicore::InputCode::mouse_xbutton6:
			// return PointerButton::xbutton6;
		}
	}
}
