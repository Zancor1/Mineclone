#pragma once
#include <string>
namespace game {
#ifdef __ANDROID__
bool PrepareAndroidStorage();
void EditAndroidText(std::string& value,int limit,int kind=0);
void ApplyAndroidText();
void CancelAndroidText();
bool AndroidBackPressed();
#else
inline bool PrepareAndroidStorage(){return true;}
inline void EditAndroidText(std::string&,int,int=0){}
inline void ApplyAndroidText(){}
inline void CancelAndroidText(){}
inline bool AndroidBackPressed(){return false;}
#endif
}
