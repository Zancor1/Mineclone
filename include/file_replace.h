#pragma once
#include <filesystem>
#include <string>
#include <system_error>
#ifdef _WIN32
#include "platform_socket.h"
#endif
namespace game {
inline void ReplaceSaveFile(const std::filesystem::path& temporary,const std::filesystem::path& path,std::error_code& error){
#ifdef _WIN32
    if(MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))error.clear();
    else error=std::error_code(int(GetLastError()),std::system_category());
#else
    std::filesystem::rename(temporary,path,error);
#endif
}
}
