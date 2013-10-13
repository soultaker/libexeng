
#include "RayTracerApp.hpp"

#include "SphereGeometry.hpp"
#include "samplers/JitteredSampler.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <cstdint>
#include <list>

#include <exeng/graphics/HeapVertexBuffer.hpp>

namespace raytracer {

using namespace exeng;
using namespace exeng::math;
using namespace exeng::scenegraph;
using namespace exeng::graphics;
using namespace exeng::framework;
using namespace exeng::input;

using namespace raytracer::samplers;

RayTracerApp::RayTracerApp() {
    this->defaultColor = 0xFF000000;
    this->backbuffer = nullptr;
    this->applicationStatus = ApplicationStatus::Running;
    this->cameraView.size = Size2i(320, 200);

    this->lastTime = Timer::getTime();

    this->fpsCurrent = 0;
    this->fpsLastTime = 0.0;
    this->fpsCurrentTime = 0.0;

	this->eye = Vector3f(0.0f, 0.0f, -75.0f);
}


RayTracerApp::~RayTracerApp() {
    this->terminate();
}


void RayTracerApp::initialize(const StringVector& cmdLine) {
    // Initialize the exeng root class and plugins.
    std::string path;

#ifdef EXENG_DEBUG
	path = "../../bin/Debug/";
#else
    path = "../../bin/Release/";
#endif

    this->root.reset(new Root());
    this->root->getPluginManager()->load("exeng-graphics-gl3", path);
    
    // initialize the gl3 driver, in windowed mode
    this->driver.reset(this->root->getGraphicsManager()->createDriver());
    this->driver->addEventHandler(this);
    
    DisplayMode mode;
    mode.size = cameraView.size;
    mode.redBits = 8;
    mode.greenBits = 8;
    mode.blueBits = 8;
    mode.alphaBits = 8;
    mode.status = DisplayStatus::Window;
    mode.depthBits = 16;
    mode.stencilBits = 0;
    
    this->driver->initialize(mode);
    
    // create the geometry (a single triangle)
    auto vertexBuffer = this->driver->createVertexBuffer(VertexFormat::makeStandardVertex(), 4);
    {
        struct Vertex {
            Vector3f coord;
            Vector2f texCoord;
        };
        
        VertexArray<Vertex> array(vertexBuffer);
        
        array[0].coord = Vector3f(-1.0f,  1.0f, 0.0f);
        array[0].texCoord = Vector2f(0.0f,  1.0f);
        
        array[1].coord = Vector3f( 1.0f,  1.0f, 0.0f);
        array[1].texCoord = Vector2f(1.0f,  1.0f);
        
        array[2].coord = Vector3f(-1.0f, -1.0f, 0.0f);
        array[2].texCoord = Vector2f(0.0f,  0.0f);
        
        array[3].coord = Vector3f( 1.0f, -1.0f, 0.0f);
        array[3].texCoord = Vector2f(1.0f,  0.0f);
    }
    this->vertexBuffer.reset(vertexBuffer);
    
    // create a texture for the render targets
	auto texture = this->driver->createTexture(
		TextureType::Tex2D, 
		Vector3f(static_cast<float>(mode.size.width), 
		static_cast<float>(mode.size.height)), 
		ColorFormat::getColorFormatR8G8B8A8()
	);
    
    struct Texel {
        std::uint8_t red;
        std::uint8_t green;
        std::uint8_t blue;
        std::uint8_t alpha;
    };
	
    Texel *textureData = reinterpret_cast<Texel*>(texture->lock());
    for (int i=0; i<mode.size.width * mode.size.height; ++i) {
        textureData[i].red      = 255;
        textureData[i].green    = 255;
        textureData[i].blue     = 255;
        textureData[i].alpha    = 255;
    }
    texture->unlock();
    
    this->texture.reset(texture);
    
    this->material.reset( new exeng::graphics::Material() );
    this->material->getLayer(0)->setTexture(texture);
    
    this->backbuffer = nullptr;
    
    this->sampler.reset(new JitteredSampler(25));
    this->scene.reset(new Scene());
    
    this->loadScene();    
}


double RayTracerApp::getFrameTime() const {
    uint32_t lastTime = Timer::getTime();
    double frameTime = static_cast<double>(lastTime - this->lastTime) / 1000.0;

    this->lastTime = lastTime;

    return frameTime;
}


void RayTracerApp::pollEvents() {
    this->driver->pollEvents();
}


ApplicationStatus RayTracerApp::getStatus() const {
    return this->applicationStatus;
}


void RayTracerApp::update(double seconds) {
    // Actualizar los cuadros por segundo
    if (this->fpsCurrentTime >= 1.0) {
        uint32_t fps = this->fpsCurrent;
        this->fpsCurrent = 0;
        this->fpsCurrentTime = 0.0;

        std::cout << "FPS: " << fps << std::endl;
    } else {
        this->fpsCurrent += 1;
        this->fpsCurrentTime += seconds;
    }
}


void RayTracerApp::render() {
    SceneNodeList nodeList;
    Vector2i screenSize = this->cameraView.size;
    Vector2i pixel;
    Color pixelColor;
    
    clear();
    flattenHierarchy(nodeList, this->scene->getRootNodePtr());
    
    for(pixel.y=0; pixel.y<screenSize.y; ++pixel.y) {
        for(pixel.x=0; pixel.x<screenSize.x; ++pixel.x) {
            
            pixelColor = traceRay(nodeList, pixel);
            
            // Pintar el backbuffer 
            std::swap(pixelColor[0], pixelColor[1]);
            putPixel(pixel, static_cast<uint32_t>(pixelColor));
        }
    }

    present();
}


int RayTracerApp::getExitCode() const {
    return 0;
}


void RayTracerApp::terminate() {
    this->driver->terminate();
}


uint32_t RayTracerApp::pointToOffset(const Vector2i &point) const {
#ifdef EXENG_DEBUG
    if (point.x < 0 || point.y < 0) {
        throw std::invalid_argument("");
    }
    
    if (point.x >= this->cameraView.size.width) {
        throw std::invalid_argument("");
    }
    
    if (point.y >= this->cameraView.size.height) {
        throw std::invalid_argument("");
    }
#endif  
    int offset = point.y * this->cameraView.size.width + point.x;
    return static_cast<uint32_t>(offset);
}


void RayTracerApp::putPixel(const Vector2i &point, uint32_t color) {
    uint32_t offset = pointToOffset(point);
    uint32_t *data = static_cast<uint32_t*>(this->backbuffer);
    
    data += offset;
    
    *data = color;
}


std::uint32_t RayTracerApp::getPixel(const Vector2i &point) const {
    auto offset = pointToOffset(point);
    auto data = static_cast<uint32_t*>(this->backbuffer);
    
    data += offset;
    
    return *data;
}


/*
Ray RayTracerApp::castRay(const Vector2f &pixel) const {
    const float pixelSize = this->cameraView.pixelSize;
    const float halfWidth = this->cameraView.size.width*0.5f;
    const float halfHeight = this->cameraView.size.height*0.5f;
    
    Ray ray;
    
    // Trazar un rayo
    float x = pixelSize * (pixel.x - halfWidth + 0.5f);
    float y = pixelSize * (pixel.y - halfHeight + 0.5f);
    float z = -50.0f;
    
    ray.setPoint(Vector3f(x, y, z));
    ray.setDirection(Vector3f(0.0f, 0.0f, 1.0f));

    return ray;
}
*/


Ray RayTracerApp::castRay(const Vector2f &pixel) const {
	// trazar el rayo usando una proyeccion ortografica
    const float pixelSize = this->cameraView.pixelSize;
    const float halfWidth = (this->cameraView.size.width - 1) * 0.5f;
    const float halfHeight = (this->cameraView.size.height - 1) * 0.5f;
	const float d = -150.0f;

    Ray ray;
    
    // Trazar un rayo
    float x = pixelSize * (pixel.x - halfWidth + 0.5f);
    float y = pixelSize * (pixel.y - halfHeight + 0.5f);
    float z = -d;
    
	ray.setPoint(this->eye);
    ray.setDirection(Vector3f(x, y, z));

    return ray;
}


/*
Ray RayTracerApp::castRay(const Vector2f &pixel, const Vector2f &sample) const {
    const float pixelSize = this->cameraView.pixelSize;
    const float halfWidth = this->cameraView.size.width*0.5f;
    const float halfHeight = this->cameraView.size.height*0.5f;
    
    Ray ray;

    // Trazar un rayo
    float x = pixelSize * (pixel.x - halfWidth + 0.5f + sample.x);
    float y = pixelSize * (pixel.y - halfHeight + 0.5f + sample.y);
    float z = -50.0f;
    
    // Hacia adelante
    ray.setPoint( Vector3f(x, y, z) );
    ray.setDirection( Vector3f(0.0f, 0.0f, 1.0f) );

    return ray;
}
*/


void RayTracerApp::flattenHierarchy(SceneNodeList &out, SceneNode* node) const {
    // Poner los nodos de escena
    if (node != nullptr && node->getDataPtr() != nullptr) {
        if (node->getDataPtr()->getTypeInfo() == TypeInfo::get<Geometry>())  {
            out.push_back(node);
        }
    }

    for (auto child : node->getChilds()) {
        this->flattenHierarchy(out, child);
    }
}


IntersectInfo RayTracerApp::intersectRay(const SceneNodeList &nodes, const Ray &ray) const {
    IntersectInfo prevInfo, currentInfo, info;

    // Determinar colision con el contenido del nodo
    for (auto node : nodes) {
        Geometry* geometry = static_cast<Geometry*>( node->getDataPtr() );
        assert(geometry != nullptr);

        if (geometry->hit(ray, &currentInfo) == true) {
            if (prevInfo.distance == 0.0f || currentInfo.distance > prevInfo.distance) {
                info = currentInfo;
                
                assert(info.materialPtr != nullptr);
                assert(info.normal != Vector3f(0.0f));
            }

            prevInfo = currentInfo;
        }
    }

    return info;
}


Color RayTracerApp::traceRay(const SceneNodeList &sceneNodeList, const Vector2i &pixel) const {
    Color color(0.0f, 0.0f, 0.0f, 1.0f);
    Vector2f pixelSample = static_cast<Vector2f>(pixel);
    Ray ray = castRay(pixel);
        
    // Intersectarlo con los objetos de la escena
    IntersectInfo info = intersectRay(sceneNodeList, ray);
        
    if (info.intersect == true)  {

        // Determinar el color
        auto factor = info.normal.dot(ray.getDirection());
        auto vcolor = info.materialPtr->getProperty4f("diffuse");

        color = Color(vcolor) * factor;
		if (color.red < 0.0f)	color.red = 0.0f;
		if (color.green < 0.0f)	color.green = 0.0f;
		if (color.blue < 0.0f)	color.blue = 0.0f;
		if (color.alpha < 0.0f)	color.alpha = 0.0f;
    } else {
        color = this->scene->getBackgroundColor();
    }

    return color;
}


/*
Color RayTracerApp::traceRayMultisampled(const SceneNodeList &sceneNodeList, const Vector2i &pixel) const {
    Color color(0.0f, 0.0f, 0.0f, 1.0f);
    
    Vector2f pixelSample = static_cast<Vector2f>(pixel);
    
    for (int i=0; i<this->sampler->getSampleCount(); ++i) {
        // Trazar un rayo
        Vector2f sample = this->sampler->sampleUnitSquare();
        Ray ray = castRay(pixelSample, sample);
        
        // Intersectarlo con los objetos de la escena
        IntersectInfo info = intersectRay(sceneNodeList, ray);
        
        if (info.intersect == true)  {

            // Determinar el color
            auto factor = info.normal.dot(ray.getDirection());
            auto vcolor = info.materialPtr->getProperty4f("diffuse");
            
            color += Color(vcolor) * factor;
        } else {
            color += this->scene->getBackgroundColor();
        }
    }
    
    float sampleCountf = static_cast<float>(this->sampler->getSampleCount());
    return color / sampleCountf;
}
*/


void RayTracerApp::clear() {
    this->driver->beginFrame(Color(0.0f, 0.0f, 0.0f, 1.0f));
    this->backbuffer = this->texture->lock();
}


void RayTracerApp::present() {
    this->texture->unlock();
    this->driver->setMaterial(this->material.get());
    this->driver->setVertexBuffer(this->vertexBuffer.get());
    this->driver->render(Primitive::TriangleStrip, 4);
    this->driver->endFrame();
}


void RayTracerApp::loadScene() {
    // Crear una escena de juguete, con una esfera al centro de la escena.
    // TODO: Cargar esta escena desde un archivo XML, o similar
    auto rootNode = this->scene->getRootNodePtr();
    auto sphereGeometry = new SphereGeometry();
    auto sphereGeometry2 = new SphereGeometry();
    
    sphereGeometry->sphere.setAttributes(25.0f, Vector3f(-50.0f, 0.0f, 0.0f));
    sphereGeometry->material.setProperty("diffuse", Vector4f(1.0f, 0.5f, 0.25f, 1.0f));
    
    sphereGeometry2->sphere.setAttributes(40.0f, Vector3f(40.0f, 0.0f, 0.0f));
    sphereGeometry2->material.setProperty("diffuse", Vector4f(0.0f, 0.0f, 1.0f, 1.0f));

    // sphereGeometry3->sphere.setAttributes(200.0, Vector3f(0.0f, 100.0f, 0.0f));
    // sphereGeometry3->material.setProperty("diffuse", Vector4f(0.0f, 1.0f, 0.0f, 1.0f));
	// auto vertexBuffer = new HeapVertexBuffer(nullptr, 

    rootNode->addChildPtr("sphereGeometry")->setDataPtr(sphereGeometry);
    rootNode->addChildPtr("sphereGeometry2")->setDataPtr(sphereGeometry2);
    // rootNode->addChildPtr("sphereGeometry3")->setDataPtr(sphereGeometry3);

	Mesh *mesh = new Mesh(1);

	VertexBuffer *vertexBuffer = new HeapVertexBuffer(nullptr, VertexFormat::makeStandardVertex(), 3);
	{
		VertexArray<StandardVertex> vertices(vertexBuffer);

		vertices[0].coord = Vector3f(-1.0f, 0.0f, 0.0f);
		vertices[0].normal = Vector3f(0.0f, 0.0f, -1.0f);
		vertices[0].texCoord = Vector2f(0.0f, 0.0f);

		vertices[1].coord = Vector3f(0.0f, 1.0f, 0.0f);
		vertices[1].normal = Vector3f(0.0f, 0.0f, -1.0f);
		vertices[1].texCoord = Vector2f(1.0f, 0.0f);

		vertices[2].coord = Vector3f(1.0f, 0.0f, 0.0f);
		vertices[2].normal = Vector3f(0.0f, 0.0f, -1.0f);
		vertices[2].texCoord = Vector2f(0.0f, 1.0f);
	}
	
	mesh->getPart(0)->setVertexBuffer(vertexBuffer);
	mesh->getPart(0)->setPrimitiveType(Primitive::TriangleList);

	rootNode->addChildPtr("triangleMesh")->setDataPtr(mesh);

}


void RayTracerApp::handleEvent(const EventData &data) {
	if (data.eventType == TypeInfo::get<InputEventData>()) {
		const InputEventData &inputEventData = data.cast<InputEventData>();

		if (inputEventData.getButtonStatus(ButtonCode::KeyEsc) == ButtonStatus::Press) {
			this->applicationStatus = ApplicationStatus::Terminated;
		}
		if (inputEventData.getButtonStatus(ButtonCode::KeyUp) == ButtonStatus::Press) {
			this->eye.z += 2.5f;
		}
		if (inputEventData.getButtonStatus(ButtonCode::KeyDown) == ButtonStatus::Press) {
			this->eye.z -= 2.5f;
		}
		if (inputEventData.getButtonStatus(ButtonCode::KeyRight) == ButtonStatus::Press) {
			this->eye.x += 2.5f;
		}
		if (inputEventData.getButtonStatus(ButtonCode::KeyLeft) == ButtonStatus::Press) {
			this->eye.x -= 2.5f;
		}
	} else if (data.eventType == TypeInfo::get<CloseEventData>()) {
		this->applicationStatus = ApplicationStatus::Terminated;
	}
}

}

EXENG_IMPLEMENT_MAIN(raytracer::RayTracerApp)
