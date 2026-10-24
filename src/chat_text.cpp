#include "chat_text.h"
#include <map>
#include <vector>
#include <stdexcept>
#include <cctype>
#include <algorithm>
namespace game {
namespace {
struct Json {
    enum Kind {Null,String,Array,Object} kind=Null;
    std::string text;
    std::vector<Json> array;
    std::map<std::string,Json> object;
    const Json& at(const std::string& key)const {static const Json empty;auto it=object.find(key);return it==object.end()?empty:it->second;}
};
void Utf8(std::string& out,unsigned cp){
    if(cp<=127)out.push_back(char(cp));else if(cp<=2047){out.push_back(char(192|(cp>>6)));out.push_back(char(128|(cp&63)));}
    else if(cp<=65535){out.push_back(char(224|(cp>>12)));out.push_back(char(128|((cp>>6)&63)));out.push_back(char(128|(cp&63)));}
    else{out.push_back(char(240|(cp>>18)));out.push_back(char(128|((cp>>12)&63)));out.push_back(char(128|((cp>>6)&63)));out.push_back(char(128|(cp&63)));}
}
struct Parser {
    const std::string& text;size_t p=0;int nodes=0;
    char take(){if(p>=text.size())throw std::runtime_error("JSON truncado");return text[p++];}
    void space(){while(p<text.size()&&std::isspace(static_cast<unsigned char>(text[p])))++p;}
    unsigned hex(){unsigned n=0;for(int i=0;i<4;++i){char c=take();int d=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;if(d<0)throw std::runtime_error("Escape JSON invalido");n=n*16+unsigned(d);}return n;}
    std::string string(){if(take()!='"')throw std::runtime_error("String JSON invalida");std::string out;
        for(;;){char c=take();if(c=='"')return out;if(static_cast<unsigned char>(c)<32)throw std::runtime_error("Controle JSON invalido");if(c!='\\'){out+=c;continue;}
            switch(take()){case '"':out+='"';break;case '\\':out+='\\';break;case '/':out+='/';break;case 'b':out+='\b';break;case 'f':out+='\f';break;case 'n':out+='\n';break;case 'r':out+='\r';break;case 't':out+='\t';break;
                case 'u':{unsigned cp=hex();if(cp>=0xd800&&cp<=0xdbff){if(take()!='\\'||take()!='u')throw std::runtime_error("Surrogate JSON invalido");unsigned low=hex();if(low<0xdc00||low>0xdfff)throw std::runtime_error("Surrogate JSON invalido");cp=0x10000+((cp-0xd800)<<10)+low-0xdc00;}else if(cp>=0xdc00&&cp<=0xdfff)throw std::runtime_error("Surrogate JSON invalido");Utf8(out,cp);break;}
                default:throw std::runtime_error("Escape JSON desconhecido");}}
    }
    Json value(int depth=0){space();if(depth>32||++nodes>4096)throw std::runtime_error("Componente JSON grande demais");Json j;if(p>=text.size())throw std::runtime_error("JSON vazio");char c=text[p];
        if(c=='"'){j.kind=Json::String;j.text=string();return j;}
        if(c=='['){++p;j.kind=Json::Array;space();if(p<text.size()&&text[p]==']'){++p;return j;}for(;;){j.array.push_back(value(depth+1));space();char separator=take();if(separator==']')return j;if(separator!=',')throw std::runtime_error("Array JSON invalido");}}
        if(c=='{'){++p;j.kind=Json::Object;space();if(p<text.size()&&text[p]=='}'){++p;return j;}for(;;){space();std::string key=string();space();if(take()!=':')throw std::runtime_error("Objeto JSON invalido");j.object[key]=value(depth+1);space();char separator=take();if(separator=='}')return j;if(separator!=',')throw std::runtime_error("Objeto JSON invalido");}}
        size_t start=p;while(p<text.size()&&text[p]!=','&&text[p]!=']'&&text[p]!='}'&&!std::isspace(static_cast<unsigned char>(text[p])))++p;
        std::string atom=text.substr(start,p-start);if(atom.empty())throw std::runtime_error("Valor JSON invalido");
        if(atom!="null"){j.kind=Json::String;j.text=atom;}return j;
    }
};
std::string Flatten(const Json& j){
    std::string out;
    if(j.kind==Json::String)return j.text;
    if(j.kind==Json::Array){for(const auto& child:j.array)out+=Flatten(child);return out;}
    if(j.kind!=Json::Object)return {};
    if(j.at("text").kind!=Json::Null)out=Flatten(j.at("text"));
    else if(j.at("translate").kind!=Json::Null){
        std::string key=Flatten(j.at("translate"));std::vector<std::string> args;for(auto& arg:j.at("with").array)args.push_back(Flatten(arg));
        if((key=="chat.type.text"||key=="chat.type.announcement"||key=="chat.type.emote")&&args.size()>=2)
            out=key=="chat.type.text"?"<"+args[0]+"> "+args[1]:key=="chat.type.announcement"?"["+args[0]+"] "+args[1]:"* "+args[0]+" "+args[1];
        else if((key=="commands.message.display.incoming"||key=="commands.message.display.outgoing")&&args.size()>=2)
            out=(key=="commands.message.display.incoming"?"De ":"Para ")+args[0]+": "+args[1];
        else if(key=="multiplayer.player.joined"&&!args.empty())out=args[0]+" entrou no jogo";
        else if(key=="multiplayer.player.left"&&!args.empty())out=args[0]+" saiu do jogo";
        else{out=key;for(auto& arg:args)out+=" "+arg;}
    }else if(j.at("score").kind==Json::Object)out=Flatten(j.at("score").at("value"));
    else if(j.at("selector").kind!=Json::Null)out=Flatten(j.at("selector"));
    for(auto& child:j.at("extra").array)out+=Flatten(child);
    if(out.size()>32768)out.resize(32768);
    return out;
}
}
std::string StripMinecraftFormatting(const std::string& text){std::string out;
    for(size_t i=0;i<text.size();){size_t marker=0;if(static_cast<unsigned char>(text[i])==0xa7)marker=1;else if(i+1<text.size()&&static_cast<unsigned char>(text[i])==0xc2&&static_cast<unsigned char>(text[i+1])==0xa7)marker=2;
        if(marker&&i+marker<text.size()){char code=char(std::tolower(static_cast<unsigned char>(text[i+marker])));if((code>='0'&&code<='9')||(code>='a'&&code<='f')||(code>='k'&&code<='o')||code=='r'){i+=marker+1;continue;}}
        unsigned char c=text[i++];if(c>=32||c=='\n'||c=='\t')out+=char(c);
    }return out;
}
std::string PlainMinecraftText(const std::string& component){
    if(component.size()>32768)return "Mensagem muito longa";
    try{Parser parser{component};Json root=parser.value();parser.space();if(parser.p!=component.size())throw std::runtime_error("JSON incompleto");return StripMinecraftFormatting(Flatten(root));}
    catch(...){return StripMinecraftFormatting(component);}
}
}
