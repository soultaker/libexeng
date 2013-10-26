/**
 * @file 
 * @brief 
 */


/*
 * Copyright (c) 2013 Felipe Apablaza.
 *
 * The license and distribution terms for this file may be
 * found in the file LICENSE in this distribution.
 */

#include <cassert>
#include <stdexcept>
#include <boost/checked_delete.hpp>

#include <exeng/scenegraph/Camera.hpp>

using namespace exeng::math;
using namespace exeng::scenegraph;

namespace exeng { namespace scenegraph {
        
/**
    * @brief Atributos y funciones privados de la clase Camera.
    */
struct Camera::Private {
    Private() {
        this->position = Vector3f(0.0f, 0.0f, 1.0f);
        this->up = Vector3f(0.0, 1.0, 0.0);
                
        // Toda la pantalla por defecto
        this->viewport.set(1.0f);
                
    }
            
    Vector3f position;
    Vector3f lookAt;
    Vector3f up;
    Rectf viewport;
    CameraProjection proj;
};
        
const char *invalidArgument = "La posicion no puede ser igual al punto de vision de la camara";
        
        
Camera::Camera() : impl(new Camera::Private()) {
}
		
		
Camera::~Camera() {
    boost::checked_delete(this->impl);
}
		
        
void Camera::setOrientation(const Vector3f &pos, const Vector3f &lookAt) {
    if (pos == lookAt) {
        throw std::invalid_argument(invalidArgument);
    }
            
    this->impl->position = pos;
    this->impl->lookAt = lookAt;
}
        
        
void Camera::setPosition(const Vector3f &pos) {
    assert(this->impl != nullptr);
            
    if (pos == this->impl->lookAt) {
        throw std::invalid_argument(invalidArgument);
    }
            
    this->impl->position = pos;
}
        
        
Vector3f Camera::getPosition() const{
    assert(this->impl != nullptr);
            
    return this->impl->position;
}
        
        
void Camera::setLookAt(const Vector3f &lookAt) {
    assert(this->impl != nullptr);
            
    if (this->impl->position == lookAt) {
        throw std::invalid_argument(invalidArgument);
    }
            
    this->impl->lookAt = lookAt;
}
        
        
Vector3f Camera::getLookAt() const {
    assert(this->impl != nullptr);
            
    return this->impl->lookAt;
}
        
        
Vector3f Camera::getUp() const {
    assert(this->impl != nullptr);
            
    return this->impl->up;
}
        
        
void Camera::setUp(const Vector3f &up) {
    assert(this->impl != nullptr);
    
#if defined(EXENG_DEBUG)
    if (up.isZero()) {
        throw std::invalid_argument("exeng::scenegraph::Camera::setUp: The length of the up vector can't be zero.");
    }
#endif
    
    this->impl->up = up;
    this->impl->up.normalize();
}
        
        
void Camera::setViewport(const Rectf &viewport) {
    assert(this->impl != nullptr);

    if (viewport.getSize() == Size2f(0.0, 0.0)) {   
        throw std::invalid_argument("El tamanio debe tener todos sus valores positivos");
    }
            
    this->impl->viewport = viewport;
}
        
        
Rectf Camera::getViewport() const {
    assert(this->impl != nullptr);
    return this->impl->viewport;
}
        
        
void Camera::setProjection(const CameraProjection &proj) {
    assert(this->impl != nullptr);
    this->impl->proj = proj;
}
        
        
CameraProjection Camera::getProjection() const {
    assert(this->impl != nullptr);
            
    return this->impl->proj;
}

}}
