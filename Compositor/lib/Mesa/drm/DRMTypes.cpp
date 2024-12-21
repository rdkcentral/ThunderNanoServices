#include "DRMTypes.h"

#define _T(x) x
#define TCHAR char

namespace Thunder {
    namespace Compositor {
        namespace DRM {
            static const std::unordered_map<std::string,property> stringToProperty = {
                { _T("Broadcast_RGB"), property::BroadcastRgb },
                { _T("Colorspace"), property::Colorspace },
                { _T("Content Protection"), property::ContentProtection },
                { _T("content type"), property::ContentType },
                { _T("CRTC_ID"), property::CrtcId }, /* atomic */
                { _T("DPMS"), property::Dpms },
                { _T("EDID"), property::Edid },
                { _T("Output_format"), property::OutputFormat },
                { _T("HDR_OUTPUT_METADATA"), property::HdrOutputMetadata },
                { _T("TILE"), property::Tile },
                { _T("link-status"), property::LinkStatus },
                { _T("max bpc"), property::MaxBpc },
                { _T("non-desktop"), property::NonDesktop },
                { _T("PATH"), property::Path },
                { _T("panel_orientation"), property::PanelOrientation },
                { _T("subconnector"), property::SubConnector },
                { _T("vrr_capable"), property::VrrCapable },
                // Plane Properties
                { _T("type"), property::Type },
                { _T("alpha"), property::Alpha },
                { _T("SRC_X"), property::SrcX },
                { _T("SRC_Y"), property::SrcY },
                { _T("SRC_W"), property::SrcW },
                { _T("SRC_H"), property::SrcH },
                { _T("CRTC_X"), property::CrtcX },
                { _T("CRTC_Y"), property::CrtcY },
                { _T("CRTC_W"), property::CrtcW },
                { _T("CRTC_H"), property::CrtcH },
                { _T("FB_DAMAGE_CLIPS"), property::FbDamageClips },
                { _T("FB_ID"), property::FbId },
                { _T("IN_FENCE_FD"), property::InFenceFd },
                { _T("IN_FORMATS"), property::InFormats },
                { _T("rotation"), property::Rotation },
                { _T("zpos"), property::ZPosition },
                // CRTC Properties
                { _T("ACTIVE"), property::Active },
                { _T("CTM"), property::Ctm },
                { _T("DEGAMMA_LUT"), property::DegammaLut },
                { _T("DEGAMMA_LUT_SIZE"), property::DegammaLutSize },
                { _T("GAMMA_LUT"), property::GammaLut },
                { _T("GAMMA_LUT_SIZE"), property::GammaLutSize },
                { _T("MODE_ID"), property::ModeId },
                { _T("VRR_ENABLED"), property::VrrEnabled },
            };

            const TCHAR* Property::PropertyToString() const {
                std::unordered_map<std::string,property>::const_iterator index = stringToProperty.begin();
                while ((index != stringToProperty.end()) && (_property != index->second)) {
                    index++;                    
                }
                return (index != stringToProperty.end() ? index->first.c_str() : _T("UnknownProperty"));
            }

            void Properties::Load(const int descriptor, object_type type, const Identifier objectId)
            {
                _properties.clear();
                _objectId = objectId;

                drmModeObjectProperties* properties = drmModeObjectGetProperties(descriptor, objectId, static_cast<uint32_t>(type));

                if (properties != nullptr) {
                    for (uint32_t i = 0; i < properties->count_props; ++i) {
                        drmModePropertyPtr drmProperty = drmModeGetProperty(descriptor, properties->props[i]);

                        const auto& property = stringToProperty.find(drmProperty->name);

                        if (property != stringToProperty.end()) {
                            
                            _properties.emplace(
                                std::piecewise_construct,
                                std::forward_as_tuple(property->second),
                                std::forward_as_tuple(Element(drmProperty->prop_id, drmProperty->flags)));
                        }

                        drmModeFreeProperty(drmProperty);
                    }

                    drmModeFreeObjectProperties(properties);
                }
            }
        }
    }
}
