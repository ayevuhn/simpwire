/*
Copyright (c) 2019 Ivan Brebric

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software
is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#ifndef SPW_COMMON_HPP_
#define SPW_COMMON_HPP_

#include <unordered_map>
#include <string>

//Windows specific DLL import/export markers
#ifdef _WIN32

#ifdef BUILDING_DLL
#define DLL_IMPORT_EXPORT __declspec(dllexport)
#else
#define DLL_IMPORT_EXPORT __declspec(dllimport)
#endif

#endif //_WIN32

namespace spw
{
/**
 * @struct Message
 * Message objects are used by TcpNode
 * to deliver error messages.
*/
struct Message
{
    std::string head;
    std::string body;
};

enum class IpVersion {ANY, IPV4, IPV6};

const std::string g_version_string = "1.0.0";

/**
 * Look up version that is currently used
 * @return String that represents the version.
*/
#ifdef _WIN32
inline std::string DLL_IMPORT_EXPORT version() 
#else
inline std::string version()
#endif
{
    return g_version_string;
}

}

#endif // SPW_COMMON_HPP_
