#pragma once
#include <raylib.h>
namespace game {
inline int AtlasX(int tile,int width){return tile%(width/16)*16;}
inline int AtlasY(int tile,int width){return tile/(width/16)*16;}
inline Rectangle AtlasTile(Texture2D atlas,int tile){return {float(AtlasX(tile,atlas.width)),float(AtlasY(tile,atlas.width)),16,16};}
}
