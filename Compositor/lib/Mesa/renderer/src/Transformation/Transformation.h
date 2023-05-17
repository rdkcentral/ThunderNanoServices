/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 Metrological B.V.
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

#include <IRenderer.h>
#include <array>
#include <math.h>

namespace WPEFramework {
namespace Compositor {
    namespace Transformation {

        using TransformType = enum : uint8_t {
            TRANSFORM_NORMAL = 0, // No transform
            TRANSFORM_90 = 1, // 90 degrees counter-clockwise
            TRANSFORM_180 = 2, //  180 degrees counter-clockwise
            TRANSFORM_270 = 3, // 270 degrees counter-clockwise
            TRANSFORM_FLIPPED = 4, // 180 degree flip around a vertical axis
            TRANSFORM_FLIPPED_90 = 5, // Flip and rotate 90 degrees counter-clockwise
            TRANSFORM_FLIPPED_180 = 6, // Flip and rotate 180 degrees counter-clockwise
            TRANSFORM_FLIPPED_270 = 7, // Flip and rotate 270 degrees counter-clockwise
            TRANSFORM_MAX // Always last
        };

        /**
         * @brief Default transformations
         */
        static const std::array<Matrix, TRANSFORM_MAX> Transformations = {
            Matrix({
                // [TRANSFORM_NORMAL]
                1.0f, 0.0f, 0.0f, //
                0.0f, 1.0f, 0.0f, //
                0.0f, 0.0f, 1.0f //
            }),
            Matrix({
                // [TRANSFORM_90]
                0.0f, 1.0f, 0.0f, //
                -1.0f, 0.0f, 0.0f, //
                0.0f, 0.0f, 1.0f //
            }),
            Matrix({
                // [TRANSFORM_180]
                -1.0f, 0.0f, 0.0f, //
                0.0f, -1.0f, 0.0f, //
                0.0f, 0.0f, 1.0f //
            }),
            Matrix({
                // [TRANSFORM_270]
                0.0f, -1.0f, 0.0f, //
                1.0f, 0.0f, 0.0f, //
                0.0f, 0.0f, 1.0f //
            }),
            Matrix({
                // [TRANSFORM_FLIPPED]
                -1.0f, 0.0f, 0.0f, //
                0.0f, 1.0f, 0.0f, //
                0.0f, 0.0f, 1.0f //
            }),
            Matrix({
                // [TRANSFORM_FLIPPED_90]
                0.0f, 1.0f, 0.0f, //
                1.0f, 0.0f, 0.0f, //
                0.0f, 0.0f, 1.0f //
            }),
            Matrix({
                // [TRANSFORM_FLIPPED_180]
                1.0f, 0.0f, 0.0f, //
                0.0f, -1.0f, 0.0f, //
                0.0f, 0.0f, 1.0f //
            }),
            Matrix({
                // [TRANSFORM_FLIPPED_270]
                0.0f, -1.0f, 0.0f, //
                -1.0f, 0.0f, 0.0f, //
                0.0f, 0.0f, 1.0f //
            })
        };

        /**
         * Initializes @matrix with a identity matrix.
         */
        static inline void Identity(Matrix& matrix)
        {
            matrix = Transformations[TRANSFORM_NORMAL];
        }

        /** mat ← a × b */
        static inline void Multiply(Matrix& matrix, const Matrix& a, const Matrix& b)
        {
            Matrix multiply = {
                (a[0] * b[0]) + (a[1] * b[3]) + (a[2] * b[6]), //
                (a[0] * b[1]) + (a[1] * b[4]) + (a[2] * b[7]), //
                (a[0] * b[2]) + (a[1] * b[5]) + (a[2] * b[8]), //
                (a[3] * b[0]) + (a[4] * b[3]) + (a[5] * b[6]), //
                (a[3] * b[1]) + (a[4] * b[4]) + (a[5] * b[7]), //
                (a[3] * b[2]) + (a[4] * b[5]) + (a[5] * b[8]), //
                (a[6] * b[0]) + (a[7] * b[3]) + (a[8] * b[6]), //
                (a[6] * b[1]) + (a[7] * b[4]) + (a[8] * b[7]), //
                (a[6] * b[2]) + (a[7] * b[5]) + (a[8] * b[8]), //
            };

            matrix = multiply;
        }

        /**
         * The transpose of a matrix is obtained by changing the rows into columns
         * and columns into rows for a given matrix.
         */
        static inline void Transpose(Matrix& matrix, const Matrix& a)
        {
            Matrix transpose = {
                a[0], a[3], a[6], //
                a[1], a[4], a[7], //
                a[2], a[5], a[8], //
            };

            matrix = transpose;
        }

        /**
         * Writes a 2D translation matrix to mat of magnitude (x, y)
         * The translate() method moves an element from its current position
         */
        static inline void Translate(Matrix& matrix, const float x, const float y)
        {
            Matrix translate = {
                1.0f, 0.0f, x, //
                0.0f, 1.0f, y, //
                0.0f, 0.0f, 1.0f, //
            };

            Multiply(matrix, matrix, translate);
        }

        /**
         * Writes a 2D scale matrix to matrix of magnitude (x, y)
         */
        static inline void Scale(Matrix& matrix, const float x, const float y)
        {
            Matrix scale = {
                x, 0.0f, 0.0f, //
                0.0f, y, 0.0f, //
                0.0f, 0.0f, 1.0f, //
            };

            Multiply(matrix, matrix, scale);
        }

        /**
         * Simple conversion from degree to radials
         */
        static inline float ToRadials(float degree)
        {
            constexpr float pi = 3.1415926536f;
            return degree * (pi / 180.0f);
        };

        /**
         *  Writes a 2D rotation matrix to @matrix at an angle of @radians radians
         */
        static inline void Rotate(Matrix& matrix, const float radians)
        {
            Matrix rotate = {
                ::cos(radians), -::sin(radians), 0.0f, //
                ::sin(radians), ::cos(radians), 0.0f, //
                0.0f, 0.0f, 1.0f, //
            };

            Multiply(matrix, matrix, rotate);
        }

        /**
         * Writes a transformation matrix which applies the specified
         * TransformType @transform to @matrix
         */
        static inline void Transform(Matrix& matrix, const TransformType transform)
        {
            Multiply(matrix, matrix, Transformations[transform]);
        }

        /**
         * Writes a 2D orthographic projection matrix to mat of (width, height) with a
         * specified transform.
         *
         * Equivalent to glOrtho(0, width, 0, height, 1, -1) with the transform applied.
         */
        static inline void Projection(Matrix& result, const uint32_t width, const uint32_t height, const TransformType transform)
        {
            result.fill(0);

            Matrix t = Transformations[transform];

            float x = 2.0f / width;
            float y = 2.0f / height;

            // Rotation + reflection
            result[0] = x * t[0];
            result[1] = x * t[1];
            result[3] = y * -t[3];
            result[4] = y * -t[4];

            // Translation
            result[2] = -copysign(1.0f, result[0] + result[1]);
            result[5] = -copysign(1.0f, result[3] + result[4]);

            // Identity
            result[8] = 1.0f;
        }

        /**
         *  Shortcut for the various matrix operations involved in projecting the
         *  specified Box onto a given orthographic projection with a given
         *  rotation. The result is written to result, which can be applied to each
         *  coordinate of the box to get a new coordinate from [-1,1].
         */
        static inline void ProjectBox(Matrix& result, const Box& box, const TransformType transform, const float radians, const Matrix& projection)
        {
            float x = box.x;
            float y = box.y;
            float width = box.width;
            float height = box.height;

            Identity(result);
            Translate(result, x, y);

            if (radians != 0) {
                Translate(result, width / 2.0f, height / 2.0f);
                Rotate(result, radians);
                Translate(result, -width / 2.0f, -height / 2.0f);
            }

            Scale(result, width, height);

            if (transform != TRANSFORM_NORMAL) {
                Translate(result, 0.5f, 0.5f);
                Transform(result, transform);
                Translate(result, -0.5f, -0.5f);
            }

            Multiply(result, projection, result);
        }
    } // namespace Transformation
} // namespace Compositor
} // namespace WPEFramework
