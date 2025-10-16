#pragma once

#include "CompositorTypes.h"
#include <xf86drm.h>
#include <drm_fourcc.h>
#include <iomanip>

namespace Thunder {
namespace Compositor {
    inline std::string ToString(const PixelFormat& format)
    {
        std::stringstream result;

        char* formatName = drmGetFormatName(format.Type());

        result << (formatName ? formatName : "UNKNOWN")
               << " [0x" << std::uppercase << std::setfill('0') << std::setw(8) << std::hex << format.Type() << std::dec << "]"
               << " - " << format.Modifiers().size() << " modifiers:";

        if (formatName) {
            free(formatName);
        }

        for (size_t modIndex = 0; modIndex < format.Modifiers().size(); ++modIndex) {
            uint64_t modifier = format.Modifiers()[modIndex];
            char* modifierName = drmGetFormatModifierName(modifier);

            result << "\n    [" << modIndex << "] ";

            if (modifierName) {
                result << modifierName << " ";
                free(modifierName);
            } else {
                result << "UNKNOWN ";
            }

            result << "[0x" << std::uppercase << std::setfill('0') << std::setw(16) << std::hex << modifier << std::dec << "]";

            if (modifier == DRM_FORMAT_MOD_LINEAR) {
                result << " (LINEAR)";
            } else if (modifier == DRM_FORMAT_MOD_INVALID) {
                result << " (INVALID/DEFAULT/AUTO)";
            } else {
                const char* vendorName = drmGetFormatModifierVendor(modifier);

                if (vendorName) {
                    result << " (" << vendorName << ")";
                } else {
                    result << " (UNKNOWN)";
                }
            }
        }

        return result.str();
    }

    inline std::string ToString(const std::vector<PixelFormat>& formats)
    {
        std::stringstream result;

        result << "Total formats: " << formats.size() << std::endl;

        for (size_t formatIndex = 0; formatIndex < formats.size(); ++formatIndex) {
            result << " [" << formatIndex << "] " << ToString(formats[formatIndex]);

            if (formatIndex < formats.size() - 1) {
                result << std::endl;
            }
        }

        return result.str();
    }

    // qualities sorted from high to low
    constexpr uint32_t FormatPreferenceTable[] = {
        DRM_FORMAT_ARGB8888, // 32bit, alpha
        DRM_FORMAT_XRGB8888, // 32bit, no alpha
        DRM_FORMAT_RGB565 // 16bit, no alpha
    };

    inline PixelFormat IntersectFormat(const PixelFormat& requested,
        const std::vector<const std::vector<PixelFormat>*>& formatLists,
        const bool qualityPreference = true)
    {
        PixelFormat selectedFormat(DRM_FORMAT_INVALID, { DRM_FORMAT_MOD_INVALID });

        if (formatLists.empty()) {
            return selectedFormat;
        }

        // Determine which format types to search for
        std::vector<uint32_t> candidateFormats;

        if (requested.Type() == DRM_FORMAT_INVALID) {
            // Use preference table
            const size_t tableSize = sizeof(FormatPreferenceTable) / sizeof(FormatPreferenceTable[0]);

            if (qualityPreference) {
                // High to low quality
                for (size_t i = 0; i < tableSize; ++i) {
                    candidateFormats.push_back(FormatPreferenceTable[i]);
                }
            } else {
                // Low to high quality
                for (int i = static_cast<int>(tableSize) - 1; i >= 0; --i) {
                    candidateFormats.push_back(FormatPreferenceTable[i]);
                }
            }
        } else {
            // Use explicit format
            candidateFormats.push_back(requested.Type());
        }

        // Search for intersection
        for (uint32_t candidateType : candidateFormats) {
            // Find this format type in all format lists
            std::vector<const PixelFormat*> matchingFormats;
            bool foundInAllLists = true;

            for (const std::vector<PixelFormat>* formatList : formatLists) {
                const PixelFormat* found = nullptr;

                for (const PixelFormat& format : *formatList) {
                    if (format.Type() == candidateType) {
                        found = &format;
                        break;
                    }
                }

                if (found == nullptr) {
                    foundInAllLists = false;
                    break;
                }

                matchingFormats.push_back(found);
            }

            if (!foundInAllLists) {
                continue; // Try next candidate format
            }

            // We found the format type in all lists, now find ALL common modifiers
            std::vector<uint64_t> commonModifiers;

            // Start with modifiers from first format
            if (!matchingFormats.empty()) {
                commonModifiers = matchingFormats[0]->Modifiers();

                // Intersect with other formats' modifiers
                for (size_t i = 1; i < matchingFormats.size(); ++i) {
                    const std::vector<uint64_t>& currentModifiers = matchingFormats[i]->Modifiers();
                    std::vector<uint64_t> intersection;

                    for (uint64_t mod : commonModifiers) {
                        if (std::find(currentModifiers.begin(), currentModifiers.end(), mod) != currentModifiers.end()) {
                            intersection.push_back(mod);
                        }
                    }

                    commonModifiers = intersection;

                    if (commonModifiers.empty()) {
                        break; // No common modifiers
                    }
                }
            }

            if (commonModifiers.empty()) {
                continue; // Try next candidate format
            }

            // Return ALL common modifiers - let GBM choose the best one
            selectedFormat = PixelFormat(candidateType, std::move(commonModifiers));
            return selectedFormat;
        }

        return selectedFormat; // Returns invalid format
    }
}
}