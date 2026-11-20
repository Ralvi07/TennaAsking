// Runs the Win32 proposal scene, including media playback and the button-dodging interaction.
#include <windows.h>
#include <commctrl.h>
#include <gdiplus.h>
#include <mmsystem.h>
#include <shlwapi.h>
#include <memory>
#include <random>
#include <string>
#include <vector>

using namespace Gdiplus;


// Track which stage of the interaction is active so input and rendering follow the same state.
enum class Scene { Intro, Dodging, Accepted };
constexpr int width = 500, height = 350;
constexpr int yesId = 100, noId = 101;
constexpr UINT dodgeMessage = WM_APP + 1;
HWND yesButton, noButton;
HFONT buttonFont;
Scene scene = Scene::Intro;
std::wstring audioPath;
IStream* backgroundStream;
std::unique_ptr<Image> background;
std::vector<UINT> delays;
UINT frame = 0;
std::mt19937 randomEngine(std::random_device{}());

// Group the embedded media resource details used by the application.
struct MediaResource {
    const BYTE* data;
    DWORD size;
};

MediaResource loadResource(WORD id) {
    HRSRC resource = FindResourceW(nullptr, MAKEINTRESOURCEW(id), RT_RCDATA);
    if (!resource) return {};
    HGLOBAL bytes = LoadResource(nullptr, resource);
    return {static_cast<const BYTE*>(LockResource(bytes)), SizeofResource(nullptr, resource)};
}

void stopMusic() {
    mciSendStringW(L"close valentineMusic", nullptr, 0, nullptr);
    if (!audioPath.empty()) {
        DeleteFileW(audioPath.c_str());
        audioPath.clear();
    }
}

void playMusic(MediaResource song) {
    if (!song.data || !song.size) return;
    wchar_t directory[MAX_PATH];
    const DWORD length = GetTempPathW(MAX_PATH, directory);
    if (!length || length >= MAX_PATH) return;
    const std::wstring path = std::wstring(directory) + L"TennaAsking-" +
        std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(randomEngine()) + L".mp3";
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                              FILE_ATTRIBUTE_TEMPORARY, nullptr);
    if (file == INVALID_HANDLE_VALUE) return;
    DWORD written = 0;
    const BOOL saved = WriteFile(file, song.data, song.size, &written, nullptr);
    CloseHandle(file);
    audioPath = path;
    const auto command = L"open \"" + audioPath + L"\" type mpegvideo alias valentineMusic";
    if (!saved || written != song.size ||
        mciSendStringW(command.c_str(), nullptr, 0, nullptr) ||
        mciSendStringW(L"play valentineMusic repeat", nullptr, 0, nullptr))
        stopMusic();
}

void clearBackground() {
    background.reset();
    if (backgroundStream) {
        backgroundStream->Release();
        backgroundStream = nullptr;
    }
}
void armFrame(HWND window) {
    if (delays.size() > 1)
        SetTimer(window, 1, delays[frame], nullptr);
}

void loadScene(HWND window, Scene next) {
    scene = next;
    KillTimer(window, 1);
    stopMusic();
    static constexpr WORD pictures[] = {201, 202, 203};
    static constexpr WORD songs[] = {211, 212, 213};
    const auto index = static_cast<unsigned>(next);
    clearBackground();
    delays.clear();
    frame = 0;
    const auto picture = loadResource(pictures[index]);
    if (picture.data && picture.size)
        backgroundStream = SHCreateMemStream(picture.data, picture.size);
    if (backgroundStream) {
        background.reset(Image::FromStream(backgroundStream));
        if (!background || background->GetLastStatus() != Ok) {
            clearBackground();
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
    playMusic(loadResource(songs[index]));
    InvalidateRect(window, nullptr, FALSE);
}

void paint(HWND window) {
    PAINTSTRUCT info{};
    HDC dc = BeginPaint(window, &info);
    Bitmap buffer(width, height, PixelFormat32bppRGB);
    {
        Graphics graphics(&buffer);
        graphics.Clear(Color(0, 0, 0));
        if (background)
            graphics.DrawImage(background.get(), 0, 0, width, height);
        FontFamily family(L"Segoe UI");
        Font title(&family, 24, FontStyleBold, UnitPixel);
        Font response(&family, 20, FontStyleRegular, UnitPixel);
        StringFormat centered;
        centered.SetAlignment(StringAlignmentCenter);
        SolidBrush hotPink(Color(255, 105, 180));
        SolidBrush pink(Color(255, 192, 203));
        graphics.DrawString(L"Will you be my Valentine?", -1, &title,
                            RectF(0, 86, width, 36), &centered, &hotPink);
        if (scene == Scene::Accepted)
            graphics.DrawString(L"YAY!!! Happy Valentine's Day!", -1, &response,
                                RectF(0, 220, width, 33), &centered, &pink);
    }
    Graphics(dc).DrawImage(&buffer, 0, 0, width, height);
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
        clearBackground();
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(window, message, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {

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
    HWND window = CreateWindowW(cls.lpszClassName, L"Valentine Question", style,
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
