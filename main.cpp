#include <Windows.h>
#include <cstdint>
#include <string>
#include <format>


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

    return 0;
}