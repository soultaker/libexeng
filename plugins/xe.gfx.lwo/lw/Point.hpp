
#pragma once

#ifndef __lw_point_hpp__
#define __lw_point_hpp__

#include <xe/Vector.hpp>
#include "Wrapper.hpp"
#include "VMapPt.hpp

namespace lw {
	
	class Point : public Wrapper<Point, ::lwPoint> {
	public:
		Point() {}
		Point(::lwPoint *value) : Wrapper<Point, ::lwPoint>(value) {}

		int npols() const {
			return this->value->npols;
		}

		int nvmaps() const {
			return this->value->nvmaps;
		}

		int pol(const int index) const {
			return this->value->pol[index];
		}

		VMapPt vm(const int index) const {
			return VMapPt(&this->value->vm[index]);
		}

		xe::Vector3f pos() const {
			return xe::Vector3f(&this->value->pos[0]);
		}
	};
}

#endif
