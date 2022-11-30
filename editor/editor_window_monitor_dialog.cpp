/*************************************************************************/
/*  editor_window_monitor_dialog.cpp                                     */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2022 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2022 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "editor_window_monitor_dialog.h"

void ScreenView::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_INTERNAL_PROCESS: {
			queue_redraw();
		} break;
		case Control::NOTIFICATION_DRAW: {
			const Size2 &size = get_size() - Vector2(40, 40);
			Rect2 bounds;
			DisplayServer *ds = DisplayServer::get_singleton();
			for (int i = 0; i < ds->get_screen_count(); i++) {
				bounds.position.x = MIN(bounds.position.x, ds->screen_get_position(i).x);
				bounds.position.y = MIN(bounds.position.y, ds->screen_get_position(i).y);
				bounds.size.x = MAX(bounds.size.x, ds->screen_get_position(i).x + ds->screen_get_size(i).x);
				bounds.size.y = MAX(bounds.size.y, ds->screen_get_position(i).y + ds->screen_get_size(i).y);
			}
			Ref<Font> f = get_theme_font(SNAME("font"));
			double scale = MIN(size.x / (bounds.size.x - bounds.position.x), size.y / (bounds.size.y - bounds.position.y));
			Vector2 offset = Vector2(20, 20) - bounds.position * scale;
			draw_rect(Rect2(offset + bounds.position * scale, Vector2(bounds.size.x - bounds.position.x, bounds.size.y - bounds.position.y) * scale), Color(0, 1, 0), false, 3);
			for (int i = 0; i < ds->get_screen_count(); i++) {
				draw_rect(Rect2(offset + Vector2(ds->screen_get_position(i)) * scale, Vector2(ds->screen_get_size(i)) * scale), Color(1, 0, 0), true);
				draw_rect(Rect2(offset + Vector2(ds->screen_get_usable_rect(i).position) * scale, Vector2(ds->screen_get_usable_rect(i).size) * scale), Color(1, 1, 1), true);
				draw_rect(Rect2(offset + Vector2(ds->screen_get_usable_rect(i).position) * scale, Vector2(ds->screen_get_usable_rect(i).size) * scale), Color(1, 0, 0), false);
				draw_string(f, offset + Vector2(0, f->get_height(16)) + Vector2(ds->screen_get_usable_rect(i).position) * scale, vformat(" [%d] DPI: %d (%f) %f %s O:%d", i, ds->screen_get_dpi(i), ds->screen_get_scale(i), ds->screen_get_refresh_rate(i), ds->screen_is_touchscreen(i) ? "T" : "", ds->screen_get_orientation(i)), HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Color(0, 0, 0));
			}
			DisplayServer::WindowID popup = ds->window_get_active_popup();
			Vector<DisplayServer::WindowID> wl = ds->get_window_list();
			for (DisplayServer::WindowID i : wl) {
				float a = 1;
				if (ds->window_get_mode(i) == DisplayServer::WINDOW_MODE_MINIMIZED) {
					a = 0.5;
				}
				Color color = Color(0, 0, 1, a);
				if (i == popup) {
					color = Color(1, 0, 1, a);
				}
				draw_rect(Rect2(offset + Vector2(ds->window_get_position_with_decorations(i)) * scale, Vector2(ds->window_get_size_with_decorations(i)) * scale), Color(0, 0, 1, a), false, 2);
				draw_rect(Rect2(offset + Vector2(ds->window_get_position(i)) * scale, Vector2(ds->window_get_size(i)) * scale), Color(0.5, 0.5, 0.5, 0.5 * a), true);
				if (ds->window_get_min_size(i) != Size2i()) {
					draw_rect(Rect2(offset + Vector2(ds->window_get_position(i)) * scale, Vector2(ds->window_get_min_size(i)) * scale), Color(0, 1, 1, a), false, 1);
				}
				if (ds->window_get_max_size(i) != Size2i() && ds->window_get_max_size(i) != RenderingServer::get_singleton()->get_maximum_viewport_size()) {
					draw_rect(Rect2(offset + Vector2(ds->window_get_position(i)) * scale, Vector2(ds->window_get_max_size(i)) * scale), Color(0, 1, 1, a), false, 1);
				}
				draw_rect(Rect2(offset + Vector2(ds->window_get_position(i)) * scale, Vector2(ds->window_get_size(i)) * scale), color, false, 3);
				Rect2 sr = ds->window_get_popup_safe_rect(i);
				if (sr != Rect2i()) {
					draw_rect(Rect2(offset + Vector2(sr.position) * scale, Vector2(sr.size) * scale), Color(1, 1, 0, 0.4), true);
				}

				draw_string(f, offset + Vector2(0, f->get_height(16) * 2) + Vector2(ds->window_get_position(i)) * scale, vformat(" (%d:%d) @%d - %s", i, ds->window_get_transient(i), ds->window_get_current_screen(i), ds->window_get_title(i)), HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Color(0, 0, 0));
				String ifo;
				switch (ds->window_get_mode(i)) {
					case DisplayServer::WINDOW_MODE_WINDOWED: {
						ifo += "W";
					} break;
					case DisplayServer::WINDOW_MODE_MINIMIZED: {
						ifo += "m";
					} break;
					case DisplayServer::WINDOW_MODE_MAXIMIZED: {
						ifo += "M";
					} break;
					case DisplayServer::WINDOW_MODE_FULLSCREEN: {
						ifo += "F";
					} break;
					case DisplayServer::WINDOW_MODE_EXCLUSIVE_FULLSCREEN: {
						ifo += "eF";
					} break;
				}
				ifo += " ";
				if (ds->window_get_flag(DisplayServer::WINDOW_FLAG_RESIZE_DISABLED, i))
					ifo += "R";
				if (ds->window_get_flag(DisplayServer::WINDOW_FLAG_BORDERLESS, i))
					ifo += "B";
				if (ds->window_get_flag(DisplayServer::WINDOW_FLAG_ALWAYS_ON_TOP, i))
					ifo += "T";
				if (ds->window_get_flag(DisplayServer::WINDOW_FLAG_TRANSPARENT, i))
					ifo += "S";
				if (ds->window_get_flag(DisplayServer::WINDOW_FLAG_NO_FOCUS, i))
					ifo += "F";
				if (ds->window_get_flag(DisplayServer::WINDOW_FLAG_POPUP, i))
					ifo += "P";
				if (ds->window_get_flag(DisplayServer::WINDOW_FLAG_EXTEND_TO_TITLE, i))
					ifo += "I";
				draw_string(f, offset + Vector2(0, f->get_height(16) * 3) + Vector2(ds->window_get_position(i)) * scale, ifo, HORIZONTAL_ALIGNMENT_LEFT, -1, 16, Color(0, 0, 0));
			}
			Vector2 mp = ds->mouse_get_position();
			draw_circle(offset + mp * scale, 5, Color(0, 1, 0));
		} break;
	}
}

void ScreenView::_bind_methods() {
}

ScreenView::ScreenView() {
	set_process_internal(true);
}

/*************************************************************************/

void WindowEventView::_notification(int p_what) {
}

void WindowEventView::_bind_methods() {
}

WindowEventView::WindowEventView() {
	set_process_internal(true);
}

/*************************************************************************/

void EditorWindowMonitorDialog::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_INTERNAL_PROCESS: {
		} break;
	}
}

void EditorWindowMonitorDialog::_bind_methods() {
}

EditorWindowMonitorDialog::EditorWindowMonitorDialog() {
	set_flag(FLAG_ALWAYS_ON_TOP, true);
	set_size(Vector2(800, 600));

	HSplitContainer *hsc = memnew(HSplitContainer);
	hsc->set_anchors_and_offsets_preset(Control::PRESET_FULL_RECT);
	add_child(hsc);

	wnd_tree = memnew(Tree);
	wnd_tree->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	wnd_tree->set_stretch_ratio(0.25);
	hsc->add_child(wnd_tree);

	VSplitContainer *vsc = memnew(VSplitContainer);
	vsc->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	hsc->add_child(vsc);

	ScreenView *scrview = memnew(ScreenView);
	scrview->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	vsc->add_child(scrview);

	WindowEventView *wndview = memnew(WindowEventView);
	wndview->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	vsc->add_child(wndview);

	set_process_internal(true);
}
