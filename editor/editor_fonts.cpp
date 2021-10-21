/*************************************************************************/
/*  editor_fonts.cpp                                                     */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2021 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2021 Godot Engine contributors (cf. AUTHORS.md).   */
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

#include "editor_fonts.h"

#include "builtin_fonts.gen.h"
#include "core/io/dir_access.h"
#include "editor_scale.h"
#include "editor_settings.h"
#include "scene/resources/default_theme/default_theme.h"
#include "scene/resources/font.h"

#define MAKE_DEFAULT_FONT(m_name, m_variations, m_base_size)                          \
	Ref<Font> m_name;                                                                 \
	m_name.instantiate();                                                             \
	m_name->set_base_size(m_base_size);                                               \
	m_name->set_spacing(TextServer::SPACING_TOP, -EDSCALE);                           \
	m_name->set_spacing(TextServer::SPACING_BOTTOM, -EDSCALE);                        \
	for (List<Ref<FontData>>::Element *E = fonts_regular.front(); E; E = E->next()) { \
		m_name->add_data(E->get());                                                   \
	}                                                                                 \
	{                                                                                 \
		Dictionary variations;                                                        \
		if (m_variations != String()) {                                               \
			Vector<String> variation_tags = m_variations.split(",");                  \
			for (int i = 0; i < variation_tags.size(); i++) {                         \
				Vector<String> tokens = variation_tags[i].split("=");                 \
				if (tokens.size() == 2) {                                             \
					variations[tokens[0]] = tokens[1].to_float();                     \
				}                                                                     \
			}                                                                         \
		}                                                                             \
		m_name->set_variation_coordinates(variations);                                \
	}

#define MAKE_BOLD_FONT(m_name, m_variations, m_base_size)                          \
	Ref<Font> m_name;                                                              \
	m_name.instantiate();                                                          \
	m_name->set_base_size(m_base_size);                                            \
	m_name->set_spacing(TextServer::SPACING_TOP, -EDSCALE);                        \
	m_name->set_spacing(TextServer::SPACING_BOTTOM, -EDSCALE);                     \
	for (List<Ref<FontData>>::Element *E = fonts_bold.front(); E; E = E->next()) { \
		m_name->add_data(E->get());                                                \
	}                                                                              \
	{                                                                              \
		Dictionary variations;                                                     \
		if (m_variations != String()) {                                            \
			Vector<String> variation_tags = m_variations.split(",");               \
			for (int i = 0; i < variation_tags.size(); i++) {                      \
				Vector<String> tokens = variation_tags[i].split("=");              \
				if (tokens.size() == 2) {                                          \
					variations[tokens[0]] = tokens[1].to_float();                  \
				}                                                                  \
			}                                                                      \
		}                                                                          \
		m_name->set_variation_coordinates(variations);                             \
	}

#define MAKE_SOURCE_FONT(m_name, m_variations, m_base_size)                           \
	Ref<Font> m_name;                                                                 \
	m_name.instantiate();                                                             \
	m_name->set_base_size(m_base_size);                                               \
	m_name->set_spacing(TextServer::SPACING_TOP, -EDSCALE);                           \
	m_name->set_spacing(TextServer::SPACING_BOTTOM, -EDSCALE);                        \
	for (List<Ref<FontData>>::Element *E = fonts_source.front(); E; E = E->next()) {  \
		m_name->add_data(E->get());                                                   \
	}                                                                                 \
	for (List<Ref<FontData>>::Element *E = fonts_regular.front(); E; E = E->next()) { \
		m_name->add_data(E->get());                                                   \
	}                                                                                 \
	{                                                                                 \
		Dictionary variations;                                                        \
		if (m_variations != String()) {                                               \
			Vector<String> variation_tags = m_variations.split(",");                  \
			for (int i = 0; i < variation_tags.size(); i++) {                         \
				Vector<String> tokens = variation_tags[i].split("=");                 \
				if (tokens.size() == 2) {                                             \
					variations[tokens[0]] = tokens[1].to_float();                     \
				}                                                                     \
			}                                                                         \
		}                                                                             \
		m_name->set_variation_coordinates(variations);                                \
	}

Ref<FontData> load_external_font(const String &p_path, TextServer::Hinting p_hinting, bool p_aa, bool p_autohint) {
	Ref<FontData> font;
	font.instantiate();

	Vector<uint8_t> data = FileAccess::get_file_as_array(p_path);

	font->set_data(data);
	font->set_antialiased(p_aa);
	font->set_hinting(p_hinting);
	font->set_force_autohinter(p_autohint);

	return font;
}

Ref<FontData> load_internal_font(const uint8_t *p_data, size_t p_size, TextServer::Hinting p_hinting, bool p_aa, bool p_autohint) {
	Ref<FontData> font;
	font.instantiate();

	font->set_data_ptr(p_data, p_size);
	font->set_antialiased(p_aa);
	font->set_hinting(p_hinting);
	font->set_force_autohinter(p_autohint);

	return font;
}

void editor_register_fonts(Ref<Theme> p_theme) {
	DirAccess *dir = DirAccess::create(DirAccess::ACCESS_FILESYSTEM);

	/* Custom font */

	bool font_antialiased = (bool)EditorSettings::get_singleton()->get("interface/editor/font_antialiased");
	int font_hinting_setting = (int)EditorSettings::get_singleton()->get("interface/editor/font_hinting");

	TextServer::Hinting font_hinting;
	switch (font_hinting_setting) {
		case 0:
			// The "Auto" setting uses the setting that best matches the OS' font rendering:
			// - macOS doesn't use font hinting.
			// - Windows uses ClearType, which is in between "Light" and "Normal" hinting.
			// - Linux has configurable font hinting, but most distributions including Ubuntu default to "Light".
#ifdef OSX_ENABLED
			font_hinting = TextServer::HINTING_NONE;
#else
			font_hinting = TextServer::HINTING_LIGHT;
#endif
			break;
		case 1:
			font_hinting = TextServer::HINTING_NONE;
			break;
		case 2:
			font_hinting = TextServer::HINTING_LIGHT;
			break;
		default:
			font_hinting = TextServer::HINTING_NORMAL;
			break;
	}

	List<Ref<FontData>> fonts_regular;
	List<Ref<FontData>> fonts_bold;
	List<Ref<FontData>> fonts_source;

	int default_font_size = int(EDITOR_GET("interface/editor/main_font_size")) * EDSCALE;

	// Load custom regular font data.
	String custom_font_path = EditorSettings::get_singleton()->get("interface/editor/main_font");
	if (custom_font_path.length() > 0 && dir->file_exists(custom_font_path)) {
		fonts_regular.push_back(load_external_font(custom_font_path, font_hinting, font_antialiased, true));
	} else {
		EditorSettings::get_singleton()->set_manually("interface/editor/main_font", "");
	}

	/* Load custom bold font data */
	String custom_font_path_bold = EditorSettings::get_singleton()->get("interface/editor/main_font_bold");
	if (custom_font_path_bold.length() > 0 && dir->file_exists(custom_font_path_bold)) {
		fonts_bold.push_back(load_external_font(custom_font_path_bold, font_hinting, font_antialiased, true));
	} else {
		EditorSettings::get_singleton()->set_manually("interface/editor/main_font_bold", "");
	}

	// Load custom source code font data.
	String custom_font_path_source = EditorSettings::get_singleton()->get("interface/editor/code_font");
	if (custom_font_path_source.length() > 0 && dir->file_exists(custom_font_path_source)) {
		fonts_source.push_back(load_external_font(custom_font_path_source, font_hinting, font_antialiased, true));
	} else {
		EditorSettings::get_singleton()->set_manually("interface/editor/code_font", "");
	}

	memdelete(dir);

	// Load built-in font data.
	fonts_regular.push_back(load_internal_font(_font_NotoSans_Regular, _font_NotoSans_Regular_size, font_hinting, font_antialiased, true));
	fonts_bold.push_back(load_internal_font(_font_NotoSans_Bold, _font_NotoSans_Bold_size, font_hinting, font_antialiased, true));
	fonts_source.push_back(load_internal_font(_font_Hack_Regular, _font_Hack_Regular_size, font_hinting, font_antialiased, true));

	// Load external font data.
	DirAccessRef da = DirAccess::open(EditorPaths::get_singleton()->get_data_dir().plus_file("fonts"));
	if (da) {
		da->list_dir_begin();
		String path = da->get_next();
		while (path != String()) {
			if (!da->current_is_dir() && (path.ends_with(".ttf") || path.ends_with(".otf") || path.ends_with(".woff") || path.ends_with(".pfb") || path.ends_with(".pfm"))) {
				if (path.get_basename().ends_with("Regular")) {
					fonts_regular.push_back(load_external_font(EditorPaths::get_singleton()->get_data_dir().plus_file("fonts").plus_file(path), font_hinting, font_antialiased, true));
				} else if (path.get_basename().ends_with("Bold")) {
					fonts_bold.push_back(load_external_font(EditorPaths::get_singleton()->get_data_dir().plus_file("fonts").plus_file(path), font_hinting, font_antialiased, true));
				} else if (path.get_basename().ends_with("Mono")) {
					fonts_source.push_back(load_external_font(EditorPaths::get_singleton()->get_data_dir().plus_file("fonts").plus_file(path), font_hinting, font_antialiased, true));
				}
			}
			path = da->get_next();
		}

		da->list_dir_end();
	}

	// Default font.
	MAKE_DEFAULT_FONT(df, String(), default_font_size);
	p_theme->set_default_theme_font(df); // Default theme font
	p_theme->set_default_theme_font_size(default_font_size);

	p_theme->set_font_size("main_size", "EditorFonts", default_font_size);
	p_theme->set_font("main", "EditorFonts", df);

	// Bold font.
	MAKE_BOLD_FONT(df_bold, String(), default_font_size);
	p_theme->set_font_size("bold_size", "EditorFonts", default_font_size);
	p_theme->set_font("bold", "EditorFonts", df_bold);

	// Title font.
	p_theme->set_font_size("title_size", "EditorFonts", default_font_size + 1 * EDSCALE);
	p_theme->set_font("title", "EditorFonts", df_bold);

	p_theme->set_font_size("main_button_font_size", "EditorFonts", default_font_size + 1 * EDSCALE);
	p_theme->set_font("main_button_font", "EditorFonts", df_bold);

	p_theme->set_font("font", "Label", df);

	p_theme->set_type_variation("HeaderSmall", "Label");
	p_theme->set_font("font", "HeaderSmall", df_bold);
	p_theme->set_font_size("font_size", "HeaderSmall", default_font_size);

	p_theme->set_type_variation("HeaderMedium", "Label");
	p_theme->set_font("font", "HeaderMedium", df_bold);
	p_theme->set_font_size("font_size", "HeaderMedium", default_font_size + 1 * EDSCALE);

	p_theme->set_type_variation("HeaderLarge", "Label");
	p_theme->set_font("font", "HeaderLarge", df_bold);
	p_theme->set_font_size("font_size", "HeaderLarge", default_font_size + 3 * EDSCALE);

	// Documentation fonts.
	String code_font_custom_variations = EditorSettings::get_singleton()->get("interface/editor/code_font_custom_variations");
	MAKE_SOURCE_FONT(df_code, code_font_custom_variations, default_font_size);
	p_theme->set_font_size("doc_size", "EditorFonts", int(EDITOR_GET("text_editor/help/help_font_size")) * EDSCALE);
	p_theme->set_font("doc", "EditorFonts", df);
	p_theme->set_font_size("doc_bold_size", "EditorFonts", int(EDITOR_GET("text_editor/help/help_font_size")) * EDSCALE);
	p_theme->set_font("doc_bold", "EditorFonts", df_bold);
	p_theme->set_font_size("doc_title_size", "EditorFonts", int(EDITOR_GET("text_editor/help/help_title_font_size")) * EDSCALE);
	p_theme->set_font("doc_title", "EditorFonts", df_bold);
	p_theme->set_font_size("doc_source_size", "EditorFonts", int(EDITOR_GET("text_editor/help/help_source_font_size")) * EDSCALE);
	p_theme->set_font("doc_source", "EditorFonts", df_code);
	p_theme->set_font_size("doc_keyboard_size", "EditorFonts", (int(EDITOR_GET("text_editor/help/help_source_font_size")) - 1) * EDSCALE);
	p_theme->set_font("doc_keyboard", "EditorFonts", df_code);

	// Ruler font.
	p_theme->set_font_size("rulers_size", "EditorFonts", 8 * EDSCALE);
	p_theme->set_font("rulers", "EditorFonts", df);

	// Rotation widget font.
	p_theme->set_font_size("rotation_control_size", "EditorFonts", 14 * EDSCALE);
	p_theme->set_font("rotation_control", "EditorFonts", df);

	// Code font
	p_theme->set_font_size("source_size", "EditorFonts", int(EDITOR_GET("interface/editor/code_font_size")) * EDSCALE);
	p_theme->set_font("source", "EditorFonts", df_code);

	p_theme->set_font_size("expression_size", "EditorFonts", (int(EDITOR_GET("interface/editor/code_font_size")) - 1) * EDSCALE);
	p_theme->set_font("expression", "EditorFonts", df_code);

	p_theme->set_font_size("output_source_size", "EditorFonts", int(EDITOR_GET("run/output/font_size")) * EDSCALE);
	p_theme->set_font("output_source", "EditorFonts", df_code);

	p_theme->set_font_size("status_source_size", "EditorFonts", default_font_size);
	p_theme->set_font("status_source", "EditorFonts", df_code);
}
