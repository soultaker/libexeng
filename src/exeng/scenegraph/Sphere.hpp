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

#ifndef __EXENG_SCENEGRAPH_SPHERE_HPP__
#define __EXENG_SCENEGRAPH_SPHERE_HPP__

#include <exeng/math/TVector.hpp>

namespace exeng { namespace scenegraph {

class Ray;
struct IntersectInfo;

/**
    * @brief Esfera en el espacio tridimensional.
    */
class Sphere {
public:
    Sphere();
    
    /**
        * @brief Inicializa una esfera en el origen, y con radio arbitrario
        */
    explicit Sphere(float radius);
    
    /**
        * @brief Inicializa una esfera cuyo centro este en la posicion y con el radio indicados.
        * @param radius El radio de la esfera, en unidades de mundo.
        * @param center La posicion en el espacio, en donde estara ubicado el centro de la esfera.
        */
    explicit Sphere(float radius, const exeng::math::Vector3f &center);
    
    /**
        * @brief Inicializa todos los atributos de la esfera de una sola vez.
        */
    auto setAttributes(float radius, const exeng::math::Vector3f &center) -> void;
    
    /**
        * @brief Establece el radio de la esfera
        */
    auto setRadius(float radius) -> void;
    
    /**
        * @brief Establece la posicion en donde estara ubicado el centro de la esfera.
        */
    auto setCenter(const exeng::math::Vector3f &center) -> void;
    
    /**
        * @brief Devuelve el radio actual de la esfera
        */
    auto getRadius() const -> float;
    
    /**
        * @brief Devuelve la posicion en donde esta ubicado el centro de la esfera
        */
    auto getCenter() const -> exeng::math::Vector3f;
    
    /**
        * @brief Calcula la interseccion entre el rayo indicado, y la esfera.
        */
    auto intersect(const Ray& ray, IntersectInfo *intersectInfo=NULL) -> bool;
    
    /**
        * @brief Comprueba si dos esferas son iguales.
        */
    bool operator== (const Sphere &sphere) const;
    
    /**
        * @brief Comprueba si dos esferas son distintas.
        */
    bool operator!= (const Sphere &sphere) const;
    
private:
    exeng::math::Vector3f center;
    float radius;
};


}}

std::ostream& operator<< (std::ostream &os, const exeng::scenegraph::Sphere &sphere);


#include <exeng/scenegraph/Ray.hpp>
#include <exeng/scenegraph/IntersectInfo.hpp>

namespace exeng { namespace scenegraph {
    
inline Sphere::Sphere() {
    this->setAttributes(1.0f, exeng::math::Vector3f(0.0));
}


inline Sphere::Sphere(float radius) {
    this->setAttributes(radius, exeng::math::Vector3f(0.0));
}


inline Sphere::Sphere(float radius, const exeng::math::Vector3f &center) {
    this->setAttributes(radius, center);
}


inline void Sphere::setAttributes(float radius, const exeng::math::Vector3f &center){
    this->setRadius(radius);
    this->setCenter(center);
}


inline auto Sphere::setRadius(float radius) -> void {
    if (radius < 0.0f) {
        radius = 0.0;
    }
    
    this->radius = radius;
}


inline void Sphere::setCenter(const exeng::math::Vector3f &center) {
    this->center = center;
}


inline float Sphere::getRadius() const {
    return this->radius;
}


inline exeng::math::Vector3f Sphere::getCenter() const{
    return this->center;
}


inline auto Sphere::intersect(const Ray& ray, IntersectInfo *intersectInfo) -> bool {
    using namespace exeng::math;
    
    // Variables de apoyo al algoritmo
    float t;
    bool result = false;

    // Variables del modelo matematico
    auto r0 = ray.getPoint();
    auto d = ray.getDirection();

    auto c = this->getCenter();
    auto r = this->getRadius();

    // Temporales para ahorrar calculos
    auto r_2 = r*r;

    auto r0_sub_c = r0 - c;
    auto r0_c_2 = r0_sub_c.getMagnitudeSq();

    // Temporales de la ecuacion cuadratica
    auto B = 2.0f * d.dot(r0_sub_c);
    auto C = r0_c_2 - r_2;

    // Calcular el discriminante
    auto disc = B*B - 4.0f*C;
    
    if (disc == 0.0f) {
        // Solo existe un punto de interseccion (el rayo es tangente a la esfera)
        t = -B / 2.0f;
        
        // Ver si el punto esta "adelante" del rayo, y no detras
        if (t > 0.0f) {
            result = true;
        }
    } else if (disc > 0.0f) {
        // Existen dos puntos de interseccion 
        auto rootDisc = std::sqrt(disc);

        // Determinar el primer punto de interseccion
        float t1 = ( -B - rootDisc ) / 2.0f;
        float t2 = ( -B + rootDisc ) / 2.0f;
        
        t = std::min(t1, t2);

        if (t > 0.0f) {
            result = true;
        }
    } else {
        // El rayo no toca a la esfera, ni por delante no por detras (caso mas comun)
        result = false;
    }
    
    // Rellenar la estructura con la informacion sobre las colisiones
    if (intersectInfo != nullptr) {
        intersectInfo->intersect = result;

        if (intersectInfo->intersect == true) {
            auto normal = ray.getPointAt(t);
            normal.normalize();

            intersectInfo->normal = normal;
            intersectInfo->distance = t;
        }
    }
    
    return result;
}


inline bool Sphere::operator== (const Sphere &sphere) const {
    if (this->getCenter() != sphere.getCenter()) {
        return false;
    }
    
    if (this->getRadius() != sphere.getRadius()) {
        return false;
    }
    
    return true;
}


inline bool Sphere::operator!= (const Sphere &sphere) const {
    return ! (*this == sphere);
}  
}}


inline std::ostream& operator<< (std::ostream &os, const exeng::scenegraph::Sphere &sphere) {
    std::cout << sphere.getCenter() << ", " << sphere.getRadius();
    return os;
}

#endif  //__EXENG_SCENEGRAPH_SPHERE_HPP__
