#include <Windows.h>


//ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    // メッセージに応じて固有の処理を行う
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



//Windoesアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

  //  WNDCLASS wc{};
    // ウィンドウプロシージャ
   // wc.lpfnWndProc = WindowProc;
    // ウィンドウクラス名    
   // wc.lpszClassName = L"CG2WindowClass";
    // インスタンスハンドル
   // wc.hInstance = GetModuleHandle(nullptr);
    //カーソル
    //wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    //ウィンドウクラスを登録する
    //ReGisterClass(&wc); 

    // 出力ウィンドウへの文字入力
    OutputDebugStringA("Hello, DirectX!\n");

    return 0;
}

  