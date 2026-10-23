#include "world.h"
#include <cstdio>
#include <cstdlib>

namespace {
using namespace game;
void Check(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}
void ClearWorld() {
    for (int x = 0; x < Width; ++x)
        for (int y = 0; y < Height; ++y)
            for (int z = 0; z < Width; ++z)
                if (At(x, y, z) != Air) Change(x, y, z, Air);
}
void Tick(int count = 1) {
    for (int i = 0; i < count; ++i) UpdateWater(0.15f);
}
void Corridor() {
    ClearWorld();
    for (int x = 8; x <= 17; ++x) {
        Change(x, 7, 8, Stone);
        Change(x, 8, 7, Stone);
        Change(x, 8, 9, Stone);
    }
    Change(7, 8, 8, Stone);
    Change(8, 8, 8, Water);
}
}
int main() {
    using namespace game;
    ClearWorld();
    Change(8, 8, 8, Water);
    Tick();
    Check(At(8, 7, 8) == Water && WaterLevelAt(8, 7, 8) == 7, "gravity creates level seven below");
    Check(At(9, 8, 8) == Air && At(7, 8, 8) == Air, "gravity suppresses lateral flow");
    Check(At(8, 6, 8) == Air, "new cells wait for next tick");
    Tick();
    Check(At(8, 6, 8) == Water, "fall continues in next tick");

    ClearWorld();
    Change(8, 7, 8, Stone);
    Change(7, 7, 8, Stone);
    Change(8, 7, 7, Stone);
    Change(8, 7, 9, Stone);
    Change(8, 8, 8, Water);
    Tick();
    Check(WaterLevelAt(9, 8, 8) == 6, "adjacent hole receives source level minus one");
    Check(At(7, 8, 8) == Air && At(8, 8, 9) == Air, "drop wins over equally empty flat neighbors");
    Tick();
    Check(WaterLevelAt(9, 7, 8) == 7 && At(10, 8, 8) == Air, "edge water falls instead of crossing hole");

    Corridor();
    Tick();
    Check(WaterLevelAt(9, 8, 8) == 6, "fixture has water at the edge");
    Change(9, 7, 8, Air);
    Change(7, 8, 8, Air);
    Change(7, 7, 8, Stone);
    Tick();
    Check(WaterLevelAt(9, 7, 8) == 7, "occupied adjacent edge drains immediately");
    Check(At(7, 8, 8) == Air, "full-level edge cannot be filtered out in favor of ordinary spread");

    ClearWorld();
    Change(8, 7, 8, Stone);
    Change(7, 7, 8, Stone);
    Change(8, 7, 7, Stone);
    Change(8, 7, 9, Stone);
    for(int x=1;x<=15;++x) for(int z=1;z<=15;++z)
        if(x!=9||z!=8) Change(x,7,z,Stone);
    Change(9, 8, 8, Stone);
    Change(8, 8, 8, Water);
    Tick();
    Check(WaterLevelAt(7, 8, 8) == 6 && WaterLevelAt(8, 8, 9) == 6,
          "solid neighbor above air is not a reachable drop");
    Check(At(9, 7, 8) == Air, "flow cannot pass through a solid neighbor");

    Corridor();
    Tick(12);
    for (int x = 8; x <= 14; ++x)
        Check(WaterLevelAt(x, 8, 8) == 15 - x, "linear levels seven to one");
    Check(At(15, 8, 8) == Air, "level one cannot spread");

    Corridor();
    Tick();
    Check(WaterLevelAt(9, 8, 8) == 6, "fixture creates non-source level six");
    Change(9, 9, 8, Water);
    Tick();
    Check(WaterLevelAt(9, 8, 8) == 7, "fall upgrades lower-level water to seven");
    Check(At(10, 9, 8) == Air, "lower-level refill suppresses horizontal spread");

    Corridor();
    Tick(2);
    Change(10, 8, 9, Water);
    Change(10, 7, 9, Stone);
    Change(11, 7, 9, Stone);
    Change(11, 8, 9, Air);
    Change(10, 8, 10, Stone);
    Tick();
    Check(WaterLevelAt(11, 8, 9) == 6, "air is selected over higher-level water");
    Check(WaterLevelAt(10, 8, 8) == 5, "non-minimum eligible neighbor is not upgraded");

    Corridor();
    Tick(8);
    Change(8, 8, 8, Stone);
    Tick();
    Check(WaterLevelAt(9, 8, 8) == 5, "disconnected water decays one level per tick");
    Tick(6);
    for (int x = 9; x <= 17; ++x)
        Check(At(x, 8, 8) == Air, "disconnected corridor dries completely");

    ClearWorld();
    for (int x = 4; x <= 18; ++x)
        for (int z = 4; z <= 18; ++z) Change(x, 7, z, Stone);
    Change(11, 8, 11, Water);
    Tick(8);
    Change(11, 8, 11, Stone);
    Tick(7);
    for (int x = 0; x < Width; ++x)
        for (int y = 0; y < Height; ++y)
            for (int z = 0; z < Width; ++z)
                Check(At(x, y, z) != Water, "flow cycles cannot sustain water without source");

    ClearWorld();
    Change(0, 7, 8, Stone);
    Change(1, 7, 8, Stone);
    Change(0, 8, 8, Water);
    Tick();
    Check(At(0, 8, 9) == Water && At(1, 8, 8) == Air, "world boundary is not an adjacent hole");
    Check(!Solid(Water), "water has no solid collision");
    ClearWorld();
    for (int x = 20; x <= 44; ++x)
        for (int z = 20; z <= 44; ++z) Change(x, 7, z, Stone);
    Change(32, 8, 32, Water);
    Tick(80);
    int wet = 0, sources = 0;
    for (int x = 0; x < Width; ++x) {
        for (int z = 0; z < Width; ++z) {
            const int distance = std::abs(x - 32) + std::abs(z - 32);
            if (distance <= 6) {
                Check(WaterLevelAt(x, 8, z) == 7 - distance, "long-running flat flow keeps exact exhaustion levels");
            } else {
                Check(At(x, 8, z) != Water, "long-running flat flow cannot exceed level-one boundary");
            }
            if (At(x, 8, z) == Water) ++wet;
            if (IsWaterSource(x, 8, z)) ++sources;
        }
    }
    Check(wet == 85 && sources == 1, "spread stops at 85 cells and cannot manufacture sources");
    Tick(80);
    int stableWet = 0;
    for (int x = 0; x < Width; ++x) for (int z = 0; z < Width; ++z)
        if (At(x, 8, z) == Water) ++stableWet;
    Check(stableWet == wet, "another eighty ticks cannot expand the stable footprint");
    Change(32, 8, 32, Stone);
    Tick(7);
    for (int x = 0; x < Width; ++x) for (int y = 0; y < Height; ++y) for (int z = 0; z < Width; ++z)
        Check(At(x, y, z) != Water, "stable flat flow dries completely after source removal");

    ClearWorld();
    for (int x = 20; x <= 44; ++x) for (int z = 20; z <= 44; ++z) Change(x, 2, z, Stone);
    Change(32, 8, 32, Water);
    Tick(40);
    Check(IsWaterSource(32, 8, 32), "original waterfall source is retained");
    for (int y = 3; y < 8; ++y)
        Check(WaterLevelAt(32, y, 32) == 7 && !IsWaterSource(32, y, 32),
              "falling level seven remains flowing water, never a source");
    Check(WaterLevelAt(33,3,32)==6&&WaterLevelAt(38,3,32)==1&&At(39,3,32)==Air,
          "waterfall resumes horizontal flow on landing with levels six to one");
    for (int x = 20; x <= 44; ++x) for (int z = 20; z <= 44; ++z)
        Change(x, 6, z, Stone);
    Tick(7);
    for (int y = 0; y < 6; ++y) for (int x = 0; x < Width; ++x) for (int z = 0; z < Width; ++z)
        Check(At(x, y, z) != Water, "blocking the column dries its lower flow");
    ClearWorld();
    for(int x=20;x<=44;++x) for(int z=20;z<=44;++z) Change(x,7,z,Stone);
    Change(32,8,32,Water);Tick(8);
    Check(At(31,8,32)==Water,"fixture starts with an existing flat puddle");
    Change(35,7,32,Air);Change(35,6,32,Stone);
    Change(32,7,37,Air);Change(32,6,37,Stone);
    Tick(40);
    Check(At(32,7,37)==Air,"farther competing hole is not selected");
    Check(WaterLevelAt(33,8,32)==6&&WaterLevelAt(34,8,32)==5&&WaterLevelAt(35,8,32)==4,
          "source takes shortest three-cell route to the hole");
    Check(WaterLevelAt(35,7,32)==7,"water falls into nearest hole");
    Check(At(31,8,32)==Air&&At(32,8,31)==Air&&At(32,8,33)==Air&&At(36,8,32)==Air,
          "drain route suppresses spreading sideways and beyond the hole");
    Check(At(35,7,31)!=Water&&At(35,7,33)!=Water,"landing remains inside the hole");
    Tick(80);
    int drainWet=0;
    for(int x=0;x<Width;++x) for(int y=0;y<Height;++y) for(int z=0;z<Width;++z)
        if(At(x,y,z)==Water) ++drainWet;
    Check(drainWet==5,"filled drain stays stable without overflowing after eighty more ticks");
    Change(32,8,32,Stone);Tick(7);
    for(int x=0;x<Width;++x) for(int y=0;y<Height;++y) for(int z=0;z<Width;++z)
        Check(At(x,y,z)!=Water,"drain and falling water dry after source removal");
    ClearWorld();
    for(int x=20;x<=44;++x) for(int z=20;z<=44;++z)
        for(int y=5;y<=7;++y) Change(x,y,z,Stone);
    for(int x=35;x<=39;++x) Change(x,7,32,Air);
    for(int x=38;x<=40;++x) Change(x,6,32,Air);
    Change(32,8,32,Water);
    Tick(60);
    Check(WaterLevelAt(35,7,32)==7,"first drop reaches channel floor");
    Check(WaterLevelAt(36,7,32)==6&&WaterLevelAt(37,7,32)==5&&WaterLevelAt(38,7,32)==4,
          "water continues through channel after first landing");
    Check(WaterLevelAt(38,6,32)==7,"channel flow reaches second drop");
    Check(WaterLevelAt(39,6,32)==6&&WaterLevelAt(40,6,32)==5,
          "water continues normally after second landing");
    Check(At(39,7,32)==Air&&At(31,8,32)==Air,
          "continuation preserves nearest-drop priority on upper tiers");
    Check(!IsWaterSource(35,7,32)&&!IsWaterSource(38,6,32),
          "landings do not create permanent sources");
    Change(32,8,32,Stone);Tick(7);
    for(int x=0;x<Width;++x) for(int y=0;y<Height;++y) for(int z=0;z<Width;++z)
        Check(At(x,y,z)!=Water,"both downstream tiers dry after source removal");
    ClearWorld();
    for(int x=20;x<=44;++x) for(int z=20;z<=44;++z) Change(x,7,z,Stone);
    Change(33,8,32,Stone);
    Change(34,7,32,Air);Change(34,6,32,Stone);
    Change(32,7,37,Air);Change(32,6,37,Stone);
    Change(32,8,32,Water);Tick(40);
    Check(At(32,8,31)==Water&&At(32,8,33)==Water,
          "equally short detours around a wall are both used");
    Check(At(34,7,32)==Water&&At(32,7,37)==Air,
          "shortest traversable route wins over a farther unobstructed hole");
    Check(At(31,8,32)==Air,"longer detour is not chosen");
    ClearWorld();
    for(int x=20;x<=44;++x) for(int z=20;z<=44;++z) Change(x,7,z,Stone);
    Change(30,7,32,Air);Change(30,6,32,Stone);
    Change(34,7,32,Air);Change(34,6,32,Stone);
    Change(32,8,32,Water);Tick(30);
    Check(At(30,7,32)==Water&&At(34,7,32)==Water,
          "equidistant opposite drains both receive water");
    Check(At(32,8,31)==Air&&At(32,8,33)==Air,
          "ordinary spreading is suppressed when shortest drain routes exist");
    std::puts("PASS: fluid regression scenarios, nearest drains and continued flow after consecutive drops");
}
