#include <Windows.h>

//Windoesアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    // 出力ウィンドウへの文字入力
    OutputDebugStringA("Hello, DirectX!\n");

    return 0;
}

  