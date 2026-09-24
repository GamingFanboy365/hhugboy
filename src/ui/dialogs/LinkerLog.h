
#ifndef HHUGBOY_LINKERLOG_H
#define HHUGBOY_LINKERLOG_H

#ifndef HHUGBOY_SDL
#include <windef.h>
#endif

#define ID_LINKER_LOG 189
#define ID_LINKER_LOG_LOG 188
#define ID_LINKER_LOG_BTN 187
#define ID_LINKER_LOG_BTN_STOP 186
#define ID_LINKER_LOG_CHECKBOX 185

class LinkerLog {
#ifndef HHUGBOY_SDL
private:
    static bool linkerInitialised;
    static HWND linkerLogDialog;
public:
    static BOOL CALLBACK LinkerLogDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
#endif
public:
    static void SpawnLinkerLog();
    static void addMessage(const wchar_t* message);
    static void addMessage(const char* message);
};

#endif //HHUGBOY_LINKERLOG_H
