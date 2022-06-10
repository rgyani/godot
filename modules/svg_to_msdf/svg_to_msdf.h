/*************************************************************************/
/*  svg_to_msdf.h                                                        */
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

#include "core/io/resource.h"
#include "core/templates/thread_work_pool.h"
#include "scene/resources/texture.h"

#include "core/ShapeDistanceFinder.h"
#include "core/contour-combiners.h"
#include "core/edge-selectors.h"
#include "msdfgen.h"
#include "thorvg.h"

namespace tvg {
struct Iterator;
};

class SVGtoMSDF : public Resource {
	GDCLASS(SVGtoMSDF, Resource);

	int pixel_range = 0;
	bool ready = false;

	struct ShapeData {
		Ref<ImageTexture> texture;
		Vector2 offset;
		Rect2 uv_rect;
		Color fill_color;
		Color stroke_color;
		float stroke_width = 0.f;
	};

	Vector<ShapeData> data;
	mutable ThreadWorkPool work_pool;

protected:
	static void _bind_methods();

	void _generateMTSDF_threaded(uint32_t y, void *p_td) const;
	bool _access_shape(const tvg::Paint *p_paint);
	bool _access_children(tvg::Iterator *p_it, tvg::IteratorAccessor &p_itr);

public:
	bool create_from_string(const String &p_string, int p_pixel_range);
	void draw(RID p_canvas_item, const Rect2 &p_rect, const Color &p_modulate = Color(1, 1, 1)) const;

	~SVGtoMSDF() {
		work_pool.finish();
	}
};
