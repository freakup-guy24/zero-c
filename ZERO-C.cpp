#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shellapi.h>
#include <algorithm>
#include <string>
#include <thread>
#include <vector>

namespace {
constexpr int ID_EDITOR = 101;
constexpr int ID_OUTPUT = 102;
constexpr int ID_OPEN = 103;
constexpr int ID_SAVE = 104;
constexpr int ID_NEW = 105;
constexpr int ID_BUILD = 106;
constexpr int ID_RUN = 107;
constexpr int ID_STATUS = 108;
constexpr int ID_FILES = 109;
constexpr int ID_EXPLORER_TITLE = 110;
constexpr int ID_TAB_TITLE = 111;
constexpr int ID_OUTPUT_TITLE = 112;
constexpr int IDI_ZERO_C = 101;
constexpr UINT WM_BUILD_OUTPUT = WM_APP + 1;
constexpr UINT WM_BUILD_DONE = WM_APP + 2;

UINT findReplaceMessage;
const COLORREF backgroundColor = RGB(24, 22, 23);
const COLORREF panelColor = RGB(32, 29, 30);
const COLORREF sidebarColor = RGB(28, 26, 27);
const COLORREF editorColor = RGB(23, 21, 22);
const COLORREF borderColor = RGB(57, 49, 51);
const COLORREF textColor = RGB(232, 226, 226);
const COLORREF mutedColor = RGB(157, 145, 147);
const COLORREF accentColor = RGB(194, 79, 82);
const COLORREF activeColor = RGB(45, 34, 35);

HWND mainWindow;
HWND editorWindow;
HWND outputWindow;
HWND statusWindow;
HWND fileListWindow;
HWND explorerTitle;
HWND tabTitle;
HWND outputTitle;
HWND openButton;
HWND saveButton;
HWND newButton;
HWND buildButton;
HWND runButton;
HFONT uiFont;
HFONT codeFont;
HBRUSH backgroundBrush;
HBRUSH panelBrush;
HBRUSH editorBrush;
HBRUSH sidebarBrush;
HBRUSH accentBrush;
std::wstring currentPath;
std::wstring appDirectory;
std::wstring lastExecutable;
std::wstring workspaceSettingsPath;
std::wstring bookmarksPath;
std::wstring findText;
std::wstring replaceText;
WCHAR findTextBuffer[256]{};
WCHAR replaceTextBuffer[256]{};
FINDREPLACEW findReplace{};
HWND findDialog;
int bookmarks[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
int bookmarkPrefixPending;
bool dirty;
bool loadingFile;
bool building;
std::thread buildThread;
std::vector<std::wstring> sourceFiles;
HFONT smallFont;

std::wstring Quote(const std::wstring& value)
{
    return L"\"" + value + L"\"";
}

std::wstring GetControlText(HWND control)
{
    const int length = GetWindowTextLengthW(control);
    std::wstring value(static_cast<size_t>(length) + 1, L'\0');
    GetWindowTextW(control, value.data(), length + 1);
    value.resize(static_cast<size_t>(length));
    return value;
}

std::wstring LeafName(const std::wstring& path)
{
    const size_t separator = path.find_last_of(L"\\/");
    return separator == std::wstring::npos ? path : path.substr(separator + 1);
}

std::wstring ParentPath(const std::wstring& path)
{
    const size_t separator = path.find_last_of(L"\\/");
    return separator == std::wstring::npos ? L"." : path.substr(0, separator);
}

std::wstring ExecutablePath(const std::wstring& source)
{
    const size_t dot = source.find_last_of(L'.');
    const size_t separator = source.find_last_of(L"\\/");
    const size_t stemEnd = dot != std::wstring::npos &&
                           (separator == std::wstring::npos || dot > separator)
                               ? dot
                               : source.size();
    return source.substr(0, stemEnd) + L".GCC.exe";
}

void SetStatus(const std::wstring& message);
void OpenSpecificFile(const std::wstring& path);

void InitializeWorkspace()
{
    wchar_t appData[MAX_PATH]{};
    const DWORD length = GetEnvironmentVariableW(L"APPDATA", appData, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return;
    }
    const std::wstring directory = std::wstring(appData) + L"\\ZERO C";
    if (!CreateDirectoryW(directory.c_str(), nullptr) &&
        GetLastError() != ERROR_ALREADY_EXISTS) {
        return;
    }
    workspaceSettingsPath = directory + L"\\workspace.ini";
}

void LoadBookmarks()
{
    bookmarksPath = currentPath;
    for (int index = 0; index < 10; ++index) {
        wchar_t key[16]{};
        wchar_t value[32]{};
        swprintf_s(key, L"Bookmark%d", index);
        GetPrivateProfileStringW(currentPath.c_str(), key, L"-1", value,
                                 static_cast<DWORD>(std::size(value)),
                                 workspaceSettingsPath.c_str());
        bookmarks[index] = _wtoi(value);
    }
}

void SaveBookmarks()
{
    if (workspaceSettingsPath.empty() || bookmarksPath.empty()) {
        return;
    }
    for (int index = 0; index < 10; ++index) {
        wchar_t key[16]{};
        wchar_t value[32]{};
        swprintf_s(key, L"Bookmark%d", index);
        swprintf_s(value, L"%d", bookmarks[index]);
        WritePrivateProfileStringW(bookmarksPath.c_str(), key, value,
                                   workspaceSettingsPath.c_str());
    }
}

void SaveWorkspace()
{
    if (workspaceSettingsPath.empty()) {
        return;
    }
    if (!currentPath.empty() && editorWindow) {
        DWORD start = 0;
        DWORD end = 0;
        SendMessageW(editorWindow, EM_GETSEL, reinterpret_cast<WPARAM>(&start),
                     reinterpret_cast<LPARAM>(&end));
        wchar_t cursor[32]{};
        swprintf_s(cursor, L"%u", start);
        WritePrivateProfileStringW(L"Workspace", L"File", currentPath.c_str(),
                                   workspaceSettingsPath.c_str());
        WritePrivateProfileStringW(L"Workspace", L"Cursor", cursor,
                                   workspaceSettingsPath.c_str());
    } else {
        WritePrivateProfileStringW(L"Workspace", L"File", L"",
                                   workspaceSettingsPath.c_str());
        WritePrivateProfileStringW(L"Workspace", L"Cursor", L"0",
                                   workspaceSettingsPath.c_str());
    }
    SaveBookmarks();

    if (mainWindow) {
        RECT bounds{};
        if (GetWindowRect(mainWindow, &bounds)) {
            const int values[] = {bounds.left, bounds.top,
                                  bounds.right - bounds.left,
                                  bounds.bottom - bounds.top};
            const wchar_t* keys[] = {L"X", L"Y", L"Width", L"Height"};
            for (size_t index = 0; index < std::size(values); ++index) {
                wchar_t value[32]{};
                swprintf_s(value, L"%d", values[index]);
                WritePrivateProfileStringW(L"Window", keys[index], value,
                                           workspaceSettingsPath.c_str());
            }
        }
    }
}

void RestoreCursorPosition()
{
    if (workspaceSettingsPath.empty()) {
        return;
    }
    const DWORD position = GetPrivateProfileIntW(L"Workspace", L"Cursor", 0,
                                                  workspaceSettingsPath.c_str());
    const LRESULT length = GetWindowTextLengthW(editorWindow);
    const DWORD boundedPosition = static_cast<DWORD>(
        std::min<LRESULT>(position, std::max<LRESULT>(0, length)));
    SendMessageW(editorWindow, EM_SETSEL, boundedPosition, boundedPosition);
}

void FindNext()
{
    if (findText.empty()) {
        return;
    }
    const std::wstring contents = GetControlText(editorWindow);
    DWORD selectionStart = 0;
    DWORD selectionEnd = 0;
    SendMessageW(editorWindow, EM_GETSEL,
                 reinterpret_cast<WPARAM>(&selectionStart),
                 reinterpret_cast<LPARAM>(&selectionEnd));
    const size_t start = std::min<size_t>(selectionEnd, contents.size());
    const bool matchCase = (findReplace.Flags & FR_MATCHCASE) != 0;
    size_t match = std::wstring::npos;
    for (size_t index = start; index + findText.size() <= contents.size(); ++index) {
        const bool equal = matchCase
            ? contents.compare(index, findText.size(), findText) == 0
            : _wcsnicmp(contents.c_str() + index, findText.c_str(), findText.size()) == 0;
        if (equal) {
            match = index;
            break;
        }
    }
    if (match == std::wstring::npos) {
        for (size_t index = 0; index < start && index + findText.size() <= contents.size(); ++index) {
            const bool equal = matchCase
                ? contents.compare(index, findText.size(), findText) == 0
                : _wcsnicmp(contents.c_str() + index, findText.c_str(), findText.size()) == 0;
            if (equal) {
                match = index;
                break;
            }
        }
    }
    if (match == std::wstring::npos) {
        SetStatus(L"Search text not found");
        return;
    }
    SendMessageW(editorWindow, EM_SETSEL, static_cast<WPARAM>(match),
                 static_cast<LPARAM>(match + findText.size()));
    SetFocus(editorWindow);
}

void ShowFindDialog(bool replace)
{
    if (findDialog && IsWindow(findDialog)) {
        SetForegroundWindow(findDialog);
        return;
    }
    findTextBuffer[0] = L'\0';
    replaceTextBuffer[0] = L'\0';
    findReplace = {};
    findReplace.lStructSize = sizeof(findReplace);
    findReplace.hwndOwner = mainWindow;
    findReplace.lpstrFindWhat = findTextBuffer;
    findReplace.wFindWhatLen = static_cast<WORD>(std::size(findTextBuffer));
    findReplace.lpstrReplaceWith = replaceTextBuffer;
    findReplace.wReplaceWithLen = static_cast<WORD>(std::size(replaceTextBuffer));
    findReplace.Flags = FR_DOWN;
    findDialog = replace ? ReplaceTextW(&findReplace) : FindTextW(&findReplace);
}

bool JumpToDiagnostic(const std::wstring& line)
{
    const size_t marker = line.find(L": error:");
    const size_t warning = line.find(L": warning:");
    const size_t note = line.find(L": note:");
    const size_t messageStart = std::min({marker, warning, note});
    if (messageStart == std::wstring::npos) {
        return false;
    }
    const size_t columnSeparator = line.rfind(L':', messageStart - 1);
    if (columnSeparator == std::wstring::npos || columnSeparator == 0) {
        return false;
    }
    const size_t lineSeparator = line.rfind(L':', columnSeparator - 1);
    if (lineSeparator == std::wstring::npos || lineSeparator + 1 == columnSeparator) {
        return false;
    }
    const std::wstring path = line.substr(0, lineSeparator);
    const int lineNumber = _wtoi(line.substr(lineSeparator + 1,
                                             columnSeparator - lineSeparator - 1).c_str());
    const int columnNumber = _wtoi(line.substr(columnSeparator + 1,
                                               messageStart - columnSeparator - 1).c_str());
    if (path.empty() || lineNumber < 1 || columnNumber < 1) {
        return false;
    }
    if (_wcsicmp(path.c_str(), currentPath.c_str()) != 0) {
        OpenSpecificFile(path);
        if (_wcsicmp(path.c_str(), currentPath.c_str()) != 0) {
            return true;
        }
    }
    const LRESULT lineIndex = SendMessageW(editorWindow, EM_LINEINDEX, lineNumber - 1, 0);
    if (lineIndex == -1) {
        SetStatus(L"Compiler diagnostic line is outside the current file");
        return true;
    }
    const LRESULT lineLength = SendMessageW(editorWindow, EM_LINELENGTH, lineIndex, 0);
    const LRESULT position = lineIndex + std::min<LRESULT>(columnNumber - 1, lineLength);
    SendMessageW(editorWindow, EM_SETSEL, position, position);
    SendMessageW(editorWindow, EM_SCROLLCARET, 0, 0);
    SetFocus(editorWindow);
    SetStatus(L"Jumped to compiler diagnostic");
    return true;
}

void UpdateTitle()
{
    std::wstring title = L"ZERO C";
    if (!currentPath.empty()) {
        title += L" — " + LeafName(currentPath);
    } else {
        title += L" — Untitled.c";
    }
    if (dirty) {
        title = L"* " + title;
    }
    SetWindowTextW(mainWindow, title.c_str());
    if (tabTitle) {
        std::wstring tab = dirty ? L"●  " : L"   ";
        tab += currentPath.empty() ? L"Untitled.c" : LeafName(currentPath);
        SetWindowTextW(tabTitle, tab.c_str());
    }
}

void SetStatus(const std::wstring& message)
{
    SetWindowTextW(statusWindow, (L"  " + message).c_str());
}

bool SaveFile(bool saveAs);
bool ConfirmDiscard();
void RefreshSourceFiles();
void OpenSpecificFile(const std::wstring& path);
void SaveWorkspace();
void ShowFindDialog(bool replace);
void FindNext();

void OpenSpecificFile(const std::wstring& path)
{
    if (!ConfirmDiscard()) {
        return;
    }
    SaveBookmarks();

    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        MessageBoxW(mainWindow, L"Could not open the selected file.",
                    L"Open failed", MB_OK | MB_ICONERROR);
        return;
    }
    const DWORD size = GetFileSize(file, nullptr);
    if (size == INVALID_FILE_SIZE || size > 16 * 1024 * 1024) {
        CloseHandle(file);
        MessageBoxW(mainWindow, L"This editor supports source files up to 16 MB.",
                    L"File too large", MB_OK | MB_ICONWARNING);
        return;
    }
    std::string bytes(size, '\0');
    DWORD read = 0;
    const BOOL succeeded = size == 0 || ReadFile(file, bytes.data(), size, &read, nullptr);
    CloseHandle(file);
    if (!succeeded || read != size) {
        MessageBoxW(mainWindow, L"Reading the selected file failed.",
                    L"Open failed", MB_OK | MB_ICONERROR);
        return;
    }

    int offset = 0;
    if (bytes.size() >= 3 &&
        static_cast<unsigned char>(bytes[0]) == 0xEF &&
        static_cast<unsigned char>(bytes[1]) == 0xBB &&
        static_cast<unsigned char>(bytes[2]) == 0xBF) {
        offset = 3;
    }
    const int charCount = MultiByteToWideChar(CP_UTF8, 0, bytes.data() + offset,
                                               static_cast<int>(bytes.size()) - offset,
                                               nullptr, 0);
    std::wstring contents(static_cast<size_t>(charCount), L'\0');
    if (charCount > 0) {
        MultiByteToWideChar(CP_UTF8, 0, bytes.data() + offset,
                            static_cast<int>(bytes.size()) - offset,
                            contents.data(), charCount);
    }
    loadingFile = true;
    SetWindowTextW(editorWindow, contents.c_str());
    loadingFile = false;
    currentPath = path;
    lastExecutable.clear();
    dirty = false;
    LoadBookmarks();
    UpdateTitle();
    RestoreCursorPosition();
    RefreshSourceFiles();
    SetStatus(L"Ready  •  " + currentPath);
    SaveWorkspace();
}

void EnumerateSourceFiles(const std::wstring& directory, int depth)
{
    if (depth > 3) return;
    WIN32_FIND_DATAW item{};
    HANDLE search = FindFirstFileW((directory + L"\\*").c_str(), &item);
    if (search == INVALID_HANDLE_VALUE) return;
    do {
        if (wcscmp(item.cFileName, L".") == 0 || wcscmp(item.cFileName, L"..") == 0) continue;
        const std::wstring fullPath = directory + L"\\" + item.cFileName;
        if (item.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            EnumerateSourceFiles(fullPath, depth + 1);
        } else {
            const wchar_t* extension = wcsrchr(item.cFileName, L'.');
            if (extension && (_wcsicmp(extension, L".c") == 0 ||
                              _wcsicmp(extension, L".h") == 0 ||
                              _wcsicmp(extension, L".cpp") == 0)) {
                sourceFiles.push_back(fullPath);
            }
        }
    } while (FindNextFileW(search, &item));
    FindClose(search);
}

void RefreshSourceFiles()
{
    if (!fileListWindow) return;
    sourceFiles.clear();
    EnumerateSourceFiles(appDirectory + L"\\C\\TC", 0);
    std::sort(sourceFiles.begin(), sourceFiles.end(),
              [](const std::wstring& left, const std::wstring& right) {
                  return _wcsicmp(LeafName(left).c_str(), LeafName(right).c_str()) < 0;
              });
    SendMessageW(fileListWindow, LB_RESETCONTENT, 0, 0);
    for (const auto& path : sourceFiles) {
        SendMessageW(fileListWindow, LB_ADDSTRING, 0,
                     reinterpret_cast<LPARAM>(LeafName(path).c_str()));
    }
    if (!currentPath.empty()) {
        for (size_t index = 0; index < sourceFiles.size(); ++index) {
            if (_wcsicmp(sourceFiles[index].c_str(), currentPath.c_str()) == 0) {
                SendMessageW(fileListWindow, LB_SETCURSEL, index, 0);
                break;
            }
        }
    }
    InvalidateRect(fileListWindow, nullptr, TRUE);
}

bool ConfirmDiscard()
{
    if (!dirty) {
        return true;
    }
    const int choice = MessageBoxW(mainWindow,
                                   L"Save your changes before continuing?",
                                   L"Unsaved changes",
                                   MB_YESNOCANCEL | MB_ICONQUESTION);
    if (choice == IDYES) {
        return SaveFile(false);
    }
    return choice == IDNO;
}

bool SaveFile(bool saveAs)
{
    const std::wstring previousPath = currentPath;
    if (currentPath.empty() || saveAs) {
        wchar_t path[MAX_PATH] = L"";
        if (!currentPath.empty()) {
            wcsncpy_s(path, currentPath.c_str(), _TRUNCATE);
        }
        OPENFILENAMEW dialog{};
        dialog.lStructSize = sizeof(dialog);
        dialog.hwndOwner = mainWindow;
        dialog.lpstrFilter = L"C source files (*.c;*.h;*.cpp)\0*.c;*.h;*.cpp\0All files (*.*)\0*.*\0";
        dialog.lpstrFile = path;
        dialog.nMaxFile = MAX_PATH;
        dialog.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
        dialog.lpstrDefExt = L"c";
        if (!GetSaveFileNameW(&dialog)) {
            return false;
        }
        currentPath = path;
    }

    const std::wstring contents = GetControlText(editorWindow);
    HANDLE file = CreateFileW(currentPath.c_str(), GENERIC_WRITE, 0, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        MessageBoxW(mainWindow, L"Could not save this file. Check that the file is writable.",
                    L"Save failed", MB_OK | MB_ICONERROR);
        return false;
    }

    const int bytesNeeded = WideCharToMultiByte(CP_UTF8, 0, contents.data(),
                                                 static_cast<int>(contents.size()),
                                                 nullptr, 0, nullptr, nullptr);
    std::string utf8(static_cast<size_t>(bytesNeeded), '\0');
    if (bytesNeeded > 0) {
        WideCharToMultiByte(CP_UTF8, 0, contents.data(),
                            static_cast<int>(contents.size()), utf8.data(),
                            bytesNeeded, nullptr, nullptr);
    }
    DWORD written = 0;
    const BOOL succeeded = utf8.empty() ||
                           WriteFile(file, utf8.data(), static_cast<DWORD>(utf8.size()),
                                     &written, nullptr);
    CloseHandle(file);
    if (!succeeded || written != utf8.size()) {
        MessageBoxW(mainWindow, L"Writing the file failed.", L"Save failed",
                    MB_OK | MB_ICONERROR);
        return false;
    }

    dirty = false;
    if (bookmarksPath.empty() ||
        _wcsicmp(previousPath.c_str(), currentPath.c_str()) != 0) {
        if (!bookmarksPath.empty() &&
            _wcsicmp(bookmarksPath.c_str(), currentPath.c_str()) != 0) {
            SaveBookmarks();
        }
        LoadBookmarks();
    }
    UpdateTitle();
    RefreshSourceFiles();
    SetStatus(L"Saved  •  " + currentPath);
    SaveWorkspace();
    return true;
}

void OpenFile()
{
    wchar_t path[MAX_PATH] = L"";
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.hwndOwner = mainWindow;
    dialog.lpstrFilter = L"C source files (*.c;*.h;*.cpp)\0*.c;*.h;*.cpp\0All files (*.*)\0*.*\0";
    dialog.lpstrFile = path;
    dialog.nMaxFile = MAX_PATH;
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    if (!GetOpenFileNameW(&dialog)) {
        return;
    }
    OpenSpecificFile(path);
}

void NewFile()
{
    if (!ConfirmDiscard()) {
        return;
    }
    SaveBookmarks();
    currentPath.clear();
    bookmarksPath.clear();
    std::fill(std::begin(bookmarks), std::end(bookmarks), -1);
    lastExecutable.clear();
    dirty = false;
    loadingFile = true;
    SetWindowTextW(editorWindow, L"");
    loadingFile = false;
    UpdateTitle();
    RefreshSourceFiles();
    SetStatus(L"New C source file");
    SaveWorkspace();
    SetFocus(editorWindow);
}

void AppendOutput(const std::wstring& text)
{
    const int end = GetWindowTextLengthW(outputWindow);
    SendMessageW(outputWindow, EM_SETSEL, end, end);
    SendMessageW(outputWindow, EM_REPLACESEL, FALSE,
                 reinterpret_cast<LPARAM>(text.c_str()));
}

void SetBuildButtons(bool enabled)
{
    EnableWindow(buildButton, enabled);
    EnableWindow(runButton, enabled);
}

void BuildProgram()
{
    if (building) {
        return;
    }
    if (currentPath.empty() || dirty) {
        if (!SaveFile(false)) {
            return;
        }
    }
    if (buildThread.joinable()) {
        buildThread.join();
    }

    const std::wstring source = currentPath;
    const std::wstring output = ExecutablePath(source);
    const std::wstring compiler = appDirectory + L"\\GCC\\mingw64\\bin\\gcc.exe";
    const std::wstring compat = appDirectory + L"\\C\\TC\\compat";
    const std::wstring command = Quote(compiler) +
        L" -std=gnu11 -Wall -Wextra -I " + Quote(compat) +
        L" -x c " + Quote(source) + L" -o " + Quote(output);

    SetWindowTextW(outputWindow, L"");
    AppendOutput(L"Building " + LeafName(source) + L"...\r\n\r\n");
    SetStatus(L"Compiling...");
    building = true;
    SetBuildButtons(false);
    lastExecutable.clear();

    buildThread = std::thread([source, output, command]() {
        SECURITY_ATTRIBUTES security{sizeof(security), nullptr, TRUE};
        HANDLE readPipe = nullptr;
        HANDLE writePipe = nullptr;
        DWORD result = 1;
        std::wstring error;
        if (!CreatePipe(&readPipe, &writePipe, &security, 0)) {
            error = L"Could not create a compiler output pipe.";
        } else {
            SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);
            STARTUPINFOW startup{};
            startup.cb = sizeof(startup);
            startup.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
            startup.wShowWindow = SW_HIDE;
            startup.hStdOutput = writePipe;
            startup.hStdError = writePipe;
            startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
            PROCESS_INFORMATION process{};
            std::vector<wchar_t> mutableCommand(command.begin(), command.end());
            mutableCommand.push_back(L'\0');
            const std::wstring workingDirectory = ParentPath(source);
            if (CreateProcessW(nullptr, mutableCommand.data(), nullptr, nullptr, TRUE,
                               CREATE_NO_WINDOW, nullptr, workingDirectory.c_str(),
                               &startup, &process)) {
                CloseHandle(writePipe);
                writePipe = nullptr;
                char buffer[2048];
                DWORD count = 0;
                while (ReadFile(readPipe, buffer, sizeof(buffer), &count, nullptr) && count > 0) {
                    const int wideCount = MultiByteToWideChar(CP_UTF8, 0, buffer,
                                                               static_cast<int>(count),
                                                               nullptr, 0);
                    if (wideCount > 0) {
                        std::wstring outputText(static_cast<size_t>(wideCount), L'\0');
                        MultiByteToWideChar(CP_UTF8, 0, buffer, static_cast<int>(count),
                                            outputText.data(), wideCount);
                        auto* message = new std::wstring(std::move(outputText));
                        if (!PostMessageW(mainWindow, WM_BUILD_OUTPUT, 0,
                                          reinterpret_cast<LPARAM>(message))) {
                            delete message;
                        }
                    }
                }
                WaitForSingleObject(process.hProcess, INFINITE);
                DWORD exitCode = 1;
                GetExitCodeProcess(process.hProcess, &exitCode);
                result = exitCode;
                CloseHandle(process.hThread);
                CloseHandle(process.hProcess);
            } else {
                error = L"Could not start GCC. Check that GCC\\mingw64\\bin\\gcc.exe exists.";
            }
        }
        if (readPipe) CloseHandle(readPipe);
        if (writePipe) CloseHandle(writePipe);
        auto* completion = new std::wstring();
        if (!error.empty()) {
            *completion = error;
        } else if (result == 0) {
            *completion = L"BUILD_OK\n" + output;
        } else {
            *completion = L"BUILD_FAILED\n";
        }
        PostMessageW(mainWindow, WM_BUILD_DONE, static_cast<WPARAM>(result),
                     reinterpret_cast<LPARAM>(completion));
    });
}

void RunProgram()
{
    if (!lastExecutable.empty() && !dirty &&
        GetFileAttributesW(lastExecutable.c_str()) != INVALID_FILE_ATTRIBUTES) {
        const std::wstring workingDirectory = ParentPath(lastExecutable);
        const std::wstring compilerDirectory = appDirectory + L"\\GCC\\mingw64\\bin";
        const DWORD originalPathLength = GetEnvironmentVariableW(L"PATH", nullptr, 0);
        std::wstring originalPath;
        if (originalPathLength > 0) {
            originalPath.resize(originalPathLength);
            GetEnvironmentVariableW(L"PATH", originalPath.data(), originalPathLength);
            originalPath.resize(wcslen(originalPath.c_str()));
        }
        SetEnvironmentVariableW(L"PATH", (compilerDirectory + L";" + originalPath).c_str());
        SHELLEXECUTEINFOW execute{};
        execute.cbSize = sizeof(execute);
        execute.fMask = SEE_MASK_NOCLOSEPROCESS;
        execute.hwnd = mainWindow;
        execute.lpFile = lastExecutable.c_str();
        execute.lpDirectory = workingDirectory.c_str();
        execute.nShow = SW_SHOW;
        const BOOL launched = ShellExecuteExW(&execute);
        SetEnvironmentVariableW(L"PATH", originalPathLength > 0 ? originalPath.c_str() : nullptr);
        if (launched) {
            if (execute.hProcess) CloseHandle(execute.hProcess);
            SetStatus(L"Program started in its own window");
        } else {
            MessageBoxW(mainWindow, L"Could not start the compiled program.",
                        L"Run failed", MB_OK | MB_ICONERROR);
        }
        return;
    }
    BuildProgram();
}

void LayoutControls(HWND window)
{
    RECT client{};
    GetClientRect(window, &client);
    const int width = client.right;
    const int height = client.bottom;
    const int activityWidth = 48;
    const int explorerWidth = 208;
    const int topbarHeight = 58;
    const int tabsHeight = 38;
    const int statusHeight = 28;
    const int outputHeight = height < 650 ? 142 : 174;
    const int contentLeft = activityWidth + explorerWidth;
    const int contentWidth = std::max(120, width - contentLeft - 18);
    const int panelTop = topbarHeight + tabsHeight + 10;
    const int outputTop = height - outputHeight - statusHeight - 10;

    MoveWindow(openButton, width - 455, 13, 70, 32, TRUE);
    MoveWindow(saveButton, width - 377, 13, 72, 32, TRUE);
    MoveWindow(newButton, width - 297, 13, 62, 32, TRUE);
    MoveWindow(buildButton, width - 227, 13, 102, 32, TRUE);
    MoveWindow(runButton, width - 117, 13, 82, 32, TRUE);
    MoveWindow(explorerTitle, activityWidth + 16, topbarHeight + 14, explorerWidth - 28, 24, TRUE);
    MoveWindow(fileListWindow, activityWidth + 8, topbarHeight + 47,
               explorerWidth - 16, height - topbarHeight - statusHeight - 58, TRUE);
    MoveWindow(tabTitle, contentLeft + 22, topbarHeight + 8, contentWidth - 44, tabsHeight - 8, TRUE);
    MoveWindow(editorWindow, contentLeft + 12, panelTop + 12,
               contentWidth - 24, std::max(60, outputTop - panelTop - 18), TRUE);
    MoveWindow(outputTitle, contentLeft + 20, outputTop + 8, contentWidth - 40, 19, TRUE);
    MoveWindow(outputWindow, contentLeft + 12, outputTop + 31,
               contentWidth - 24, outputHeight - 43, TRUE);
    MoveWindow(statusWindow, 0, height - statusHeight, width, statusHeight, TRUE);
}

void DrawButton(const DRAWITEMSTRUCT* item)
{
    const bool primary = item->CtlID == ID_BUILD || item->CtlID == ID_RUN;
    const bool pressed = (item->itemState & ODS_SELECTED) != 0;
    COLORREF fill = primary ? (pressed ? RGB(133, 55, 58) : RGB(167, 66, 70))
                            : (pressed ? RGB(60, 49, 50) : RGB(43, 38, 39));
    HBRUSH brush = CreateSolidBrush(fill);
    HPEN pen = CreatePen(PS_SOLID, 1, primary ? RGB(199, 97, 99) : RGB(72, 62, 64));
    HGDIOBJ oldBrush = SelectObject(item->hDC, brush);
    HGDIOBJ oldPen = SelectObject(item->hDC, pen);
    RoundRect(item->hDC, item->rcItem.left, item->rcItem.top,
              item->rcItem.right, item->rcItem.bottom, 9, 9);
    SelectObject(item->hDC, oldBrush);
    SelectObject(item->hDC, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);
    wchar_t label[64]{};
    GetWindowTextW(item->hwndItem, label, 64);
    SetBkMode(item->hDC, TRANSPARENT);
    SetTextColor(item->hDC, RGB(234, 240, 250));
    HGDIOBJ oldFont = SelectObject(item->hDC, uiFont);
    DrawTextW(item->hDC, label, -1, const_cast<RECT*>(&item->rcItem),
              DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    SelectObject(item->hDC, oldFont);
}

void DrawMainSurface(HWND window)
{
    PAINTSTRUCT paint{};
    HDC dc = BeginPaint(window, &paint);
    RECT client{};
    GetClientRect(window, &client);
    const int width = client.right;
    const int height = client.bottom;
    const int activityWidth = 48;
    const int explorerWidth = 208;
    const int topbarHeight = 58;
    const int tabsHeight = 38;
    const int statusHeight = 28;
    const int outputHeight = height < 650 ? 142 : 174;
    const int contentLeft = activityWidth + explorerWidth;
    const int panelTop = topbarHeight + tabsHeight + 10;
    const int outputTop = height - outputHeight - statusHeight - 10;

    HBRUSH fill = CreateSolidBrush(backgroundColor);
    FillRect(dc, &client, fill);
    DeleteObject(fill);
    RECT topbar{0, 0, width, topbarHeight};
    fill = CreateSolidBrush(RGB(30, 27, 28));
    FillRect(dc, &topbar, fill);
    DeleteObject(fill);
    RECT activity{0, topbarHeight, activityWidth, height - statusHeight};
    fill = CreateSolidBrush(RGB(26, 23, 24));
    FillRect(dc, &activity, fill);
    DeleteObject(fill);
    RECT explorer{activityWidth, topbarHeight, contentLeft, height - statusHeight};
    FillRect(dc, &explorer, sidebarBrush);
    RECT status{0, height - statusHeight, width, height};
    FillRect(dc, &status, accentBrush);

    HPEN line = CreatePen(PS_SOLID, 1, borderColor);
    HGDIOBJ oldPen = SelectObject(dc, line);
    HGDIOBJ oldBrush = SelectObject(dc, panelBrush);
    RoundRect(dc, contentLeft + 4, topbarHeight + 2, width - 8,
              topbarHeight + tabsHeight, 12, 12);
    SelectObject(dc, editorBrush);
    RoundRect(dc, contentLeft + 4, panelTop, width - 8,
              outputTop - 8, 12, 12);
    SelectObject(dc, panelBrush);
    RoundRect(dc, contentLeft + 4, outputTop, width - 8,
              height - statusHeight - 8, 12, 12);
    MoveToEx(dc, 0, topbarHeight, nullptr);
    LineTo(dc, width, topbarHeight);
    MoveToEx(dc, activityWidth, topbarHeight, nullptr);
    LineTo(dc, activityWidth, height - statusHeight);
    MoveToEx(dc, contentLeft, topbarHeight, nullptr);
    LineTo(dc, contentLeft, height - statusHeight);
    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);
    DeleteObject(line);
    HBRUSH tabAccent = CreateSolidBrush(accentColor);
    RECT activeTab{contentLeft + 18, topbarHeight + tabsHeight - 3,
                   contentLeft + 142, topbarHeight + tabsHeight};
    FillRect(dc, &activeTab, tabAccent);
    DeleteObject(tabAccent);
    RECT accentLine{0, topbarHeight, 3, height - statusHeight};
    FillRect(dc, &accentLine, accentBrush);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(231, 220, 221));
    HGDIOBJ oldFont = SelectObject(dc, uiFont);
    RECT brand{activityWidth + 14, 0, activityWidth + explorerWidth, topbarHeight};
    DrawTextW(dc, L"ZERO C", -1, &brand, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    RECT icon{9, topbarHeight + 18, 39, topbarHeight + 48};
    fill = CreateSolidBrush(activeColor);
    HGDIOBJ logoBrush = SelectObject(dc, fill);
    HPEN logoPen = CreatePen(PS_SOLID, 2, accentColor);
    HGDIOBJ logoOldPen = SelectObject(dc, logoPen);
    RoundRect(dc, icon.left, icon.top, icon.right, icon.bottom, 8, 8);
    SelectObject(dc, logoOldPen);
    SelectObject(dc, logoBrush);
    DeleteObject(fill);
    DeleteObject(logoPen);
    SetTextColor(dc, RGB(244, 224, 225));
    DrawTextW(dc, L"0C", -1, &icon, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(dc, oldFont);
    EndPaint(window, &paint);
}

void DrawFileItem(const DRAWITEMSTRUCT* item)
{
    if (item->itemID == static_cast<UINT>(-1)) return;
    const bool selected = (item->itemState & ODS_SELECTED) != 0;
    RECT row = item->rcItem;
    row.left += 2;
    row.right -= 2;
    row.top += 1;
    row.bottom -= 1;
    HBRUSH brush = selected ? CreateSolidBrush(activeColor) : sidebarBrush;
    FillRect(item->hDC, &row, brush);
    if (selected) DeleteObject(brush);
    SetBkMode(item->hDC, TRANSPARENT);
    SetTextColor(item->hDC, selected ? RGB(238, 221, 222) : RGB(190, 180, 181));
    HGDIOBJ oldFont = SelectObject(item->hDC, uiFont);
    wchar_t filename[MAX_PATH]{};
    SendMessageW(item->hwndItem, LB_GETTEXT, item->itemID,
                 reinterpret_cast<LPARAM>(filename));
    RECT label = row;
    label.left += 25;
    DrawTextW(item->hDC, L"C", -1, &label, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    label.left += 20;
    label.right -= 6;
    DrawTextW(item->hDC, filename, -1, &label,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    SelectObject(item->hDC, oldFont);
}

void MatchBracket()
{
    const std::wstring text = GetControlText(editorWindow);
    DWORD selectionStart = 0;
    DWORD selectionEnd = 0;
    SendMessageW(editorWindow, EM_GETSEL,
                 reinterpret_cast<WPARAM>(&selectionStart),
                 reinterpret_cast<LPARAM>(&selectionEnd));
    size_t position = selectionStart;
    if (position >= text.size() ||
        (text[position] != L'(' && text[position] != L')' &&
         text[position] != L'{' && text[position] != L'}' &&
         text[position] != L'[' && text[position] != L']')) {
        if (position == 0 ||
            (text[position - 1] != L'(' && text[position - 1] != L')' &&
             text[position - 1] != L'{' && text[position - 1] != L'}' &&
             text[position - 1] != L'[' && text[position - 1] != L']')) {
            SetStatus(L"Place the caret beside a bracket to find its pair");
            return;
        }
        --position;
    }

    const wchar_t opening = text[position];
    const bool forward = opening == L'(' || opening == L'{' || opening == L'[';
    const wchar_t match = opening == L'(' ? L')' : opening == L'{' ? L'}' :
                         opening == L'[' ? L']' : opening == L')' ? L'(' :
                         opening == L'}' ? L'{' : L'[';
    int depth = 0;
    if (forward) {
        for (size_t index = position; index < text.size(); ++index) {
            if (text[index] == opening) ++depth;
            if (text[index] == match && --depth == 0) {
                SendMessageW(editorWindow, EM_SETSEL, index, index + 1);
                SetStatus(L"Matching bracket found");
                return;
            }
        }
    } else {
        for (size_t index = position + 1; index > 0; --index) {
            const size_t current = index - 1;
            if (text[current] == opening) ++depth;
            if (text[current] == match && --depth == 0) {
                SendMessageW(editorWindow, EM_SETSEL, current, current + 1);
                SetStatus(L"Matching bracket found");
                return;
            }
        }
    }
    SetStatus(L"No matching bracket found");
}

void SetBookmark(int index)
{
    DWORD start = 0;
    DWORD end = 0;
    SendMessageW(editorWindow, EM_GETSEL, reinterpret_cast<WPARAM>(&start),
                 reinterpret_cast<LPARAM>(&end));
    bookmarks[index] = static_cast<int>(start);
    SaveWorkspace();
    SetStatus(L"Bookmark " + std::to_wstring(index) + L" set");
}

void GoToBookmark(int index)
{
    if (bookmarks[index] < 0) {
        SetStatus(L"Bookmark " + std::to_wstring(index) + L" is not set");
        return;
    }
    const LRESULT length = GetWindowTextLengthW(editorWindow);
    const LRESULT position = std::min<LRESULT>(bookmarks[index], length);
    SendMessageW(editorWindow, EM_SETSEL, position, position);
    SendMessageW(editorWindow, EM_SCROLLCARET, 0, 0);
    SetFocus(editorWindow);
}

void InsertIndentedNewline(HWND editor)
{
    const std::wstring text = GetControlText(editor);
    DWORD start = 0;
    DWORD end = 0;
    SendMessageW(editor, EM_GETSEL, reinterpret_cast<WPARAM>(&start),
                 reinterpret_cast<LPARAM>(&end));
    const size_t caret = std::min<size_t>(start, text.size());
    const size_t lineStart = text.rfind(L'\n', caret == 0 ? 0 : caret - 1);
    const size_t indentStart = lineStart == std::wstring::npos ? 0 : lineStart + 1;
    size_t firstCode = indentStart;
    while (firstCode < caret && (text[firstCode] == L' ' || text[firstCode] == L'\t')) {
        ++firstCode;
    }
    std::wstring indent = text.substr(indentStart, firstCode - indentStart);
    size_t lastCode = caret;
    while (lastCode > firstCode && (text[lastCode - 1] == L' ' || text[lastCode - 1] == L'\t')) {
        --lastCode;
    }
    if (lastCode > firstCode && text[lastCode - 1] == L'{') {
        indent += L"    ";
    }
    const wchar_t newline[] = L"\r\n";
    SendMessageW(editor, EM_REPLACESEL, TRUE,
                 reinterpret_cast<LPARAM>((std::wstring(newline) + indent).c_str()));
}

LRESULT CALLBACK OutputSubclass(HWND window, UINT message, WPARAM wParam,
                                LPARAM lParam, UINT_PTR, DWORD_PTR)
{
    if (message == WM_LBUTTONDBLCLK) {
        const LRESULT character = LOWORD(SendMessageW(window, EM_CHARFROMPOS, 0, lParam));
        const LRESULT lineNumber = SendMessageW(window, EM_LINEFROMCHAR, character, 0);
        const LRESULT lineStart = SendMessageW(window, EM_LINEINDEX, lineNumber, 0);
        const LRESULT lineLength = SendMessageW(window, EM_LINELENGTH, lineStart, 0);
        const std::wstring output = GetControlText(window);
        if (lineStart >= 0 && lineLength > 0 &&
            static_cast<size_t>(lineStart + lineLength) <= output.size()) {
            const std::wstring line = output.substr(static_cast<size_t>(lineStart),
                                                    static_cast<size_t>(lineLength));
            if (!JumpToDiagnostic(line)) {
                SetStatus(L"That output line has no clickable GCC diagnostic");
            }
        }
        return 0;
    }
    return DefSubclassProc(window, message, wParam, lParam);
}

void HandleFindReplace()
{
    if (findReplace.Flags & FR_DIALOGTERM) {
        findDialog = nullptr;
        return;
    }
    findText = findTextBuffer;
    replaceText = replaceTextBuffer;
    if (findText.empty()) {
        return;
    }
    if (findReplace.Flags & FR_REPLACEALL) {
        std::wstring contents = GetControlText(editorWindow);
        size_t count = 0;
        size_t position = 0;
        while (position + findText.size() <= contents.size()) {
            const bool equal = (findReplace.Flags & FR_MATCHCASE)
                ? contents.compare(position, findText.size(), findText) == 0
                : _wcsnicmp(contents.c_str() + position, findText.c_str(), findText.size()) == 0;
            if (equal) {
                contents.replace(position, findText.size(), replaceText);
                position += replaceText.size();
                ++count;
            } else {
                ++position;
            }
        }
        SetWindowTextW(editorWindow, contents.c_str());
        SetStatus(L"Replaced " + std::to_wstring(count) + L" occurrence(s)");
        return;
    }
    if (findReplace.Flags & FR_REPLACE) {
        DWORD start = 0;
        DWORD end = 0;
        SendMessageW(editorWindow, EM_GETSEL, reinterpret_cast<WPARAM>(&start),
                     reinterpret_cast<LPARAM>(&end));
        const std::wstring contents = GetControlText(editorWindow);
        if (end > start && end - start == findText.size() &&
            static_cast<size_t>(end) <= contents.size()) {
            const bool equal = (findReplace.Flags & FR_MATCHCASE)
                ? contents.compare(start, findText.size(), findText) == 0
                : _wcsnicmp(contents.c_str() + start, findText.c_str(), findText.size()) == 0;
            if (equal) {
                SendMessageW(editorWindow, EM_REPLACESEL, TRUE,
                             reinterpret_cast<LPARAM>(replaceText.c_str()));
            }
        }
    }
    FindNext();
}

LRESULT CALLBACK EditorSubclass(HWND window, UINT message, WPARAM wParam,
                                LPARAM lParam, UINT_PTR, DWORD_PTR)
{
    if (message == WM_KEYDOWN && wParam == VK_TAB) {
        SendMessageW(window, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(L"    "));
        return 0;
    }
    if (message == WM_KEYDOWN && wParam == VK_RETURN) {
        InsertIndentedNewline(window);
        return 0;
    }
    if (message == WM_CHAR && wParam == L'\r') {
        InsertIndentedNewline(window);
        return 0;
    }
    if (message == WM_KEYDOWN && (GetKeyState(VK_CONTROL) & 0x8000)) {
        if (wParam == 'S') {
            SaveFile(false);
            return 0;
        }
        if (wParam == 'O') {
            OpenFile();
            return 0;
        }
        if (wParam == 'F') {
            ShowFindDialog(false);
            return 0;
        }
        if (wParam == 'H') {
            ShowFindDialog(true);
            return 0;
        }
        if (wParam == 'M') {
            MatchBracket();
            return 0;
        }
        if (wParam == 'K' || wParam == 'Q') {
            bookmarkPrefixPending = wParam == 'K' ? 1 : 2;
            return 0;
        }
    }
    if (message == WM_KEYDOWN && bookmarkPrefixPending &&
        wParam >= '0' && wParam <= '9') {
        const int index = static_cast<int>(wParam - '0');
        if (bookmarkPrefixPending == 1) {
            SetBookmark(index);
        } else {
            GoToBookmark(index);
        }
        bookmarkPrefixPending = 0;
        return 0;
    }
    if (message == WM_KEYDOWN && bookmarkPrefixPending &&
        !(GetKeyState(VK_CONTROL) & 0x8000)) {
        bookmarkPrefixPending = 0;
    }
    return DefSubclassProc(window, message, wParam, lParam);
}

LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (findReplaceMessage && message == findReplaceMessage) {
        HandleFindReplace();
        return 0;
    }
    switch (message) {
    case WM_CREATE: {
        mainWindow = window;
        HINSTANCE instance = GetModuleHandleW(nullptr);
        explorerTitle = CreateWindowW(L"STATIC", L"EXPLORER",
                                      WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
                                      0, 0, 0, 0, window,
                                      reinterpret_cast<HMENU>(ID_EXPLORER_TITLE), instance, nullptr);
        tabTitle = CreateWindowW(L"STATIC", L"   Untitled.c",
                                 WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
                                 0, 0, 0, 0, window,
                                 reinterpret_cast<HMENU>(ID_TAB_TITLE), instance, nullptr);
        outputTitle = CreateWindowW(L"STATIC", L"BUILD OUTPUT  •  GCC",
                                    WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
                                    0, 0, 0, 0, window,
                                    reinterpret_cast<HMENU>(ID_OUTPUT_TITLE), instance, nullptr);
        openButton = CreateWindowW(L"BUTTON", L"Open", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                   0, 0, 0, 0, window, reinterpret_cast<HMENU>(ID_OPEN), instance, nullptr);
        saveButton = CreateWindowW(L"BUTTON", L"Save", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                   0, 0, 0, 0, window, reinterpret_cast<HMENU>(ID_SAVE), instance, nullptr);
        newButton = CreateWindowW(L"BUTTON", L"New", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                  0, 0, 0, 0, window, reinterpret_cast<HMENU>(ID_NEW), instance, nullptr);
        buildButton = CreateWindowW(L"BUTTON", L"Build", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                    0, 0, 0, 0, window, reinterpret_cast<HMENU>(ID_BUILD), instance, nullptr);
        runButton = CreateWindowW(L"BUTTON", L"Run", WS_CHILD | WS_VISIBLE | BS_OWNERDRAW,
                                  0, 0, 0, 0, window, reinterpret_cast<HMENU>(ID_RUN), instance, nullptr);
        fileListWindow = CreateWindowExW(0, L"LISTBOX", L"",
                                         WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY |
                                         LBS_HASSTRINGS | LBS_OWNERDRAWFIXED | LBS_NOINTEGRALHEIGHT |
                                         WS_TABSTOP,
                                         0, 0, 0, 0, window,
                                         reinterpret_cast<HMENU>(ID_FILES), instance, nullptr);
        SendMessageW(fileListWindow, LB_SETITEMHEIGHT, 0, 30);
        editorWindow = CreateWindowExW(0, L"EDIT", L"",
                                       WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
                                       ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL |
                                       ES_NOHIDESEL | ES_WANTRETURN,
                                       0, 0, 0, 0, window, reinterpret_cast<HMENU>(ID_EDITOR), instance, nullptr);
        outputWindow = CreateWindowExW(0, L"EDIT", L"",
                                       WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE |
                                       ES_AUTOVSCROLL | ES_READONLY,
                                       0, 0, 0, 0, window, reinterpret_cast<HMENU>(ID_OUTPUT), instance, nullptr);
        statusWindow = CreateWindowW(L"STATIC", L"  READY",
                                     WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE,
                                     0, 0, 0, 0, window, reinterpret_cast<HMENU>(ID_STATUS), instance, nullptr);
        SendMessageW(editorWindow, WM_SETFONT, reinterpret_cast<WPARAM>(codeFont), TRUE);
        SendMessageW(outputWindow, WM_SETFONT, reinterpret_cast<WPARAM>(codeFont), TRUE);
        SendMessageW(statusWindow, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        SendMessageW(explorerTitle, WM_SETFONT, reinterpret_cast<WPARAM>(smallFont), TRUE);
        SendMessageW(tabTitle, WM_SETFONT, reinterpret_cast<WPARAM>(uiFont), TRUE);
        SendMessageW(outputTitle, WM_SETFONT, reinterpret_cast<WPARAM>(smallFont), TRUE);
        SetWindowSubclass(editorWindow, EditorSubclass, 1, 0);
        SetWindowSubclass(outputWindow, OutputSubclass, 2, 0);
        RefreshSourceFiles();
        LayoutControls(window);
        return 0;
    }
    case WM_SIZE:
        LayoutControls(window);
        RedrawWindow(window, nullptr, nullptr,
                     RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
        return 0;
    case WM_GETMINMAXINFO: {
        auto* limits = reinterpret_cast<MINMAXINFO*>(lParam);
        limits->ptMinTrackSize.x = 780;
        limits->ptMinTrackSize.y = 560;
        return 0;
    }
    case WM_PAINT:
        DrawMainSurface(window);
        return 0;
    case WM_ERASEBKGND:
        return 1;
    case WM_COMMAND:
        if (LOWORD(wParam) == ID_EDITOR && HIWORD(wParam) == EN_CHANGE && !loadingFile) {
            dirty = true;
            UpdateTitle();
        } else if (LOWORD(wParam) == ID_FILES && HIWORD(wParam) == LBN_DBLCLK) {
            const LRESULT selected = SendMessageW(fileListWindow, LB_GETCURSEL, 0, 0);
            if (selected >= 0 && static_cast<size_t>(selected) < sourceFiles.size()) {
                OpenSpecificFile(sourceFiles[static_cast<size_t>(selected)]);
            }
        } else if (HIWORD(wParam) == BN_CLICKED) {
            switch (LOWORD(wParam)) {
            case ID_OPEN: OpenFile(); break;
            case ID_SAVE: SaveFile(false); break;
            case ID_NEW: NewFile(); break;
            case ID_BUILD: BuildProgram(); break;
            case ID_RUN: RunProgram(); break;
            }
        }
        return 0;
    case WM_DRAWITEM:
        if (reinterpret_cast<DRAWITEMSTRUCT*>(lParam)->CtlID == ID_FILES) {
            DrawFileItem(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
        } else {
            DrawButton(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
        }
        return TRUE;
    case WM_CTLCOLOREDIT: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        HWND control = reinterpret_cast<HWND>(lParam);
        SetTextColor(dc, control == outputWindow ? RGB(205, 190, 192) : textColor);
        SetBkColor(dc, control == outputWindow ? panelColor : editorColor);
        return reinterpret_cast<INT_PTR>(control == outputWindow ? panelBrush : editorBrush);
    }
    case WM_CTLCOLORSTATIC: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        HWND control = reinterpret_cast<HWND>(lParam);
        if (control == outputWindow) {
            SetTextColor(dc, RGB(205, 190, 192));
            SetBkColor(dc, panelColor);
            return reinterpret_cast<INT_PTR>(panelBrush);
        }
        if (control == explorerTitle || control == outputTitle) {
            SetTextColor(dc, RGB(165, 151, 153));
            SetBkColor(dc, sidebarColor);
            return reinterpret_cast<INT_PTR>(sidebarBrush);
        }
        if (control == tabTitle) {
            SetTextColor(dc, dirty ? RGB(236, 197, 198) : RGB(218, 208, 209));
            SetBkColor(dc, panelColor);
            return reinterpret_cast<INT_PTR>(panelBrush);
        }
        SetTextColor(dc, RGB(255, 246, 246));
        SetBkColor(dc, accentColor);
        return reinterpret_cast<INT_PTR>(accentBrush);
    }
    case WM_CTLCOLORLISTBOX: {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, textColor);
        SetBkColor(dc, sidebarColor);
        return reinterpret_cast<INT_PTR>(sidebarBrush);
    }
    case WM_BUILD_OUTPUT: {
        auto* text = reinterpret_cast<std::wstring*>(lParam);
        if (text) {
            AppendOutput(*text);
            delete text;
        }
        return 0;
    }
    case WM_BUILD_DONE: {
        auto* completion = reinterpret_cast<std::wstring*>(lParam);
        building = false;
        SetBuildButtons(true);
        if (completion && completion->rfind(L"BUILD_OK\n", 0) == 0) {
            lastExecutable = completion->substr(9);
            AppendOutput(L"\r\nBuild succeeded. Click Run to launch the program.\r\n");
            SetStatus(L"Build succeeded  •  " + LeafName(lastExecutable));
        } else if (completion && completion->rfind(L"BUILD_FAILED\n", 0) == 0) {
            AppendOutput(L"\r\nBuild failed. Fix the reported errors and build again.\r\n");
            SetStatus(L"Build failed  •  See compiler output below");
        } else {
            const std::wstring error = completion ? *completion : L"Build failed unexpectedly.";
            AppendOutput(L"\r\n" + error + L"\r\n");
            SetStatus(error);
        }
        delete completion;
        return 0;
    }
    case WM_CLOSE:
        if (ConfirmDiscard()) {
            SaveWorkspace();
            DestroyWindow(window);
        }
        return 0;
    case WM_DESTROY:
        if (buildThread.joinable()) {
            buildThread.join();
        }
        DeleteObject(uiFont);
        DeleteObject(codeFont);
        DeleteObject(smallFont);
        DeleteObject(backgroundBrush);
        DeleteObject(panelBrush);
        DeleteObject(editorBrush);
        DeleteObject(sidebarBrush);
        DeleteObject(accentBrush);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}
} // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand)
{
    wchar_t executablePath[MAX_PATH]{};
    GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
    appDirectory = ParentPath(executablePath);
    InitializeWorkspace();
    findReplaceMessage = RegisterWindowMessageW(FINDMSGSTRINGW);

    wchar_t restorePath[MAX_PATH]{};
    int windowX = CW_USEDEFAULT;
    int windowY = CW_USEDEFAULT;
    int windowWidth = 1100;
    int windowHeight = 800;
    if (!workspaceSettingsPath.empty()) {
        GetPrivateProfileStringW(L"Workspace", L"File", L"", restorePath,
                                 static_cast<DWORD>(std::size(restorePath)),
                                 workspaceSettingsPath.c_str());
        windowX = GetPrivateProfileIntW(L"Window", L"X", CW_USEDEFAULT,
                                        workspaceSettingsPath.c_str());
        windowY = GetPrivateProfileIntW(L"Window", L"Y", CW_USEDEFAULT,
                                        workspaceSettingsPath.c_str());
        windowWidth = GetPrivateProfileIntW(L"Window", L"Width", 1100,
                                            workspaceSettingsPath.c_str());
        windowHeight = GetPrivateProfileIntW(L"Window", L"Height", 800,
                                             workspaceSettingsPath.c_str());
        if (windowWidth < 780 || windowHeight < 560) {
            windowX = CW_USEDEFAULT;
            windowY = CW_USEDEFAULT;
            windowWidth = 1100;
            windowHeight = 800;
        }
    }

    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&controls);
    uiFont = CreateFontW(-14, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE,
                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                         CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    codeFont = CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           CLEARTYPE_QUALITY, FIXED_PITCH, L"Cascadia Code");
    smallFont = CreateFontW(-11, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    backgroundBrush = CreateSolidBrush(backgroundColor);
    panelBrush = CreateSolidBrush(panelColor);
    editorBrush = CreateSolidBrush(editorColor);
    sidebarBrush = CreateSolidBrush(sidebarColor);
    accentBrush = CreateSolidBrush(accentColor);

    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = L"ZeroCNativeWindow";
    windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    windowClass.hbrBackground = backgroundBrush;
    windowClass.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_ZERO_C));
    if (!windowClass.hIcon) {
        windowClass.hIcon = LoadIconW(nullptr, MAKEINTRESOURCEW(32512));
    }
    if (!RegisterClassW(&windowClass)) {
        MessageBoxW(nullptr, L"Could not register the editor window.",
                    L"ZERO C", MB_OK | MB_ICONERROR);
        return 1;
    }

    mainWindow = CreateWindowExW(0, windowClass.lpszClassName,
                                 L"ZERO C — Untitled.c",
                                 WS_OVERLAPPEDWINDOW, windowX, windowY,
                                 windowWidth, windowHeight, nullptr, nullptr, instance, nullptr);
    if (!mainWindow) {
        MessageBoxW(nullptr, L"Could not create the editor window.",
                    L"ZERO C", MB_OK | MB_ICONERROR);
        return 1;
    }
    if (windowClass.hIcon) {
        SendMessageW(mainWindow, WM_SETICON, ICON_SMALL,
                     reinterpret_cast<LPARAM>(windowClass.hIcon));
    }
    ShowWindow(mainWindow, showCommand);
    UpdateWindow(mainWindow);
    RedrawWindow(mainWindow, nullptr, nullptr,
                 RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    if (restorePath[0] && GetFileAttributesW(restorePath) != INVALID_FILE_ATTRIBUTES) {
        OpenSpecificFile(restorePath);
    } else if (restorePath[0]) {
        SetStatus(L"Last workspace file could not be found");
    }

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}
