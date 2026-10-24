#pragma once
#include <cstddef>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef NOGDI
#define NOGDI
#endif
#ifndef NOUSER
#define NOUSER
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#endif
namespace game::netio {
#ifdef _WIN32
using Socket=SOCKET;
using SocketLength=int;
using PollFd=WSAPOLLFD;
constexpr Socket Invalid=INVALID_SOCKET;
inline bool Initialize(){static const bool ok=[]{WSADATA data{};return WSAStartup(MAKEWORD(2,2),&data)==0;}();return ok;}
inline int LastError(){return WSAGetLastError();}
inline bool Temporary(int e){return e==WSAEWOULDBLOCK||e==WSAEINTR;}
inline bool Connecting(int e){return e==WSAEWOULDBLOCK||e==WSAEINPROGRESS||e==WSAEALREADY;}
inline void Close(Socket fd){closesocket(fd);}
inline void Shutdown(Socket fd){shutdown(fd,SD_BOTH);}
inline bool SetNonblocking(Socket fd,bool enabled){u_long mode=enabled?1:0;return ioctlsocket(fd,FIONBIO,&mode)==0;}
inline int Poll(PollFd* fds,unsigned count,int timeout){return WSAPoll(fds,count,timeout);}
inline int Send(Socket fd,const void* data,size_t size){return send(fd,static_cast<const char*>(data),int(size),0);}
inline int Receive(Socket fd,void* data,size_t size){return recv(fd,static_cast<char*>(data),int(size),0);}
inline bool SetOption(Socket fd,int level,int option,const void* data,int size){return setsockopt(fd,level,option,static_cast<const char*>(data),size)==0;}
inline void SendTimeout(Socket fd){DWORD ms=2000;SetOption(fd,SOL_SOCKET,SO_SNDTIMEO,&ms,sizeof(ms));}
#else
using Socket=int;
using SocketLength=socklen_t;
using PollFd=pollfd;
constexpr Socket Invalid=-1;
inline bool Initialize(){return true;}
inline int LastError(){return errno;}
inline bool Temporary(int e){return e==EAGAIN||e==EWOULDBLOCK||e==EINTR;}
inline bool Connecting(int e){return e==EINPROGRESS;}
inline void Close(Socket fd){close(fd);}
inline void Shutdown(Socket fd){shutdown(fd,SHUT_RDWR);}
inline bool SetNonblocking(Socket fd,bool enabled){int flags=fcntl(fd,F_GETFL,0);return flags>=0&&fcntl(fd,F_SETFL,enabled?flags|O_NONBLOCK:flags&~O_NONBLOCK)==0;}
inline int Poll(PollFd* fds,unsigned count,int timeout){return poll(fds,count,timeout);}
inline int Send(Socket fd,const void* data,size_t size){return int(send(fd,data,size,MSG_NOSIGNAL));}
inline int Receive(Socket fd,void* data,size_t size){return int(recv(fd,data,size,0));}
inline bool SetOption(Socket fd,int level,int option,const void* data,int size){return setsockopt(fd,level,option,data,size)==0;}
inline void SendTimeout(Socket fd){timeval timeout{2,0};SetOption(fd,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));}
#endif
inline int SocketError(Socket fd){int error=0;SocketLength size=sizeof(error);
#ifdef _WIN32
    if(getsockopt(fd,SOL_SOCKET,SO_ERROR,reinterpret_cast<char*>(&error),&size)!=0)return LastError();
#else
    if(getsockopt(fd,SOL_SOCKET,SO_ERROR,&error,&size)!=0)return LastError();
#endif
    return error;
}
}
