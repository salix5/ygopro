#ifndef _C_GUI_IMAGE_BUTTON_H_
#define _C_GUI_IMAGE_BUTTON_H_

#include <irrlicht.h>

namespace irr {
namespace gui {

void Draw2DImageRotation(video::IVideoDriver* driver, video::ITexture* image, core::rect<s32> sourceRect,
                         core::position2d<s32> position, core::position2d<s32> rotationPoint, f32 rotation = 0.0f,
                         core::vector2df scale = core::vector2df(1.0, 1.0), bool useAlphaChannel = true, video::SColor color = 0xffffffff);
void Draw2DImageQuad(video::IVideoDriver* driver, video::ITexture* image, core::rect<s32> sourceRect,
                     core::position2d<s32> corner[4], bool useAlphaChannel = true, video::SColor color = 0xffffffff);
}
}

#endif //_C_GUI_IMAGE_BUTTON_H_
