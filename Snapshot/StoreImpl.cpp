
#include "Module.h"
#include "StoreImpl.h"

#include <png.h>

namespace WPEFramework {
namespace Plugin {

    bool StoreImpl::R8_G8_B8_A8(const unsigned char* buffer, const unsigned int width, const unsigned int height)
    {
        png_structp pngPointer = nullptr;
        bool result = false;

        pngPointer = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
        if (pngPointer == nullptr) {

            return result;
        }

        png_infop infoPointer = nullptr;
        infoPointer = png_create_info_struct(pngPointer);
        if (infoPointer == nullptr) {

            png_destroy_write_struct(&pngPointer, &infoPointer);
            return result;
        }

        // Set up error handling.
        if (setjmp(png_jmpbuf(pngPointer))) {

            png_destroy_write_struct(&pngPointer, &infoPointer);
            return result;
        }

        // Set image attributes.
        int depth = 8;
        png_set_IHDR(pngPointer,
            infoPointer,
            width,
            height,
            depth,
            PNG_COLOR_TYPE_RGB,
            PNG_INTERLACE_NONE,
            PNG_COMPRESSION_TYPE_DEFAULT,
            PNG_FILTER_TYPE_DEFAULT);

        // Initialize rows of PNG.
        png_byte** rowLines = static_cast<png_byte**>(png_malloc(pngPointer, height * sizeof(png_byte*)));
        const int pixelSize = 4; // RGBA
        for (unsigned int i = 0; i < height; ++i) {

            png_byte* rowLine = static_cast<png_byte*>(png_malloc(pngPointer, sizeof(png_byte) * width * pixelSize));
            const uint8_t* rowSource = buffer + (sizeof(png_byte) * i * width * pixelSize);
            rowLines[i] = rowLine;
            for (unsigned int j = 0; j < width * pixelSize; j += pixelSize) {
                *rowLine++ = rowSource[j + 2]; // Red
                *rowLine++ = rowSource[j + 1]; // Green
                *rowLine++ = rowSource[j + 0]; // Blue
                // ignore alpha
            }
        }

        // Duplicate file descriptor and create File stream based on it.
        FILE* filePointer = static_cast<FILE*>(*_file);
        if (nullptr != filePointer) {
            // Write the image data to "file".
            png_init_io(pngPointer, filePointer);
            png_set_rows(pngPointer, infoPointer, rowLines);
            png_write_png(pngPointer, infoPointer, PNG_TRANSFORM_IDENTITY, nullptr);
            // All went well.
            result = true;
        }

        // Close stream to flush and release allocated buffers
        fclose(filePointer);
        _file->Core::File::LoadFileInfo();
        _file->Core::File::Position(false, 0);

        for (unsigned int i = 0; i < height; i++) {
            png_free(pngPointer, rowLines[i]);
        }

        png_free(pngPointer, rowLines);
        png_destroy_write_struct(&pngPointer, &infoPointer);

        return result;
    }

} // namespace Plugin
}