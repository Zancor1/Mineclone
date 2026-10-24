#include "chat_text.h"
#include <cstdio>
#include <cstdlib>
namespace {void Check(bool b,const char* name){if(!b){std::fprintf(stderr,"FAIL: %s\n",name);std::exit(1);}}}
int main(){using namespace game;
    Check(PlainMinecraftText(R"({"translate":"chat.type.text","with":["Zancor","ola"]})")=="<Zancor> ola","translated chat retains nickname and body");
    Check(PlainMinecraftText(R"({"translate":"chat.type.text","with":[{"text":"[VIP] ","extra":[{"text":"Zancor"}]},{"text":"oi","extra":[{"text":"!"}]}]})")=="<[VIP] Zancor> oi!","nested nickname and message components");
    Check(PlainMinecraftText(R"([{"text":"A"},"B",{"text":"C"}])")=="ABC","component arrays");
    Check(PlainMinecraftText(R"({"text":"Ol\u00e1 \ud83d\ude00\nTeste"})")=="Olá 😀\nTeste","unicode and surrogate escapes");
    Check(PlainMinecraftText(R"({"text":"\u00a7a[VIP] \u00a7rNick"})")=="[VIP] Nick","legacy formatting is not displayed as garbage");
    Check(PlainMinecraftText(R"({"text":"a\"b\\c"})")=="a\"b\\c","escaped quotes and slash");
    Check(PlainMinecraftText(R"({"translate":"chat.type.announcement","with":["Servidor","aviso"]})")=="[Servidor] aviso","server announcements");
    Check(PlainMinecraftText("texto sem JSON")=="texto sem JSON","plain chat fallback");
    Check(!PlainMinecraftText("{invalid").empty(),"malformed components do not crash");
    std::puts("PASS: chat nicknames, nested components, translations, unicode and formatting");
}
