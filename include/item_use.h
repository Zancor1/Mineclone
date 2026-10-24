#pragma once
#include "block.h"
namespace game {
inline bool PotionItem(Block item){return item>=SpeedPotion&&item<=InvisibilityPotion;}
inline bool TimedUseItem(Block item){return item==GoldenApple||PotionItem(item);}
inline float ItemUseDuration(Block item){return item==GoldenApple?1.61f:1.6f;}
constexpr float ItemUseMovement=.2f;
}
