#include <windows.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <mmsystem.h>
#include <filesystem>
#include <memory>
#include <random>
#include <vector>

using namespace Gdiplus;
namespace fs = std::filesystem;

enum class Scene { Intro, Dodging, Accepted };
constexpr int width = 500, height = 350;
constexpr int yesId = 100, noId = 101;
constexpr UINT dodgeMessage = WM_APP + 1;
HWND yesButton, noButton;
HFONT buttonFont;
Scene scene = Scene::Intro;
fs::path mediaFolder;
std::unique_ptr<Image> background;
std::vector<UINT> delays;
UINT frame = 0;
std::mt19937 randomEngine(std::random_device{}());

void stopMusic() {
    mciSendStringW(L"close valentineMusic", nullptr, 0, nullptr);
}

void armFrame(HWND window) {
    if (delays.size() > 1)
        SetTimer(window, 1, delays[frame], nullptr);
}

void loadScene(HWND window, Scene next) {
    scene = next;
    KillTimer(window, 1);
    stopMusic();
    static const wchar_t* pictures[] = {L"dr_tenna.gif", L"tennacrazy.gif", L"tennahappy.gif"};
    static const wchar_t* songs[] = {L"miketheboard.mp3", L"doomboard.mp3", L"halloffame.mp3"};
    const auto index = static_cast<unsigned>(next);
    background.reset();
    delays.clear();
    frame = 0;
    const auto picture = mediaFolder / pictures[index];
    if (fs::exists(picture)) {
        background = std::make_unique<Image>(picture.c_str());
        if (background->GetLastStatus() != Ok) {
            background.reset();
        } else {
            delays.assign(background->GetFrameCount(&FrameDimensionTime), 100);
            const auto size = background->GetPropertyItemSize(PropertyTagFrameDelay);
            if (size >= sizeof(PropertyItem)) {
                auto storage = std::make_unique<BYTE[]>(size);
                auto* item = reinterpret_cast<PropertyItem*>(storage.get());
                if (background->GetPropertyItem(PropertyTagFrameDelay, size, item) == Ok &&
                    item->type == PropertyTagTypeLong && item->value) {
                    auto* values = static_cast<const ULONG*>(item->value);
                    for (size_t i = 0; i < delays.size() && i < item->length / sizeof(ULONG); ++i)
                        delays[i] = values[i] && values[i] < 6000 ? values[i] * 10 : 100;
                }
            }
            armFrame(window);
        }
    }
    const auto song = mediaFolder / songs[index];
    if (fs::exists(song)) {
        const auto command = L"open \"" + song.wstring() + L"\" type mpegvideo alias valentineMusic";
        if (!mciSendStringW(command.c_str(), nullptr, 0, nullptr))
            mciSendStringW(L"play valentineMusic repeat", nullptr, 0, nullptr);
    }
    InvalidateRect(window, nullptr, FALSE);
}

void paint(HWND window) {
    PAINTSTRUCT info{};
    HDC dc = BeginPaint(window, &info);
    {
        Graphics graphics(dc);
        if (background) {
            graphics.DrawImage(background.get(), 0, 0, width, height);
        } else {
            const Color colors[] = {Color(39, 20, 38), Color(66, 24, 57), Color(104, 35, 74)};
            graphics.Clear(colors[static_cast<unsigned>(scene)]);
        }
        FontFamily family(L"Segoe UI");
        Font title(&family, 24, FontStyleBold, UnitPixel);
        Font response(&family, 20, FontStyleRegular, UnitPixel);
        StringFormat centered;
        centered.SetAlignment(StringAlignmentCenter);
        SolidBrush hotPink(Color(255, 105, 180));
        SolidBrush pink(Color(255, 192, 203));
        graphics.DrawString(L"Will you be my Valentine? \U0001F498", -1, &title,
                            RectF(0, 86, width, 36), &centered, &hotPink);
        if (scene == Scene::Accepted)
            graphics.DrawString(L"YAY!!! \U0001F495\U0001F970 Happy Valentine\u2019s Day!", -1, &response,
                                RectF(0, 220, width, 33), &centered, &pink);
    }
    EndPaint(window, &info);
}

LRESULT CALLBACK noSubclass(HWND button, UINT message, WPARAM wParam, LPARAM lParam,
                            UINT_PTR id, DWORD_PTR entered) {
    if (message == WM_MOUSEMOVE && !entered && IsWindowEnabled(button)) {
        TRACKMOUSEEVENT tracking{sizeof(tracking), TME_LEAVE, button, 0};
        TrackMouseEvent(&tracking);
        SetWindowSubclass(button, noSubclass, id, 1);
        PostMessageW(GetParent(button), dodgeMessage, 0, 0);
    } else if (message == WM_MOUSELEAVE) {
        SetWindowSubclass(button, noSubclass, id, 0);
    } else if (message == WM_NCDESTROY) {
        RemoveWindowSubclass(button, noSubclass, id);
    }
    return DefSubclassProc(button, message, wParam, lParam);
}

LRESULT CALLBACK windowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE: {
        const auto instance = reinterpret_cast<LPCREATESTRUCTW>(lParam)->hInstance;
        buttonFont = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                                 0, 0, 0, 0, L"Segoe UI");
        yesButton = CreateWindowW(L"BUTTON", L"YES", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                  214, 127, 72, 30, window, reinterpret_cast<HMENU>(yesId), instance, nullptr);
        noButton = CreateWindowW(L"BUTTON", L"NO", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                 214, 172, 72, 30, window, reinterpret_cast<HMENU>(noId), instance, nullptr);
        if (!yesButton || !noButton) return -1;
        SendMessageW(yesButton, WM_SETFONT, reinterpret_cast<WPARAM>(buttonFont), FALSE);
        SendMessageW(noButton, WM_SETFONT, reinterpret_cast<WPARAM>(buttonFont), FALSE);
        SetWindowSubclass(noButton, noSubclass, 1, 0);
        loadScene(window, Scene::Intro);
        return 0;
    }
    case dodgeMessage: {
        if (scene == Scene::Accepted) return 0;
        if (scene == Scene::Intro) loadScene(window, Scene::Dodging);
        std::uniform_int_distribution<int> horizontal(-150, 149), vertical(-100, 99);
        SetWindowPos(noButton, nullptr, 214 + horizontal(randomEngine), 172 + vertical(randomEngine),
                     0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == yesId && HIWORD(wParam) == BN_CLICKED && scene != Scene::Accepted) {
            loadScene(window, Scene::Accepted);
            SetWindowPos(noButton, nullptr, 214, 172, 0, 0,
                         SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
            EnableWindow(yesButton, FALSE);
            EnableWindow(noButton, FALSE);
        }
        return 0;
    case WM_TIMER:
        if (wParam == 1 && background && delays.size() > 1) {
            frame = (frame + 1) % static_cast<UINT>(delays.size());
            background->SelectActiveFrame(&FrameDimensionTime, frame);
            armFrame(window);
            InvalidateRect(window, nullptr, FALSE);
        }
        return 0;
    case WM_PAINT:
        paint(window);
        return 0;
    case WM_DESTROY:
        KillTimer(window, 1);
        stopMusic();
        background.reset();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    wchar_t executable[32768];
    const auto length = GetModuleFileNameW(nullptr, executable, 32768);
    if (!length || length == 32768) return 1;
    mediaFolder = fs::path(executable).parent_path() / L"resources";
    INITCOMMONCONTROLSEX controls{sizeof(controls), ICC_STANDARD_CLASSES};
    InitCommonControlsEx(&controls);
    GdiplusStartupInput input;
    ULONG_PTR token = 0;
    if (GdiplusStartup(&token, &input, nullptr) != Ok) return 1;

    WNDCLASSW cls{};
    cls.lpfnWndProc = windowProc;
    cls.hInstance = instance;
    cls.lpszClassName = L"TennaAsking";
    cls.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    if (!RegisterClassW(&cls)) {
        GdiplusShutdown(token);
        return 1;
    }
    constexpr DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;
    RECT area{0, 0, width, height};
    AdjustWindowRect(&area, style, FALSE);
    HWND window = CreateWindowW(cls.lpszClassName, L"\U0001F49D Valentine Question \U0001F49D", style,
                                CW_USEDEFAULT, CW_USEDEFAULT, area.right - area.left, area.bottom - area.top,
                                nullptr, nullptr, instance, nullptr);
    MSG message{};
    BOOL result = -1;
    if (window) {
        ShowWindow(window, show);
        while ((result = GetMessageW(&message, nullptr, 0, 0)) > 0) {
            if (!IsDialogMessageW(window, &message)) {
                TranslateMessage(&message);
                DispatchMessageW(&message);
            }
        }
    }
    if (buttonFont) DeleteObject(buttonFont);
    GdiplusShutdown(token);
    return result == 0 ? static_cast<int>(message.wParam) : 1;
}
