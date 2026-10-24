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
bool LightPassableAt(int x,int y,int z){auto state=MapStateAt(x,y,z);if(state.shape==MapShape::Cube||state.shape==MapShape::Log||state.shape==MapShape::Dispenser||state.shape==MapShape::Jukebox||state.shape==MapShape::RedstoneBlock)return LightPassable(state.material);return LightPassable(At(x,y,z));}
bool HasMapShape(int x,int y,int z){return At(x,y,z)>=MapSlab;}
BlockBoxes MapShapeBoxes(int x,int y,int z){
    auto state=MapStateAt(x,y,z);BlockBoxes result;
    auto add=[&](float a,float b,float c,float d,float e,float f){result.boxes[result.count++]={{x+a,y+b,z+c},{x+d,y+e,z+f}};};
    switch(state.shape){
    case MapShape::Slab:if(state.data&8)add(0,.5f,0,1,1,1);else add(0,0,0,1,.5f,1);break;
    case MapShape::Stairs:{bool upper=state.data&4;add(0,upper?.5f:0,0,1,upper?1:.5f,1);float low=upper?0:.5f,high=upper?.5f:1;switch(state.data&3){case 0:add(.5f,low,0,1,high,1);break;case 1:add(0,low,0,.5f,high,1);break;case 2:add(0,low,.5f,1,high,1);break;case 3:add(0,low,0,1,high,.5f);break;}break;}
    case MapShape::Fence:case MapShape::Bars:{bool bars=state.shape==MapShape::Bars;float lo=bars?.4375f:.375f,hi=1-lo;add(lo,0,lo,hi,1,hi);const int dx[]={1,-1,0,0},dz[]={0,0,1,-1};for(int i=0;i<4;++i){Block n=At(x+dx[i],y,z+dz[i]);if(!Solid(n))continue;float a=dx[i]<0?0:lo,d=dx[i]>0?1:hi,c=dz[i]<0?0:lo,f=dz[i]>0?1:hi;if(bars)add(a,0,c,d,1,f);else{add(a,.375f,c,d,.5625f,f);add(a,.75f,c,d,.9375f,f);}}break;}
    case MapShape::Carpet:add(0,0,0,1,.0625f,1);break;
    case MapShape::Torch:{float cx=.5f,cz=.5f;if(state.data==1)cx=.125f;if(state.data==2)cx=.875f;if(state.data==3)cz=.125f;if(state.data==4)cz=.875f;add(cx-.0625f,.1f,cz-.0625f,cx+.0625f,.725f,cz+.0625f);break;}
    case MapShape::Pot:add(.3125f,0,.3125f,.6875f,.375f,.6875f);break;
    case MapShape::Plate:add(.0625f,0,.0625f,.9375f,.0625f,.9375f);break;
    case MapShape::Hopper:add(0,.625f,0,1,1,1);add(.25f,.25f,.25f,.75f,.625f,.75f);add(.375f,0,.375f,.625f,.25f,.625f);break;
    case MapShape::Sign:add(.0625f,.4f,.4375f,.9375f,.95f,.5625f);add(.4375f,0,.4375f,.5625f,.4f,.5625f);break;
    case MapShape::Portal:add(0,0,.375f,1,1,.625f);break;
    case MapShape::Wall:{add(.25f,0,.25f,.75f,1,.75f);const int dx[]={1,-1,0,0},dz[]={0,0,1,-1};for(int i=0;i<4;++i)if(Solid(At(x+dx[i],y,z+dz[i]))){add(dx[i]<0?0:.3125f,0,dz[i]<0?0:.3125f,dx[i]>0?1:.6875f,.8125f,dz[i]>0?1:.6875f);}break;}
    case MapShape::Pane:{const int dx[]={1,-1,0,0},dz[]={0,0,1,-1};bool connected=false;for(int i=0;i<4;++i)if(Solid(At(x+dx[i],y,z+dz[i]))||MapStateAt(x+dx[i],y,z+dz[i]).shape==MapShape::Pane){add(dx[i]<0?0:.4375f,0,dz[i]<0?0:.4375f,dx[i]>0?1:.5625f,1,dz[i]>0?1:.5625f);connected=true;}if(!connected){add(0,0,.4375f,1,1,.5625f);add(.4375f,0,0,.5625f,1,1);}break;}
    case MapShape::Trapdoor:{int d=state.data&3;if(!(state.data&4))add(0,state.data&8?.8125f:0,0,1,state.data&8?1:.1875f,1);else if(d<2)add(0,0,d==0?.8125f:0,1,1,d==0?1:.1875f);else add(d==2?.8125f:0,0,0,d==2?1:.1875f,1,1);break;}
    case MapShape::Door:{int d=state.data;if(d&8)d=MapStateAt(x,y-1,z).data;int facing=d&3;if(d&4)facing=(facing+1)%4;switch(facing){case 0:add(0,0,0,.1875f,1,1);break;case 1:add(0,0,0,1,1,.1875f);break;case 2:add(.8125f,0,0,1,1,1);break;default:add(0,0,.8125f,1,1,1);}break;}
    case MapShape::Gate:{bool axis=state.data&1;for(int side=0;side<2;++side)if(axis)add(.375f,0,side*.8125f,.625f,1,side*.8125f+.1875f);else add(side*.8125f,0,.375f,side*.8125f+.1875f,1,.625f);if(!(state.data&4)){if(axis)add(.4375f,.375f,0,.5625f,.875f,1);else add(0,.375f,.4375f,1,.875f,.5625f);}break;}
    case MapShape::Rail:add(0,0,0,1,.0625f,1);break;
    case MapShape::Vine:{bool any=false;if(state.data&1){add(0,0,.98f,1,1,1);any=true;}if(state.data&2){add(0,0,0,.02f,1,1);any=true;}if(state.data&4){add(0,0,0,1,1,.02f);any=true;}if(state.data&8){add(.98f,0,0,1,1,1);any=true;}if(!any)add(0,.98f,0,1,1,1);break;}
    case MapShape::Plant:add(.1f,0,.49f,.9f,.9f,.51f);add(.49f,0,.1f,.51f,.9f,.9f);break;
    case MapShape::Anvil:add(.125f,0,.125f,.875f,.25f,.875f);add(.3125f,.25f,.25f,.6875f,.625f,.75f);if(state.data&1)add(0,.625f,.1875f,1,1,.8125f);else add(.1875f,.625f,0,.8125f,1,1);break;
    case MapShape::Skull:add(.25f,0,.25f,.75f,.5f,.75f);break;
    case MapShape::Cocoa:{float radius=.125f+.0625f*((state.data>>2)&3);int d=state.data&3;float cx=.5f,cz=.5f;if(d==0)cz=1-radius;if(d==1)cx=radius;if(d==2)cz=radius;if(d==3)cx=1-radius;add(cx-radius,.25f,cz-radius,cx+radius,.75f,cz+radius);break;}
    case MapShape::Hook:case MapShape::Button:{float thickness=state.shape==MapShape::Button?.125f:.25f;switch(state.data&7){case 1:add(0,.375f,.3125f,thickness,.625f,.6875f);break;case 2:add(1-thickness,.375f,.3125f,1,.625f,.6875f);break;case 3:add(.3125f,.375f,0,.6875f,.625f,thickness);break;case 4:add(.3125f,.375f,1-thickness,.6875f,.625f,1);break;default:add(.3125f,0,.375f,.6875f,thickness,.625f);}break;}
    case MapShape::PortalFrame:add(0,0,0,1,.8125f,1);if(state.data&4)add(.3125f,.8125f,.3125f,.6875f,1,.6875f);break;
    case MapShape::Beacon:add(0,0,0,1,1,1);add(.125f,0,.125f,.875f,.1875f,.875f);add(.1875f,.1875f,.1875f,.8125f,.8125f,.8125f);break;
    case MapShape::Dispenser:case MapShape::Jukebox:case MapShape::RedstoneBlock:case MapShape::Log:add(0,0,0,1,1,1);break;
    case MapShape::Barrier:add(0,0,0,1,1,1);break;
    case MapShape::Fire:case MapShape::Web:add(0,0,.48f,1,1,.52f);add(.48f,0,0,.52f,1,1);break;
    default:add(0,0,0,1,CollisionHeight(state.block),1);break;
    }
    return result;
}
unsigned char BlockEmissionAt(int x,int y,int z){auto state=MapStateAt(x,y,z);if(state.block==MapDecoration)return (state.shape==MapShape::Torch||state.shape==MapShape::Fire)?14:state.shape==MapShape::Beacon?15:0;return Emission(state.block);}
BlockBoxes CollisionBoxesAt(int x,int y,int z){auto state=MapStateAt(x,y,z);bool extra=state.shape>=MapShape::Wall&&state.shape!=MapShape::Vine&&state.shape!=MapShape::Plant&&state.shape!=MapShape::Rail&&state.shape!=MapShape::Hook&&state.shape!=MapShape::Button;bool solid=extra||state.shape==MapShape::Barrier||Solid(state.block)||(state.shape==MapShape::Cube&&Solid(state.material))||state.shape==MapShape::Pot||state.shape==MapShape::Plate||state.shape==MapShape::Hopper;return solid?MapShapeBoxes(x,y,z):BlockBoxes{};}
}
