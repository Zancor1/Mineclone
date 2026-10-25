#pragma once
#include "texture_atlas.h"
#include "projectile_physics.h"
#include <rlgl.h>
namespace game {
inline void DrawPrimedTnt(Texture2D atlas,int tile,Vector3 position,float remaining){
    float size=TntPulseScale(remaining);position.y+=(size-.98f)*.5f;
    if(!atlas.id){DrawCubeV(position,{size,size,size},RED);return;}
    constexpr float corners[6][4][3]={
        {{1,0,1},{1,0,0},{1,1,0},{1,1,1}},{{0,0,0},{0,0,1},{0,1,1},{0,1,0}},
        {{0,1,1},{1,1,1},{1,1,0},{0,1,0}},{{0,0,0},{1,0,0},{1,0,1},{0,0,1}},
        {{0,0,1},{1,0,1},{1,1,1},{0,1,1}},{{1,0,0},{0,0,0},{0,1,0},{1,1,0}}};
    constexpr float uv[4][2]={{0,1},{1,1},{1,0},{0,0}};
    constexpr unsigned char shades[]={210,185,255,150,235,200};
    rlPushMatrix();rlTranslatef(position.x,position.y,position.z);rlScalef(size,size,size);
    rlSetTexture(atlas.id);rlBegin(RL_QUADS);
    for(int face=0;face<6;++face){rlColor4ub(shades[face],shades[face],shades[face],255);for(int i=0;i<4;++i){
        rlTexCoord2f((AtlasX(tile,atlas.width)+.5f+uv[i][0]*15)/atlas.width,(AtlasY(tile,atlas.width)+.5f+uv[i][1]*15)/atlas.height);
        rlVertex3f(corners[face][i][0]-.5f,corners[face][i][1]-.5f,corners[face][i][2]-.5f);
    }}
    rlEnd();rlSetTexture(0);rlPopMatrix();
}
}
