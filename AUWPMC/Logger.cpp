#include "Logger.h"
#include <Shlobj.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.h>
#include "Cheat.h"

bool Enabled = true;

void Logger::Log(const char* format, ...)
{
    SYSTEMTIME rawtime;
    GetSystemTime(&rawtime);
    char buf[MAX_PATH];
    auto size = GetTimeFormatA(LOCALE_CUSTOM_DEFAULT, 0, &rawtime, "[HH':'mm':'ss] ", buf, MAX_PATH) - 1;
    size += snprintf(buf + size, sizeof(buf) - size, "[TID: 0x%X] ", GetCurrentThreadId());
    va_list argptr;
    va_start(argptr, format);
    size += vsnprintf(buf + size, sizeof(buf) - size, format, argptr);
    WriteFile(file, buf, size, NULL, NULL);
    va_end(argptr);
}


bool Logger::Remove()
{
    if (!file) return true;
    return CloseHandle(file);
}

bool Logger::Init(std::wstring Param)
{
    // C:\\Users\\Dev\\AppData\\Local\\Packages\\StudioWildcard.4558480580BB9_1w2mm55455e38\\TempState\\Debug.txt

    if (Enabled)
    {
        //RetrieveUWPFolder();
        //std::wstring zinger = s2ws(RetrieveUWPFolder());
        //std::wstring FilePath(L"C:\\Users\\Dev\\AppData\\Local\\Packages\\StudioWildcard.4558480580BB9_1w2mm55455e38\\TempState\\Debug.txt");
        file = CreateFileW(Param.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        return file != INVALID_HANDLE_VALUE;
    }
}
//DUMP------------------------------------------------------

void Logger::Print(const char* format, ...)
{
    char Buf[MAX_PATH];
    va_list argptr;
    va_start(argptr, format);
    auto Size = vsnprintf(Buf, sizeof(Buf), format, argptr);
    WriteFile(File2, Buf, Size, NULL, NULL);
    va_end(argptr);
}

bool Logger::Init2()
{
    WCHAR name[UNLEN + 1];
    DWORD size = UNLEN + 1;
    GetUserNameW(name, &size);
    std::wstring usernameWString = std::wstring(name);

    std::wstring basePath = L"C:\\Users\\" + usernameWString + L"\\AppData\\Local\\Packages\\StudioWildcard.4558480580BB9_1w2mm55455e38\\TempState\\";
    Settings.dllPath = std::string(basePath.begin(), basePath.end());

    std::wstring usernameString(basePath + OutputDebugFileName);
    std::wstring usernameString2(basePath + OutputFileName);
    File = CreateFileW(usernameString.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    File2 = CreateFileW(usernameString2.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}

bool Logger::Remove2()
{
    if (!File && !File2) return true;
    if (File2) CloseHandle(File2);
    return CloseHandle(File);
}

void Logger::Log2(const char* format, ...)
{
    SYSTEMTIME RawTime;
    GetSystemTime(&RawTime);
    char Buf[MAX_PATH];
    auto Size = GetTimeFormatA(LOCALE_CUSTOM_DEFAULT, 0, &RawTime, "[HH':'mm':'ss] ", Buf, MAX_PATH) - 1;
    Size += snprintf(Buf + Size, sizeof(Buf) - Size, "[TID: 0x%X] ", GetCurrentThreadId());
    va_list ArgPtr;
    va_start(ArgPtr, format);
    Size += vsnprintf(Buf + Size, sizeof(Buf) - Size, format, ArgPtr);
    WriteFile(File, Buf, Size, NULL, NULL);
    va_end(ArgPtr);
}