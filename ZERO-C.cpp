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
constexpr UINT WM_BUILD_OUTPUT = WM_APP + 1;
constexpr UINT WM_BUILD_DONE = WM_APP + 2;

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

void OpenSpecificFile(const std::wstring& path)
{
    if (!ConfirmDiscard()) {
        return;
    }

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
    UpdateTitle();
    RefreshSourceFiles();
    SetStatus(L"Ready  •  " + currentPath);
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
    UpdateTitle();
    RefreshSourceFiles();
    SetStatus(L"Saved  •  " + currentPath);
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
    currentPath.clear();
    lastExecutable.clear();
    dirty = false;
    loadingFile = true;
    SetWindowTextW(editorWindow, L"");
    loadingFile = false;
    UpdateTitle();
    RefreshSourceFiles();
    SetStatus(L"New C source file");
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
    FillRect(dc, &icon, fill);
    DeleteObject(fill);
    SetTextColor(dc, RGB(238, 222, 223));
    DrawTextW(dc, L"EX", -1, &icon, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

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

LRESULT CALLBACK EditorSubclass(HWND window, UINT message, WPARAM wParam,
                                LPARAM lParam, UINT_PTR, DWORD_PTR)
{
    if (message == WM_KEYDOWN && wParam == VK_TAB) {
        SendMessageW(window, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(L"    "));
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
    }
    return DefSubclassProc(window, message, wParam, lParam);
}

LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
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
        RefreshSourceFiles();
        LayoutControls(window);
        return 0;
    }
    case WM_SIZE:
        LayoutControls(window);
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
    windowClass.hIcon = LoadIconW(nullptr, MAKEINTRESOURCEW(32512));
    if (!RegisterClassW(&windowClass)) {
        MessageBoxW(nullptr, L"Could not register the editor window.",
                    L"ZERO C", MB_OK | MB_ICONERROR);
        return 1;
    }

    mainWindow = CreateWindowExW(0, windowClass.lpszClassName,
                                 L"ZERO C — Untitled.c",
                                 WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                 1100, 800, nullptr, nullptr, instance, nullptr);
    if (!mainWindow) {
        MessageBoxW(nullptr, L"Could not create the editor window.",
                    L"ZERO C", MB_OK | MB_ICONERROR);
        return 1;
    }
    ShowWindow(mainWindow, showCommand);
    UpdateWindow(mainWindow);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}
