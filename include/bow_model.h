#pragma once
#include "texture_atlas.h"
#include "arrow_model.h"
#include "projectile_physics.h"
#include <cmath>
#include <rlgl.h>
namespace game {
inline void DrawBowModel(float seconds,Color tint=WHITE,Texture2D atlas={},int woodTile=0){
    float pull=seconds<0?0:std::clamp(seconds/.9f,0.f,1.f);
    auto color=[&](Color c){return Color{static_cast<unsigned char>(c.r*tint.r/255),static_cast<unsigned char>(c.g*tint.g/255),static_cast<unsigned char>(c.b*tint.b/255),255};};
    Vector3 ends[2];
    for(int i=0;i<2;++i){float sign=i?1.f:-1.f;Vector3 points[]={{0,0,0},{.005f,.2f*sign,0},{.06f-.03f*pull,.4f*sign,0},{.28f-.08f*pull,.57f*sign,0},{.42f-.12f*pull,.65f*sign,0}};
        for(int n=0;n<4;++n){
            Vector3 delta=Vector3Subtract(points[n+1],points[n]);
            float length=Vector3Length(delta),width=.072f-n*.009f,depth=.052f;
            rlPushMatrix();
            Vector3 mid=Vector3Scale(Vector3Add(points[n],points[n+1]),.5f);
            rlTranslatef(mid.x,mid.y,mid.z);
            rlRotatef(std::atan2(-delta.x,delta.y)*RAD2DEG,0,0,1);
            // Six textured faces keep the wood grain visible from either side.
            Vector3 vertices[]={{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};
            const int faces[6][4]={{4,5,6,7},{1,0,3,2},{0,4,7,3},{5,1,2,6},{7,6,2,3},{0,1,5,4}};
            const float shades[]={1.f,.72f,.8f,.9f,1.f,.65f};
            rlSetTexture(atlas.id);rlBegin(RL_QUADS);
            for(int face=0;face<6;++face){
                Color c=color({static_cast<unsigned char>(205*shades[face]),static_cast<unsigned char>(170*shades[face]),static_cast<unsigned char>(125*shades[face]),255});
                rlColor4ub(c.r,c.g,c.b,c.a);
                for(int v=0;v<4;++v){
                    float u=(AtlasX(woodTile,atlas.width)+((v==1||v==2)?13.f:3.f))/std::max(1,atlas.width);
                    float t=(AtlasY(woodTile,atlas.width)+((v>=2)?15.f:1.f))/std::max(1,atlas.height);
                    rlTexCoord2f(u,t);
                    auto pos=vertices[faces[face][v]];
                    rlVertex3f(pos.x*width*.5f,pos.y*(length+.012f)*.5f,pos.z*depth*.5f);
                }
            }
            rlEnd();rlSetTexture(0);rlPopMatrix();
        }ends[i]=points[4];}
    DrawCube({0,0,0},.085f,.22f,.075f,color({83,61,39,255}));
    for(float y:{-.065f,0.f,.065f})DrawCube({0,y,.04f},.09f,.023f,.012f,color({164,137,99,255}));
    // Positive X faces the archer: the string stays behind the wooden grip.
    Vector3 string{.42f+.38f*pull,0,0};
    for(auto end:ends)DrawCylinderEx(end,string,.006f,.006f,4,color({235,228,207,255}));
    if(seconds>=0)DrawArrowModel(Vector3Add(string,{-.85f,0,0}),{-1,0,0});
}
}
