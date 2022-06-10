/*************************************************************************/
/*  svg_to_msdf.cpp                                                      */
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

#include "svg_to_msdf.h"

#include "tvgIteratorAccessor.h"

class DistancePixelConversion {
	double invRange;

public:
	_FORCE_INLINE_ explicit DistancePixelConversion(double range) :
			invRange(1 / range) {}
	_FORCE_INLINE_ void operator()(float *pixels, const msdfgen::MultiAndTrueDistance &distance) const {
		pixels[0] = float(invRange * distance.r + .5);
		pixels[1] = float(invRange * distance.g + .5);
		pixels[2] = float(invRange * distance.b + .5);
		pixels[3] = float(invRange * distance.a + .5);
	}
};

struct MSDFThreadData {
	msdfgen::Bitmap<float, 4> *output;
	msdfgen::Shape *shape;
	msdfgen::Projection *projection;
	DistancePixelConversion *distancePixelConversion;
};

static msdfgen::Point2 tv_point2(const tvg::Point &vector) {
	return msdfgen::Point2(vector.x, vector.y);
}

void SVGtoMSDF::_generateMTSDF_threaded(uint32_t y, void *p_td) const {
	MSDFThreadData *td = static_cast<MSDFThreadData *>(p_td);

	msdfgen::ShapeDistanceFinder<msdfgen::OverlappingContourCombiner<msdfgen::MultiAndTrueDistanceSelector>> distanceFinder(*td->shape);
	int row = td->shape->inverseYAxis ? td->output->height() - y - 1 : y;
	for (int col = 0; col < td->output->width(); ++col) {
		int x = (y % 2) ? td->output->width() - col - 1 : col;
		msdfgen::Point2 p = td->projection->unproject(msdfgen::Point2(x + .5, y + .5));
		msdfgen::MultiAndTrueDistance distance = distanceFinder.distance(p);
		td->distancePixelConversion->operator()(td->output->operator()(x, row), distance);
	}
}

bool SVGtoMSDF::_access_shape(const tvg::Paint *p_paint) {
	if (!p_paint) {
		return false;
	}

	const tvg::Shape *tvg_shape = dynamic_cast<const tvg::Shape *>(p_paint);
	if (tvg_shape) {
		msdfgen::Point2 position;
		msdfgen::Shape shape;
		msdfgen::Contour *contour = nullptr;

		const tvg::PathCommand *cmds = nullptr;
		uint32_t cmd_size = tvg_shape->pathCommands(&cmds);

		const tvg::Point *points = nullptr;
		tvg_shape->pathCoords(&points);

		uint32_t p = 0;
		for (uint32_t i = 0; i < cmd_size; ++i) {
			switch (*(cmds + i)) {
				case tvg::PathCommand::Close: {
					if (!shape.contours.empty() && shape.contours.back().edges.empty()) {
						shape.contours.pop_back();
					}
					p++;
				} break;
				case tvg::PathCommand::MoveTo: {
					if (!(contour && contour->edges.empty())) {
						contour = &shape.addContour();
					}
					position = tv_point2(*(points + p));
					p++;
				} break;
				case tvg::PathCommand::LineTo: {
					msdfgen::Point2 endpoint = tv_point2(*(points + p));
					if (endpoint != position) {
						contour->addEdge(new msdfgen::LinearSegment(position, endpoint));
						position = endpoint;
					}
					p++;
				} break;
				case tvg::PathCommand::CubicTo: {
					contour->addEdge(new msdfgen::CubicSegment(position, tv_point2(*(points + p)), tv_point2(*(points + p + 1)), tv_point2(*(points + p + 2))));
					position = tv_point2(*(points + p + 2));
					p += 3;
				} break;
			}
		}

		shape.normalize();

		msdfgen::Shape::Bounds bounds = shape.getBounds(pixel_range);

		if (shape.validate() && shape.contours.size() > 0) {
			int w = (bounds.r - bounds.l);
			int h = (bounds.t - bounds.b);

			edgeColoringSimple(shape, 3.0); // Max. angle.
			msdfgen::Bitmap<float, 4> image(w, h); // Texture size.

			DistancePixelConversion distancePixelConversion(pixel_range);
			msdfgen::Projection projection(msdfgen::Vector2(1.0, 1.0), msdfgen::Vector2(-bounds.l, -bounds.b));
			msdfgen::MSDFGeneratorConfig config(true, msdfgen::ErrorCorrectionConfig());

			MSDFThreadData td;
			td.output = &image;
			td.shape = &shape;
			td.projection = &projection;
			td.distancePixelConversion = &distancePixelConversion;

			if (work_pool.get_thread_count() == 0) {
				work_pool.init();
			}
			work_pool.do_work(h, this, &SVGtoMSDF::_generateMTSDF_threaded, &td);

			msdfgen::msdfErrorCorrection(image, shape, projection, pixel_range, config);

			PackedByteArray imgdata;
			imgdata.resize(w * h * 4);
			uint8_t *wr = imgdata.ptrw();
			for (int i = 0; i < h; i++) {
				for (int j = 0; j < w; j++) {
					int ofs = (i * w + j) * 4;
					wr[ofs + 0] = (uint8_t)(CLAMP(image(j, i)[0] * 256.f, 0.f, 255.f));
					wr[ofs + 1] = (uint8_t)(CLAMP(image(j, i)[1] * 256.f, 0.f, 255.f));
					wr[ofs + 2] = (uint8_t)(CLAMP(image(j, i)[2] * 256.f, 0.f, 255.f));
					wr[ofs + 3] = (uint8_t)(CLAMP(image(j, i)[3] * 256.f, 0.f, 255.f));
				}
			}

			Ref<::Image> img;
			img.instantiate();
			img->create_from_data(w, h, false, Image::FORMAT_RGBA8, imgdata);

			ShapeData item;
			item.texture.instantiate();
			item.texture->create_from_image(img);
			item.stroke_width = tvg_shape->strokeWidth();
			item.uv_rect = Rect2(0, 0, w, h);

			uint8_t r, g, b, a;
			tvg_shape->strokeColor(&r, &g, &b, &a);
			item.stroke_color = ::Color(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);
			tvg_shape->fillColor(&r, &g, &b, &a);
			item.fill_color = ::Color(r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f);

			float sx, sy, sh, sw;
			tvg_shape->bounds(&sx, &sy, &sh, &sw, true);
			item.offset = Vector2(sx, sy);

			data.push_back(item);
		}
	}

	return true;
}

bool SVGtoMSDF::_access_children(tvg::Iterator *p_it, tvg::IteratorAccessor &p_itr) {
	while (const tvg::Paint *child = p_it->next()) {
		if (!_access_shape(child)) {
			return false;
		}
		if (tvg::Iterator *it2 = p_itr.iterator(child)) {
			if (!_access_children(it2, p_itr)) {
				delete (it2);
				return false;
			}
			delete (it2);
		}
	}

	return true;
}

bool SVGtoMSDF::create_from_string(const String &p_string, int p_pixel_range) {
	data.clear();
	ready = false;
	pixel_range = p_pixel_range;

	std::unique_ptr<tvg::Picture> picture = tvg::Picture::gen();

	PackedByteArray bytes = p_string.to_utf8_buffer();
	tvg::Result result = picture->load((const char *)bytes.ptr(), bytes.size(), "svg", true);
	if (result != tvg::Result::Success) {
		return false;
	}

	ready = _access_shape(picture.get());
	tvg::IteratorAccessor itr;
	if (tvg::Iterator *it = itr.iterator(picture.get())) {
		ready = ready && _access_children(it, itr);
		delete (it);
	}

	return ready;
}

void SVGtoMSDF::draw(RID p_canvas_item, const Rect2 &p_rect, const ::Color &p_modulate) const {
	if (ready) {
		for (const ShapeData &E : data) {
			Rect2 rect = p_rect;
			rect.position += E.offset;

			if (E.stroke_width > 0.f) {
				RenderingServer::get_singleton()->canvas_item_add_msdf_texture_rect_region(p_canvas_item, rect, E.texture->get_rid(), E.uv_rect, E.stroke_color * p_modulate, E.stroke_width, pixel_range);
			}
			RenderingServer::get_singleton()->canvas_item_add_msdf_texture_rect_region(p_canvas_item, rect, E.texture->get_rid(), E.uv_rect, E.fill_color * p_modulate, 0, pixel_range);
		}
	}
}

void SVGtoMSDF::_bind_methods() {
	ClassDB::bind_method(D_METHOD("create_from_string", "string", "pixel_range"), &SVGtoMSDF::create_from_string);
	ClassDB::bind_method(D_METHOD("draw", "canvas_item", "rect", "modulate"), &SVGtoMSDF::draw, DEFVAL(::Color(1, 1, 1)));
}
