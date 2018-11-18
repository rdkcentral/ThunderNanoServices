#ifndef __GEOMETRY_IMPLEMENTATION_H
#define __GEOMETRY_IMPLEMENTATION_H

#include "Module.h"

#include <interfaces/ITVControl.h>

namespace WPEFramework {

namespace Player {

namespace Implementation {

class Geometry : public Exchange::IStream::IControl::IGeometry {
private:
    Geometry(const Geometry&) = delete;
    Geometry& operator= (const Geometry&) = delete;

public:
    Geometry()
        : _z(0)
    {
        _rectangle.X      = 0;
        _rectangle.Y      = 0;
        _rectangle.Width  = 0;
        _rectangle.Height = 0;
    }
    Geometry(const uint32_t X, const uint32_t Y, const uint32_t Z, const uint32_t width, const uint32_t height)
        : _z(Z)
    {
        _rectangle.X      = X;
        _rectangle.Y      = Y;
        _rectangle.Width  = width;
        _rectangle.Height = height;
    }
    Geometry(const Exchange::IStream::IControl::IGeometry* copy)
        : _z(copy->Z())
    {
        _rectangle.X      = copy->X();
        _rectangle.Y      = copy->Y();
        _rectangle.Width  = copy->Width();
        _rectangle.Height = copy->Height();
    }
    virtual ~Geometry() {
    }

public:
    //IStream::IControl::IGeometry Interfaces
    virtual uint32_t X() const override {
        return (_rectangle.X);
    }
    virtual uint32_t Y() const override {
        return (_rectangle.Y);
    }
    virtual uint32_t Z() const override {
        return (_z);
    }
    virtual uint32_t Width() const override {
        return (_rectangle.Width);
    }
    virtual uint32_t Height() const override {
        return (_rectangle.Height);
    }
    inline const Rectangle& Window() const {
        return(_rectangle);
    }
    inline void Window(const Rectangle& rectangle) {
        _rectangle = rectangle;
    }
    inline uint32_t Order() const {
        return(_z);
    }
    inline void Order(const uint32_t Z) {
        _z = Z;
    }

    BEGIN_INTERFACE_MAP(Geometry)
       INTERFACE_ENTRY(Exchange::IStream::IControl::IGeometry)
    END_INTERFACE_MAP

private:
    Rectangle _rectangle;
    uint32_t _z;
};

} } } // namespace WPEFramework::Player::Implementation

#endif // __GEOMETRY_IMPLEMENTATION_H
