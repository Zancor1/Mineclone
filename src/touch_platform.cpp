#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define Rectangle WinRectangle
#define CloseWindow WinCloseWindow
#define ShowCursor WinShowCursor
#include <windows.h>
#undef Rectangle
#undef CloseWindow
#undef ShowCursor
#undef DrawText
#undef DrawTextEx
#undef LoadImage
#endif
#include "touch_controls.h"
#include <map>
#if defined(__linux__) && !defined(__ANDROID__)
#include <dlfcn.h>
#define Font X11Font
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#undef Font
#undef None
#endif
namespace game {
namespace {
std::vector<ScreenTouch> contacts;
void Contact(int id,Vector2 p,bool active){auto it=std::find_if(contacts.begin(),contacts.end(),[&](auto t){return t.id==id;});if(!active){if(it!=contacts.end())contacts.erase(it);}else if(it!=contacts.end())it->position=p;else if(contacts.size()<16)contacts.push_back({id,p});}
#if defined(_WIN32)
HWND window=nullptr;WNDPROC original=nullptr;
LRESULT CALLBACK TouchWindowProcedure(HWND hwnd,UINT message,WPARAM wParam,LPARAM lParam){
    if(message==WM_TOUCH){std::array<TOUCHINPUT,16> points{};unsigned count=LOWORD(wParam);if(count<=points.size()&&GetTouchInputInfo(reinterpret_cast<HTOUCHINPUT>(lParam),count,points.data(),sizeof(TOUCHINPUT))){
        RECT area{};GetClientRect(hwnd,&area);
        for(unsigned i=0;i<count;++i){POINT p{TOUCH_COORD_TO_PIXEL(points[i].x),TOUCH_COORD_TO_PIXEL(points[i].y)};ScreenToClient(hwnd,&p);
            Contact(int(points[i].dwID),{float(p.x)*GetScreenWidth()/std::max(1L,area.right),float(p.y)*GetScreenHeight()/std::max(1L,area.bottom)},!(points[i].dwFlags&TOUCHEVENTF_UP));}}
        CloseTouchInputHandle(reinterpret_cast<HTOUCHINPUT>(lParam));return 0;
    }
    if(message==WM_KILLFOCUS)contacts.clear();
    return CallWindowProcW(original,hwnd,message,wParam,lParam);
}
void PollNative(){if(!window){window=static_cast<HWND>(GetWindowHandle());if(window&&RegisterTouchWindow(window,0))original=reinterpret_cast<WNDPROC>(SetWindowLongPtrW(window,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(TouchWindowProcedure)));}}
#elif defined(__linux__) && !defined(__ANDROID__)
struct XTouch {
    void* xlib=nullptr;void* xi=nullptr;Display* display=nullptr;int opcode=0;bool attempted=false;
    decltype(&XOpenDisplay) open=nullptr;decltype(&XCloseDisplay) close=nullptr;decltype(&XPending) pending=nullptr;decltype(&XNextEvent) next=nullptr;decltype(&XGetEventData) data=nullptr;decltype(&XFreeEventData) free=nullptr;decltype(&XFlush) flush=nullptr;decltype(&XQueryExtension) query=nullptr;
    decltype(&XIQueryVersion) version=nullptr;decltype(&XISelectEvents) select=nullptr;
    decltype(&XIQueryDevice) devices=nullptr;decltype(&XIFreeDeviceInfo) freeDevices=nullptr;std::map<int,bool> directDevices;
    void Init(){if(attempted)return;attempted=true;
        using GetPlatform=int(*)();auto platform=reinterpret_cast<GetPlatform>(dlsym(RTLD_DEFAULT,"glfwGetPlatform"));if(platform&&platform()!=0x00060004)return;
        using GetNative=unsigned long(*)(void*);auto native=reinterpret_cast<GetNative>(dlsym(RTLD_DEFAULT,"glfwGetX11Window"));if(!native)return;
        xlib=dlopen("libX11.so.6",RTLD_LAZY|RTLD_LOCAL);xi=dlopen("libXi.so.6",RTLD_LAZY|RTLD_LOCAL);if(!xlib||!xi)return;
#define XSYM(member,name) member=reinterpret_cast<decltype(member)>(dlsym(xlib,name));if(!member)return
        XSYM(open,"XOpenDisplay");XSYM(close,"XCloseDisplay");XSYM(pending,"XPending");XSYM(next,"XNextEvent");XSYM(data,"XGetEventData");XSYM(free,"XFreeEventData");XSYM(flush,"XFlush");XSYM(query,"XQueryExtension");
#undef XSYM
        version=reinterpret_cast<decltype(version)>(dlsym(xi,"XIQueryVersion"));select=reinterpret_cast<decltype(select)>(dlsym(xi,"XISelectEvents"));devices=reinterpret_cast<decltype(devices)>(dlsym(xi,"XIQueryDevice"));freeDevices=reinterpret_cast<decltype(freeDevices)>(dlsym(xi,"XIFreeDeviceInfo"));if(!version||!select||!devices||!freeDevices)return;
        display=open(nullptr);if(!display)return;int event,error,major=2,minor=2;if(!query(display,"XInputExtension",&opcode,&event,&error)||version(display,&major,&minor)!=Success||major<2||(major==2&&minor<2)){close(display);display=nullptr;return;}
        unsigned char bits[XIMaskLen(XI_LASTEVENT)]{};XISetMask(bits,XI_TouchBegin);XISetMask(bits,XI_TouchUpdate);XISetMask(bits,XI_TouchEnd);XIEventMask mask{XIAllMasterDevices,int(sizeof(bits)),bits};
        select(display,native(GetWindowHandle()),&mask,1);flush(display);
    }
    bool DirectDevice(int id){auto cached=directDevices.find(id);if(cached!=directDevices.end())return cached->second;
        int count=0;bool direct=false;auto info=devices(display,id,&count);
        if(info){for(int i=0;i<count;++i)for(int j=0;j<info[i].num_classes;++j)if(info[i].classes[j]->type==XITouchClass&&reinterpret_cast<XITouchClassInfo*>(info[i].classes[j])->mode==XIDirectTouch)direct=true;freeDevices(info);}
        directDevices[id]=direct;return direct;
    }
    void Poll(){Init();if(!display)return;while(pending(display)){XEvent event{};next(display,&event);if(event.type==GenericEvent&&event.xcookie.extension==opcode&&data(display,&event.xcookie)){
        int kind=event.xcookie.evtype;if(kind==XI_TouchBegin||kind==XI_TouchUpdate||kind==XI_TouchEnd){auto p=static_cast<XIDeviceEvent*>(event.xcookie.data);if(DirectDevice(p->sourceid))Contact(p->detail,{float(p->event_x),float(p->event_y)},kind!=XI_TouchEnd);}free(display,&event.xcookie);}}}
    void Close(){if(display&&close)close(display);display=nullptr;if(xi)dlclose(xi);if(xlib)dlclose(xlib);xi=xlib=nullptr;attempted=false;directDevices.clear();}
} xTouch;
void PollNative(){xTouch.Poll();}
#else
void PollNative(){}
#endif
}
std::vector<ScreenTouch> PollScreenTouches(){
    PollNative();auto result=contacts;
    if(result.empty())for(int i=0;i<std::min(16,GetTouchPointCount());++i){int id=GetTouchPointId(i);auto p=GetTouchPosition(i);if(id>=0&&p.x>=0&&p.y>=0)result.push_back({id,p});}
    if(!IsWindowFocused()){contacts.clear();result.clear();}return result;
}
void ShutdownScreenTouches(){
#if defined(_WIN32)
    if(window&&original){SetWindowLongPtrW(window,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(original));UnregisterTouchWindow(window);}window=nullptr;original=nullptr;
#elif defined(__linux__) && !defined(__ANDROID__)
    xTouch.Close();
#endif
    contacts.clear();
}
}
