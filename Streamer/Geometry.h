/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#pragma once

#include "Module.h"

#include <interfaces/IStream.h>

namespace WPEFramework {

namespace Player {

    namespace Implementation {

        struct Rectangle {
            uint32_t X;
            uint32_t Y;
            uint32_t Width;
            uint32_t Height;
        };

        class Geometry : public Exchange::IStream::IControl::IGeometry {
        private:
            Geometry(const Geometry&) = delete;
            Geometry& operator=(const Geometry&) = delete;

        public:
            Geometry()
                : _z(0)
            {
                _rectangle.X = 0;
                _rectangle.Y = 0;
                _rectangle.Width = 0;
                _rectangle.Height = 0;
            }
            Geometry(const uint32_t X, const uint32_t Y, const uint32_t Z, const uint32_t width, const uint32_t height)
                : _z(Z)
            {
                _rectangle.X = X;
                _rectangle.Y = Y;
                _rectangle.Width = width;
                _rectangle.Height = height;
            }
            Geometry(const Exchange::IStream::IControl::IGeometry* copy)
                : _z(copy->Z())
            {
                _rectangle.X = copy->X();
                _rectangle.Y = copy->Y();
                _rectangle.Width = copy->Width();
                _rectangle.Height = copy->Height();
            }
            ~Geometry() override
            {
            }

        public:
            //IStream::IControl::IGeometry Interfaces
            uint32_t X() const override
            {
                return (_rectangle.X);
            }
            uint32_t Y() const override
            {
                return (_rectangle.Y);
            }
            uint32_t Z() const override
            {
                return (_z);
            }
            uint32_t Width() const override
            {
                return (_rectangle.Width);
            }
            uint32_t Height() const override
            {
                return (_rectangle.Height);
            }
            inline const Rectangle& Window() const
            {
                return (_rectangle);
            }
            inline void Window(const Rectangle& rectangle)
            {
                _rectangle = rectangle;
            }
            inline uint32_t Order() const
            {
                return (_z);
            }
            inline void Order(const uint32_t Z)
            {
                _z = Z;
            }

            BEGIN_INTERFACE_MAP(Geometry)
            INTERFACE_ENTRY(Exchange::IStream::IControl::IGeometry)
            END_INTERFACE_MAP

        private:
            Rectangle _rectangle;
            uint32_t _z;
        };
    }
}
}
