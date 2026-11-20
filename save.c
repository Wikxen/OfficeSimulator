#include "save.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlobj.h>
#include <wchar.h>
#include <string.h>

static bool save_directory(wchar_t *folder)
{
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, folder)))
        return false;
    if (wcslen(folder) + wcslen(L"\\OfficeSimulator\\office.save-00000000.tmp") >= MAX_PATH)
        return false;
    wcscat(folder, L"\\OfficeSimulator");
    return true;
}

SaveResult load_game(Sim *s)
{
    wchar_t path[MAX_PATH];
    if (!save_directory(path)) return SAVE_IO_ERROR;
    wcscat(path, L"\\office.save");
    HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND
            ? SAVE_MISSING : SAVE_IO_ERROR;
    }
    char data[256];
    DWORD size;
    BOOL ok = ReadFile(file, data, sizeof data, &size, NULL);
    CloseHandle(file);
    if (!ok) return SAVE_IO_ERROR;
    if (size >= sizeof data || memchr(data, '\0', size)) return SAVE_INVALID;
    data[size] = '\0';
    return sim_decode(data, s) ? SAVE_OK : SAVE_INVALID;
}

bool save_game(const Sim *s)
{
    wchar_t path[MAX_PATH], temporary[MAX_PATH];
    if (!save_directory(path)) return false;
    if (!CreateDirectoryW(path, NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
        return false;
    swprintf(temporary, MAX_PATH, L"%ls\\office.save-%08lx.tmp", path, GetCurrentProcessId());
    wcscat(path, L"\\office.save");
    char data[256];
    int size = sim_encode(s, data, sizeof data);
    if (size <= 0 || (size_t)size >= sizeof data) return false;
    HANDLE file = CreateFileW(temporary, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD written;
    bool ok = WriteFile(file, data, (DWORD)size, &written, NULL)
        && written == (DWORD)size && FlushFileBuffers(file);
    if (!CloseHandle(file)) ok = false;
    if (ok) ok = MoveFileExW(temporary, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    if (!ok) DeleteFileW(temporary);
    return ok;
}
