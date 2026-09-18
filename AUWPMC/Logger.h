#pragma once
#include <Windows.h>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <lm.h>  // UNLEN

#define OutputDebugFileName std::wstring(L"OffsetDumper.txt")
#define OutputFileName std::wstring(L"ClassDump.txt")

class Logger
{
public:
    static inline HANDLE file;
    static bool Init(std::wstring Param);
    static bool Remove();
    static void Log(const char* format, ...);
    inline static char szPath;

    //DUMP --------------------------------------------------------------
    static inline HANDLE File;
    static inline HANDLE File2;
    static inline std::wstring OutputFilename = L"OffsetDumper.txt";
    static inline std::wstring OutputFilename2 = L"ClassDump.txt";
    static bool Init2();
    static bool Remove2();
    static void Log2(const char* format, ...);
    static void Print(const char* format, ...);

};
