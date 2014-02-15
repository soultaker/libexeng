/**
 * @file Primitive.hpp
 * @brief Primitive::Enum enumeration
 */


/*
 * Copyright (c) 2013 Felipe Apablaza.
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution.
 */


#ifndef __EXENG_GRAPHICS_PRIMITIVE_HPP__
#define __EXENG_GRAPHICS_PRIMITIVE_HPP__

#include <exeng/TFlags.hpp>
#include <exeng/Enum.hpp>

namespace exeng { namespace graphics {
    /**
     * @brief Rendering primitives
     */
    struct Primitive : public Enum {
        enum Enum {
            PointList       = 0x00001001,   //! Point lists
            
            LineList        = 0x00002001,   //! Line lists.
            LineStrip       = 0x00002002,   //! Line strip.
            LineLoop        = 0x00002003,   //! Line loop
            
            TriangleList    = 0x00004001,   //! Triangle lists
            TriangleStrip   = 0x00004002,   //! Triangle strips
            TriangleFan     = 0x00004003,   //! Triangle fans
            
            // Group markers. Used for check if a given primitive 
            // is a point, line or triangle. They are not valid
            // enumeration entries.
            Point           = 0x00001000,
            Line            = 0x00002000,
            Triangle        = 0x00004000
        };
        
        inline static bool isTriangle(Enum e) {
            return e & Triangle?true:false;
        }
        
        inline static bool isLine(Enum e) {
            return e & Line?true:false;
        }
        
        inline static bool isPoint(Enum e) {
            return e & Point?true:false;
        }
    };
}}

#endif  //__EXENG_GRAPHICS_PRIMITIVE_HPP__
