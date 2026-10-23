#include "renderer.h"
#include <raymath.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {
using namespace game;
void Check(bool value, const char* message) {
    if (!value) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}
}
bool Near(float a,float b) {return std::abs(a-b)<.00001f;}
void ClearWorld() {
    for(int x=0;x<Width;++x) for(int y=0;y<Height;++y) for(int z=0;z<Width;++z)
        if(At(x,y,z)!=Air) Change(x,y,z,Air);
}
void Corridor(int start) {
    ClearWorld();
    for(int x=start;x<=start+9;++x) {
        Change(x,7,8,Stone);Change(x,8,7,Stone);Change(x,8,9,Stone);
    }
    Change(start-1,8,8,Stone);Change(start,8,8,Water);
    for(int i=0;i<10;++i) UpdateWater(.15f);
}
void CheckWinding(const WaterFace& face) {
    for(int i=1;i<3;++i) {
        Vector3 a=Vector3Subtract(face.vertices[i],face.vertices[0]);
        Vector3 b=Vector3Subtract(face.vertices[i+1],face.vertices[0]);
        Check(Vector3DotProduct(Vector3CrossProduct(a,b),face.normal)>0.00001f,
              "water triangles have positive area and outward winding");
    }
}
}
int main() {
    using namespace game;
    Corridor(8);
    for(int x=8;x<=14;++x) {
        const float height=(15-x)/7.f;
        Check(Near(WaterHeight(x,8,8),height),"height follows level / seven");
        const WaterFace top=BuildWaterFace(x,8,8,2);
        Check(top.visible,"exposed top exists at every flow level");
        for(Vector3 vertex:top.vertices)
            Check(Near(vertex.y,8+height),"all four top vertices share the same height");
        CheckWinding(top);
        Check(WaterContains({x+.5f,8+height-.01f,8.5f}),"submersion uses rendered height");
        Check(!WaterContains({x+.5f,8+height+.01f,8.5f}),"space above water remains dry");
    }
    for(int x=8;x<14;++x) {
        WaterFace edge=BuildWaterFace(x,8,8,0);
        Check(edge.visible,"higher neighbor emits the step wall");
        Check(!BuildWaterFace(x+1,8,8,1).visible,"lower neighbor omits overlapping internal wall");
        float low=edge.vertices[0].y,high=low;
        for(Vector3 vertex:edge.vertices) {
            low=std::min(low,vertex.y);high=std::max(high,vertex.y);
            Check(Near(vertex.x,float(x+1)),"step wall lies exactly on shared voxel boundary");
        }
        Check(Near(low,8+WaterHeight(x+1,8,8)),"step wall meets lower top without gaps");
        Check(Near(high,8+WaterHeight(x,8,8)),"step wall meets higher top without gaps");
        CheckWinding(edge);
    }
    Change(7,8,8,Water);
    Check(!BuildWaterFace(7,8,8,0).visible&&!BuildWaterFace(8,8,8,1).visible,
          "equal-level water has no internal wall");

    Change(9,9,8,Water);
    Check(Near(WaterHeight(9,8,8),1.f),"stacked lower water fills the column");
    Check(!BuildWaterFace(9,8,8,2).visible&&!BuildWaterFace(9,9,8,3).visible,
          "stacked water has no parallel internal horizontal planes");
    Change(9,9,8,Air);
    Check(Near(WaterHeight(9,8,8),6.f/7.f)&&BuildWaterFace(9,8,8,2).visible,
          "removing upper water restores level height and visible top");
    Change(9,9,8,Stone);
    Check(BuildWaterFace(9,8,8,2).visible,"partial water keeps its top below a ceiling");

    Corridor(14);
    WaterFace boundary=BuildWaterFace(15,8,8,0);
    Check(boundary.visible,"step wall survives a chunk boundary");
    Check(Near(boundary.vertices[0].y,8+5.f/7.f)&&Near(boundary.vertices[2].y,8+6.f/7.f),
          "chunk edge uses the same heights on both sides");
    const uint64_t left=ChunkVersions[0],right=ChunkVersions[ChunkCount];
    Change(16,8,8,Water);
    Check(ChunkVersions[0]>left&&ChunkVersions[ChunkCount]>right,"boundary level changes invalidate both chunks");
    Check(!BuildWaterFace(15,8,8,0).visible,"changed neighboring height updates face visibility");

    for(int x=14;x<=20;++x) for(int direction=0;direction<6;++direction) {
        WaterFace face=BuildWaterFace(x,8,8,direction);
        if(!face.visible) continue;
        CheckWinding(face);
        for(Vector3 vertex:face.vertices)
            Check(std::isfinite(vertex.x)&&std::isfinite(vertex.y)&&std::isfinite(vertex.z)&&
                  vertex.y>=8&&vertex.y<=9,"no invalid or out-of-cell water vertices");
    }
    std::puts("PASS: water mesh heights, closed steps, winding, stacked layers and chunk seams");
}
