#pragma once
#include "block.h"
#include <raylib.h>
#include <raymath.h>
#include <rlgl.h>
#include <vector>
#include <cmath>
namespace game {
inline Color ProjectilePixel(Block item,int x,int y){
    float dx=x-7.5f,dy=y-7.5f,r=dx*dx+dy*dy;if(r>48)return BLANK;
    if(item==Snowball||item==Egg){
        if(item==Egg&&((dx*dx)/.75f+dy*dy>44))return BLANK;
        int shade=int(225-dy*2-dx*2);return item==Snowball?Color{(unsigned char)shade,(unsigned char)shade,(unsigned char)std::min(255,shade+15),255}:Color{(unsigned char)shade,(unsigned char)(shade-12),(unsigned char)(shade-45),255};
    }
    if(item==EnderPearl){
        Color c=r>35?Color{13,48,47,255}:r>23?Color{31,104,91,255}:Color{36,159,132,255};
        if(x>=7&&x<=10&&y>=6&&y<=11)c={15,67,70,255};
        if(x<=6&&y<=6&&r<36)c={116,230,191,255};
        if(x==4&&y==4)c={208,255,232,255};
        return c;
    }
    int noise=(x*13+y*7+x*y*3)%17;
    Color c=noise<7?Color{61,38,30,255}:Color{105,54,25,255};
    if(noise>10||r<18)c=noise%3?Color{246,106,13,255}:Color{255,187,26,255};
    if(r<8)c={255,236,134,255};
    if(r>35)c={48,35,33,255};
    return c;
}
struct ProjectileVertex {Vector3 p;Color color;};
inline const std::vector<ProjectileVertex>& ProjectileGeometry(Block item){
    static std::vector<ProjectileVertex> pearl,fire,snowball,egg;
    auto& vertices=item==EnderPearl?pearl:item==Snowball?snowball:item==Egg?egg:fire;if(!vertices.empty())return vertices;
    constexpr int normals[6][3]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
    constexpr int corners[6][4][3]={{{1,0,1},{1,0,0},{1,1,0},{1,1,1}},{{0,0,0},{0,0,1},{0,1,1},{0,1,0}},{{0,1,1},{1,1,1},{1,1,0},{0,1,0}},{{0,0,0},{1,0,0},{1,0,1},{0,0,1}},{{0,0,1},{1,0,1},{1,1,1},{0,1,1}},{{1,0,0},{0,0,0},{0,1,0},{1,1,0}}};
    auto filled=[](int x,int y,int z){return x*x+y*y+z*z<=12;};
    const float shade[]={.82f,.70f,1.f,.55f,.88f,.95f};
    for(int x=-3;x<=3;++x)for(int y=-3;y<=3;++y)for(int z=-3;z<=3;++z)if(filled(x,y,z)){
        Color base=ProjectilePixel(item,8+x*2,7-y*2);
        if(item==Fireball){int n=std::abs(x*17+y*31+z*13)%11;base=n<5?Color{75,43,30,255}:n<9?Color{245,108,15,255}:Color{255,219,92,255};}
        for(int f=0;f<6;++f)if(!filled(x+normals[f][0],y+normals[f][1],z+normals[f][2])){
            Color c{static_cast<unsigned char>(base.r*shade[f]),static_cast<unsigned char>(base.g*shade[f]),static_cast<unsigned char>(base.b*shade[f]),255};
            for(auto& corner:corners[f])vertices.push_back({{(x+corner[0]-.5f)/7.f,(y+corner[1]-.5f)/7.f,(z+corner[2]-.5f)/7.f},c});
        }
    }
    return vertices;
}
inline void DrawProjectileItemModel(Block item,Color tint=WHITE){
    rlSetTexture(0);rlBegin(RL_QUADS);
    for(const auto& v:ProjectileGeometry(item)){rlColor4ub(v.color.r*tint.r/255,v.color.g*tint.g/255,v.color.b*tint.b/255,255);rlVertex3f(v.p.x,v.p.y,v.p.z);}
    rlEnd();
}
inline void DrawThrownItem(Block item,Vector3 position,Vector3 velocity){
    float time=float(GetTime()),size=item==Fireball?.44f:.32f;
    rlPushMatrix();rlTranslatef(position.x,position.y,position.z);rlRotatef(time*(item==Fireball?125:55),0,1,0);rlScalef(size,size,size);DrawProjectileItemModel(item);rlPopMatrix();
    auto direction=Vector3Normalize(velocity);
    for(int i=0;i<4;++i){float t=std::fmod(time*3+i*.25f,1.f);auto p=Vector3Subtract(position,Vector3Scale(direction,.12f+t*.55f));p.y+=std::sin(time*8+i)*.025f;
        float s=(1-t)*(item==Fireball?.065f:.025f);DrawCube(p,s,s,s,item==Fireball?(i%2?ORANGE:YELLOW):Color{166,84,219,255});}
}
}
