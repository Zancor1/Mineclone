#include "map_blocks.h"
#include "world.h"
#include <unordered_map>
namespace game {
namespace {
std::unordered_map<int,MapBlockState> states;
int Key(int x,int y,int z){return (x*Height+y)*Width+z;}
MapBlockState Default(Block b){
    if(b==MapSlab)return {b,StoneBricks,MapShape::Slab,0};
    if(b==MapStairs)return {b,StoneBricks,MapShape::Stairs,0};
    if(b==MapFence)return {b,Planks,MapShape::Fence,0};
    if(b==IronBars)return {b,BlockOfIron,MapShape::Bars,0};
    if(b==MapCarpet)return {b,WhiteWool,MapShape::Carpet,0};
    if(b==MapDecoration)return {b,Wood,MapShape::Torch,0};
    return {b,b,MapShape::Cube,0};
}
}
void ClearMapBlocks(){states.clear();}
void SetMapBlockState(int x,int y,int z,MapBlockState state){if(Inside(x,y,z))states[Key(x,y,z)]=state;}
MapBlockState MapStateAt(int x,int y,int z){auto b=At(x,y,z);if(states.empty())return Default(b);auto it=states.find(Key(x,y,z));return it!=states.end()&&it->second.block==b?it->second:Default(b);}
bool HasMapShape(int x,int y,int z){return At(x,y,z)>=MapSlab;}
BlockBoxes MapShapeBoxes(int x,int y,int z){
    auto state=MapStateAt(x,y,z);BlockBoxes result;
    auto add=[&](float a,float b,float c,float d,float e,float f){result.boxes[result.count++]={{x+a,y+b,z+c},{x+d,y+e,z+f}};};
    switch(state.shape){
    case MapShape::Slab:if(state.data&8)add(0,.5f,0,1,1,1);else add(0,0,0,1,.5f,1);break;
    case MapShape::Stairs:{bool upper=state.data&4;add(0,upper?.5f:0,0,1,upper?1:.5f,1);float low=upper?0:.5f,high=upper?.5f:1;switch(state.data&3){case 0:add(.5f,low,0,1,high,1);break;case 1:add(0,low,0,.5f,high,1);break;case 2:add(0,low,.5f,1,high,1);break;case 3:add(0,low,0,1,high,.5f);break;}break;}
    case MapShape::Fence:case MapShape::Bars:{bool bars=state.shape==MapShape::Bars;float lo=bars?.4375f:.375f,hi=1-lo;add(lo,0,lo,hi,1,hi);const int dx[]={1,-1,0,0},dz[]={0,0,1,-1};for(int i=0;i<4;++i){Block n=At(x+dx[i],y,z+dz[i]);if(!Solid(n))continue;float a=dx[i]<0?0:lo,d=dx[i]>0?1:hi,c=dz[i]<0?0:lo,f=dz[i]>0?1:hi;if(bars)add(a,0,c,d,1,f);else{add(a,.375f,c,d,.5625f,f);add(a,.75f,c,d,.9375f,f);}}break;}
    case MapShape::Carpet:add(0,0,0,1,.0625f,1);break;
    case MapShape::Torch:add(.4375f,0,.4375f,.5625f,.625f,.5625f);break;
    case MapShape::Pot:add(.3125f,0,.3125f,.6875f,.375f,.6875f);break;
    case MapShape::Plate:add(.0625f,0,.0625f,.9375f,.0625f,.9375f);break;
    case MapShape::Hopper:add(0,.625f,0,1,1,1);add(.25f,.25f,.25f,.75f,.625f,.75f);add(.375f,0,.375f,.625f,.25f,.625f);break;
    case MapShape::Sign:add(.0625f,.4f,.4375f,.9375f,.95f,.5625f);add(.4375f,0,.4375f,.5625f,.4f,.5625f);break;
    case MapShape::Portal:add(0,0,.375f,1,1,.625f);break;
    case MapShape::Barrier:add(0,0,0,1,1,1);break;
    case MapShape::Fire:case MapShape::Web:add(0,0,.48f,1,1,.52f);add(.48f,0,0,.52f,1,1);break;
    default:add(0,0,0,1,CollisionHeight(state.block),1);break;
    }
    return result;
}
unsigned char BlockEmissionAt(int x,int y,int z){auto state=MapStateAt(x,y,z);if(state.block==MapDecoration)return (state.shape==MapShape::Torch||state.shape==MapShape::Fire)?14:0;return Emission(state.block);}
BlockBoxes CollisionBoxesAt(int x,int y,int z){auto state=MapStateAt(x,y,z);bool solid=state.shape==MapShape::Barrier||Solid(state.block)||(state.shape==MapShape::Cube&&Solid(state.material))||state.shape==MapShape::Pot||state.shape==MapShape::Plate||state.shape==MapShape::Hopper;return solid?MapShapeBoxes(x,y,z):BlockBoxes{};}
}
