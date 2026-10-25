#include "android_platform.h"
#ifdef __ANDROID__
#include "progression.h"
#include <raylib.h>
#include <android_native_app_glue.h>
#include <jni.h>
#include <filesystem>
#include <mutex>
extern "C" android_app* GetAndroidApp();
namespace {
std::mutex textMutex;std::string* target=nullptr;std::string result;int request=0,received=-1,maximum=0,filter=0;bool cancelled=false;
}
extern "C" JNIEXPORT void JNICALL Java_com_zancor_mineclone_MineCloneActivity_nativeText(JNIEnv* env,jclass,jint token,jstring text){
    std::lock_guard<std::mutex> lock(textMutex);if(token!=request||!target)return;received=token;cancelled=!text;
    if(text){const char* value=env->GetStringUTFChars(text,nullptr);if(value){result=value;env->ReleaseStringUTFChars(text,value);}}
}
namespace game {
bool PrepareAndroidStorage(){
    auto app=GetAndroidApp();if(!app||!app->activity||!app->activity->internalDataPath)return false;
    std::error_code error;std::filesystem::path root=app->activity->internalDataPath;std::filesystem::create_directories(root,error);if(error)return false;
    std::filesystem::current_path(root,error);if(error)return false;progression::Directory=root/"profiles";return true;
}
void EditAndroidText(std::string& value,int limit,int kind){
    auto app=GetAndroidApp();if(!app||!app->activity)return;
    JNIEnv* env=nullptr;bool attached=app->activity->vm->GetEnv(reinterpret_cast<void**>(&env),JNI_VERSION_1_6)!=JNI_OK;
    if(attached&&app->activity->vm->AttachCurrentThread(&env,nullptr)!=JNI_OK)return;
    int token;{std::lock_guard<std::mutex> lock(textMutex);target=&value;maximum=limit;filter=kind;token=++request;received=-1;}
    auto cls=env->GetObjectClass(app->activity->clazz);
    auto method=cls&&!env->ExceptionCheck()?env->GetMethodID(cls,"editGameText","(Ljava/lang/String;III)V"):nullptr;
    if(method&&!env->ExceptionCheck()){
        auto text=env->NewStringUTF(value.c_str());
        if(text&&!env->ExceptionCheck())env->CallVoidMethod(app->activity->clazz,method,text,limit,kind,token);
        if(text)env->DeleteLocalRef(text);
    }
    if(env->ExceptionCheck()){env->ExceptionDescribe();env->ExceptionClear();CancelAndroidText();}
    else if(!method)CancelAndroidText();
    if(cls)env->DeleteLocalRef(cls);
    if(attached)app->activity->vm->DetachCurrentThread();
}
void ApplyAndroidText(){std::lock_guard<std::mutex> lock(textMutex);if(!target||received!=request)return;
    if(!cancelled){std::string clean;for(unsigned char c:result){bool valid=c>=32&&c<127;if(filter==1)valid=(c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_';if(filter==2)valid=c>='0'&&c<='9';if(filter==3)valid=c>32&&c<127;if(valid&&int(clean.size())<maximum)clean+=char(c);}*target=std::move(clean);}target=nullptr;received=-1;result.clear();
}
bool AndroidBackPressed(){return IsKeyPressed(KEY_BACK);}
void CancelAndroidText(){std::lock_guard<std::mutex> lock(textMutex);target=nullptr;received=-1;++request;result.clear();}
}
#endif
