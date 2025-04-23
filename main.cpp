#include <Windows.h>
#include <cstdint>
#include <string>
#include <format>

std::wstring ConvertString(const std::string& str) {
    if (str.empty()) {
        return std::wstring();
    }

    auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);

    if (sizeNeeded == 0) {
        return std::wstring();
    }
    std::wstring result(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
    return result;
}

std::string ConvertString(const std::wstring& str) {
    if (str.empty()) {
        return std::string();
    }
    auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);

    if (sizeNeeded == 0) {
        return std::string();
    }

    std::string result(sizeNeeded, 0);

    WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), &result[0], sizeNeeded, NULL, NULL);

    return result;
}


void Log(const std::string& message)
{
    OutputDebugStringA(message.c_str());
}



// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{

    // メッセージに応じてゲーム固有の処理を行う

    switch (msg) {
        // ウィンドウが破棄された
    case WM_DESTROY:
        // OSに対して,アプリ終了を伝える
        PostQuitMessage(0);
        return 0;
    }

    // 標準のメッセージ処理を行う
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

// Windoesアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{

    WNDCLASS wc {};
    // ウィンドウプロシージャ
    wc.lpfnWndProc = WindowProc;
    // ウィンドウクラス名
    wc.lpszClassName = L"CG2WindowClass";
    // インスタンスハンドル
    wc.hInstance = GetModuleHandle(nullptr);
    // カーソル
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    // ウィンドウクラスを登録する
    RegisterClass(&wc);

    // 出力ウィンドウへの文字入力
    OutputDebugStringA("Hello, DirectX!\n");

    // クライアント領域のサイズ
    const int32_t kClinetWidth = 1280; // 幅
    const int32_t kClineHeigth = 720; // 高さ

    // ウィンドウサイズを表す構造体にクライアント領域を入れる
    RECT wrc = { 0, 0, kClinetWidth, kClineHeigth };

    // クライアント領域を元に実際のサイズにwrcを変更してもらう
    AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

    // ウィンドウの作成
    HWND hwnd = CreateWindow(
        wc.lpszClassName, // 利用するクラス名
        L"CG2", // タイトルバーの文字
        WS_OVERLAPPEDWINDOW, // ウィンドウスタイル
        CW_USEDEFAULT, // x座標
        CW_USEDEFAULT, // y座標
        wrc.right - wrc.left, // 幅
        wrc.bottom - wrc.top, // 高さ
        nullptr,
        nullptr,
        wc.hInstance, // インスタンスハンドル
        nullptr // オプション
    );

    // ウィンドウを表示する

    ShowWindow(hwnd, SW_SHOW);

            Log(ConvertString(std::format(L"-------------------------------WSTRING{}\n", L"abc")));

    MSG msg {};
    // ウィンドウのxボタンが押されるまでループ
    while (msg.message != WM_QUIT) {
        // Windowにメッセージが来てたら最優先で処理させる

        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            // メッセージを処理する
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            // ゲームの更新処理を行う
            // ここにゲームの更新処理を書く
        }

    }

        Log(ConvertString(std::format(L"-------------------------------WSTRING{}\n", L"abc")));

    return 0;
}