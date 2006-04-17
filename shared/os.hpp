// wraps misc. POSIX vs. Windows facilities
#ifndef OS_HPP
#define OS_HPP

#include <fstream>
#include <string>
#include <cstdio>
#include <stdexcept>
#include <stdlib.h>
#include <cstring>
#include <sstream>
//#include "info_debug.hpp"
#include <graehl/shared/shell_escape.hpp>

#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32)
# define OS_WINDOWS
#else
#include <unistd.h>
#endif

#if !defined( MEMMAP_IO_WINDOWS ) && !defined( MEMMAP_IO_POSIX )
# if defined(OS_WINDOWS) || defined(__CYGWIN__)
#  define MEMMAP_IO_WINDOWS
#  ifndef _WIN32_WINNT
#   define _WIN32_WINNT 0x0500
#  endif
# else
#  define MEMMAP_IO_POSIX
# endif
#endif


#ifdef MEMMAP_IO_WINDOWS
# define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers
# include <windows.h>
# undef max
// WTF, windows?  a "max" macro?  don't you think that might conflict with a max() function or method?
typedef DWORD Error;

inline long get_process_id() {
    return GetCurrentProcessId();
}

#else
# include <unistd.h>
inline long get_process_id() {
    return getpid();
}
# include <errno.h>
# include <string.h>
typedef int Error;
#endif


inline int system_safe(const std::string &cmd) 
{
    int ret=::system(cmd.c_str());
    if (ret!=0)
        throw std::runtime_error(cmd);
    return ret;
}

inline int system_shell_safe(const std::string &cmd) 
{
    const char *shell="/bin/sh -c ";
    std::stringstream s;
    s << shell;
    out_shell_quote(s,cmd);
    return system_safe(s.str());
}

inline void copy_file(const std::string &source, const std::string &dest, bool skip_same_size_and_time=false)
{
    const char *rsync="rsync -qt";
    const char *cp="/bin/cp -p";
    std::stringstream s;
    s << (skip_same_size_and_time ? rsync : cp) << ' ';
    out_shell_quote(s,source);
    s << ' ';
    out_shell_quote(s,dest);
//    INFOQ("copy_file",s.str());
    system_safe(s.str());
}

inline void mkdir_parents(const std::string &dirname)
{
    const char *mkdir="/bin/mkdir -p ";
    std::stringstream s;
    s << mkdir;
    out_shell_quote(s,dirname);
    system_safe(s.str());
}

#ifdef OS_WINDOWS
# include <direct.h>
#endif

inline std::string get_current_dir() {
#ifdef OS_WINDOWS
  char *malloced=_getcwd(NULL,0);
#else
    char *malloced=::getcwd(NULL,0);
#endif
    std::string ret(malloced);
    free(malloced);
    return ret;
}


#ifdef DBP_OS_HPP
#include <graehl/shared/debugprint.hpp>
#endif


inline Error last_error() {
#ifdef MEMMAP_IO_WINDOWS
    return ::GetLastError();
#else
    return errno;
#endif
}

inline std::string error_string(Error err) {
#ifdef MEMMAP_IO_WINDOWS
    LPVOID lpMsgBuf;
    if (::FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR) &lpMsgBuf,
            0, NULL ) == 0)
        throw std::runtime_error("couldn't generate Windows error message string");
    std::string ret((LPTSTR) lpMsgBuf);
    ::LocalFree(lpMsgBuf);
    return ret;
#else
    return strerror(err);
#endif
}

inline std::string last_error_string() {
    return error_string(last_error());
}


inline bool create_file(const std::string& path,std::size_t size) {
#ifdef _WIN32
#if 0
    //VC++ only, unfortunately
    int fd=::_open(path.c_str(),_O_CREAT|_O_SHORT_LIVED);
    if (fd == -1)
        return false;
    if (::_chsize(fd,size) == -1)
        return false;
    return ::_close(fd) != -1;
#else
    HANDLE fh=::CreateFileA( path.c_str(),GENERIC_WRITE,FILE_SHARE_DELETE,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_TEMPORARY,NULL);
    if (fh == INVALID_HANDLE_VALUE)
        return false;
    if(::SetFilePointer(fh,size,NULL,FILE_BEGIN) != size)
        return false;
    if (!::SetEndOfFile(fh))
        return false;
    return ::CloseHandle(fh);
#endif
#else
    return ::truncate(path.c_str(),size) != -1;
#endif
}

inline bool remove_file(const std::string &filename) {
    return 0==remove(filename.c_str());
}

//#include <stdio.h>

struct tmp_fstream
{
    std::string filename;
    std::fstream file;
    bool exists;
    explicit tmp_fstream(const char *c)
    {
        choose_name();
        open();
        file << c;
//        DBP(c);
        reopen();
    }
    tmp_fstream(std::ios::openmode mode=std::ios::in | std::ios::out | std::ios::trunc )
    {
        choose_name();
        open(mode);
    }
    void choose_name()
    {
        filename=std::tmpnam(NULL);
#ifdef DBP_OS_HPP
        DBP(filename);
#endif
    }
    void open(std::ios::openmode mode=std::ios::in | std::ios::out | std::ios::trunc) {
        file.open(filename.c_str(),mode);
        if (!file)
            throw std::ios::failure(std::string("couldn't open temporary file ").append(filename));
    }
    void reopen()
    {
        file.flush();
        file.seekg(0);
    }
    void close()
    {
        file.close();
    }
    void remove()
    {
        remove_file(filename);
    }

    ~tmp_fstream()
    {
        close();
        remove();
    }
};


inline void throw_last_error(const std::string &module="ERROR")
{
    throw std::runtime_error(module+": "+last_error_string());    
}

#define TMPNAM_SUFFIX "XXXXXX"
#define TMPNAM_SUFFIX_LEN 6

inline bool is_tmpnam_template(const std::string &filename_template) 
{
    unsigned len=filename_template.length();
    return !(len<TMPNAM_SUFFIX_LEN || filename_template.substr(len-TMPNAM_SUFFIX_LEN,TMPNAM_SUFFIX_LEN)!=TMPNAM_SUFFIX);
}


#ifndef OS_WINDOWS
//FIXME: provide win32 implementations

//!< file is removed if keepfile==false (dangerous: another program could grab the filename first!).  returns filename created.  if template is missing XXXXXX, it's appended first.
inline std::string safe_tmpnam(const std::string &filename_template="/tmp/safe_tmpnam.XXXXXX", bool keepfile=false) 
{
    const unsigned MY_MAX_PATH=1024;
    char tmp[MY_MAX_PATH+1];
    std::strncpy(tmp, filename_template.c_str(),MY_MAX_PATH-TMPNAM_SUFFIX_LEN);
    
    if (!is_tmpnam_template(filename_template))
        std::strcpy(tmp+filename_template.length(),TMPNAM_SUFFIX);
    
    int fd=::mkstemp(tmp);

    if (fd==-1)
        throw_last_error(std::string("safe_tmpnam couldn't mkstemp ").append(tmp));

    ::close(fd);
    
    if (!keepfile)
        ::unlink(tmp);
    
    return tmp;
}

inline std::string maybe_tmpnam(const std::string &filename_template="/tmp/safe_tmpnam.XXXXXX", bool keepfile=true)
{
    return is_tmpnam_template(filename_template) ?
        safe_tmpnam(filename_template,keepfile) :
        filename_template;
}


inline bool safe_unlink(const std::string &file,bool must_succeed=true) 
{
    if (::unlink(file.c_str()) == -1) {
        if (must_succeed)
            throw_last_error(std::string("couldn't remove ").append(file));
        return false;
    } else {
        return true;
    }
}
#endif

//!< returns dir/name unless dir is empty (just name, then).  if name begins with / then just returns name.
inline std::string joined_dir_file(const std::string &basedir,const std::string &name="",char pathsep='/') 
{
    if (!name.empty() && name[0]==pathsep) //absolute name
        return name;
    if (basedir.empty()) // relative base dir
        return name;
    if (basedir[basedir.length()-1]!=pathsep) // base dir doesn't already end in pathsep
        return basedir+pathsep+name;
    return basedir+name;
}

//FIXME: test
inline void split_dir_file(const std::string &fullpath,std::string &dir,std::string &file,char pathsep='/')
{
    using namespace std;
    string::size_type p=fullpath.rfind(pathsep);
    if (p==string::npos) {
        dir=".";
        file=fullpath;
    } else {
        dir=fullpath.substr(0,p);
        file=fullpath.substr(p+1,fullpath.length()-(p+1));
    }   
}


#ifdef OS_WINDOWS
# ifdef max
#  undef max
# endif
# ifdef min
#  undef min
# endif
#endif

#endif
