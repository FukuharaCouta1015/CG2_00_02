#include <Windows.h>
#include <cassert>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <format>
#include <string>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

std::wstring ConvertString(const std::string& str)
{
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

std::string ConvertString(const std::wstring& str)
{
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

    IDXGIFactory7* dxgiFactory = nullptr;

    HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));

    assert(SUCCEEDED(hr));

    IDXGIAdapter4* uesAdapter = nullptr;

    for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&uesAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC3 adapterDesc {};
        hr = uesAdapter->GetDesc3(&adapterDesc);
        assert(SUCCEEDED(hr));
        if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
            Log(ConvertString(std::format(L"Use Adapter:{}\n", adapterDesc.Description)));
            break;
        }
        uesAdapter = nullptr;
    }
    assert(uesAdapter != nullptr);

    ID3D12Device* device = nullptr;

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_2,
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
    };

    const char* featureLevelsStrings[] = { "12.2", "12.1", "12.0" };

    for (size_t i = 0; i < _countof(featureLevels); ++i) {
        hr = D3D12CreateDevice(uesAdapter, featureLevels[i], IID_PPV_ARGS(&device));
        if (SUCCEEDED(hr)) {
            Log(std::format("FeatureLevel:{}\n", featureLevelsStrings[i]));
            break;
        }
    }
    assert(device != nullptr);
    Log("Complete create  D3D12Device!!!\n");

    Log(ConvertString(std::format(L"-------------------------------WSTRING{}\n", L"abc")));

    // コマンドキューを生成する
    ID3D12CommandQueue* commandQueue = nullptr;
    D3D12_COMMAND_QUEUE_DESC commandQueueDesc {};
    hr = device->CreateCommandQueue(&commandQueueDesc,
        IID_PPV_ARGS(&commandQueue));

    assert(SUCCEEDED(hr));

    // コマンドアロケータを生成する
    ID3D12CommandAllocator* commandAllocator = nullptr;
    hr = device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&commandAllocator));
    // コマンドアロケータの生成がうまくいかなかったので起動出来ない
    assert(SUCCEEDED(hr));

    // コマンドリストを生成する

    ID3D12GraphicsCommandList* commandList = nullptr;
    hr = device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        commandAllocator,
        nullptr,
        IID_PPV_ARGS(&commandList));
    // コマンドリストの生成がうまくいかなかったので起動出来ない
    assert(SUCCEEDED(hr));

    // スワップチェーンを生成する
    IDXGISwapChain4* swapChain = nullptr;
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc {};
    swapChainDesc.Width = kClinetWidth; // 画面の幅,ウィンドウのクライアント領域を同じものにする
    swapChainDesc.Height = kClineHeigth; // 画面の高さ,ウィンドウのクライアント領域を同じものにする
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 画面の形式
    swapChainDesc.SampleDesc.Count = 1; //マルチサンプルしない
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 描画のターゲットとして利用する
    swapChainDesc.BufferCount = 2; // バッファの数
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; //　モニターにうつしたら中身を破棄

    hr = dxgiFactory->CreateSwapChainForHwnd(
        commandQueue, 
        hwnd, 
        &swapChainDesc, 
        nullptr, 
        nullptr, 
        reinterpret_cast<IDXGISwapChain1**>(&swapChain)); // スワップチェーンの取得
    assert(SUCCEEDED(hr));

    // ディスクリプタヒープの生成
    ID3D12DescriptorHeap* rtvDescriptorHeap = nullptr;
    D3D12_DESCRIPTOR_HEAP_DESC rtvDescriptorHeapDesc {};
    rtvDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; // レンダーターゲットビュー用
    rtvDescriptorHeapDesc.NumDescriptors = 2; // ダブルバッファように2つ,多くても別にかまわない
    hr = device->CreateDescriptorHeap(&rtvDescriptorHeapDesc, IID_PPV_ARGS(&rtvDescriptorHeap));
    assert(SUCCEEDED(hr));

    // SwapChainからResourceを取得する
    ID3D12Resource* swapChainResources[2] = {nullptr};
    hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
    assert(SUCCEEDED(hr));
    hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
    assert(SUCCEEDED(hr));

    //RTVの設定
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc {};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 出力結果をSRGBに変換して書き込む
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
    // ディスクリプタの先頭を取得する
    D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    //2つ用意
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
    //1つめを作る
    rtvHandles[0] = rtvStartHandle;
    device->CreateRenderTargetView(swapChainResources[0], &rtvDesc, rtvHandles[0]);
    // 2つめのディスクリプタハンドルを得る
    rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV); 
    // 2つめを作る
    device->CreateRenderTargetView(swapChainResources[1], &rtvDesc, rtvHandles[1]);

    // これから書き込むバックバファの禁書目録を取得
    UINT backBufferindex = swapChain->GetCurrentBackBufferIndex();
    //　描画先のRTVを設定する
    commandList->OMSetRenderTargets(1, &rtvHandles[backBufferindex], false, nullptr);
    // 指定した色で画面全体をクリアする
    float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
    commandList->ClearRenderTargetView(rtvHandles[backBufferindex], clearColor, 0, nullptr);
    // コマンドリストの内容を確定させる。全てのコマンドを積んでからCloseすること
    hr = commandList->Close();
    assert(SUCCEEDED(hr));
    // GPUにコマンドリストを実行させる
    ID3D12CommandList* commandLists[] = { commandList };
    commandQueue->ExecuteCommandLists(1, commandLists);
    // GPUとosに画面の交換を行うように通知する
    swapChain->Present(1, 0);
    // 次のフレーム用のコマンドリストを準備
    hr = commandAllocator->Reset();
    assert(SUCCEEDED(hr));
    hr = commandList->Reset(commandAllocator, nullptr);
    assert(SUCCEEDED(hr));


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