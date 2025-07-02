#include <Windows.h>
#include <cassert>
#include <cstdint>
#include <d3d12.h>
#include <dxcapi.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <format>
#include <string>
#include "externals/imagui/imgui.h"
#include "externals/imagui/imgui_impl_dx12.h"
#include "externals/imagui/imgui_impl_win32.h"
#include "externals/DirectXTex/DirectXTex.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

struct Vector4 {
    float x;
    float y;
    float z;
    float w;
};

struct Vector2 {
    float x;
    float y;
   
};

struct VertexData {
    Vector4 position; 
    Vector2 texcoord;
    
};

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

ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes)
{

    D3D12_HEAP_PROPERTIES uploadHeapProperties {};
    uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC vertexResourceDesc {};

    vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    vertexResourceDesc.Width = sizeInBytes; // 3頂点分のサイズ
    vertexResourceDesc.Height = 1;
    vertexResourceDesc.DepthOrArraySize = 1;
    vertexResourceDesc.MipLevels = 1;
    vertexResourceDesc.SampleDesc.Count = 1;
    vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ID3D12Resource* vertexResoursc = nullptr;
    HRESULT hr = device->CreateCommittedResource(
        &uploadHeapProperties,
        D3D12_HEAP_FLAG_NONE,
        &vertexResourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vertexResoursc));
    assert(SUCCEEDED(hr));

    return vertexResoursc;
}

ID3D12DescriptorHeap* CreateDescriptorHeap(
    ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible)
{

    // ディスクリプタヒープの生成
    ID3D12DescriptorHeap* descriptorHeap = nullptr;
    D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc {};
    descriptorHeapDesc.Type = heapType; // レンダーターゲットビュー用
    descriptorHeapDesc.NumDescriptors = numDescriptors; // ダブルバッファように2つ,多くても別にかまわない
    descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    HRESULT hr = device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
    assert(SUCCEEDED(hr));
    return descriptorHeap;
}

// Tetureデータを読み込む
DirectX::ScratchImage LoadTexture(const std::string filePath)
{
    DirectX::ScratchImage image {};
    std::wstring filePathW = ConvertString(filePath);
    HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
    assert(SUCCEEDED(hr));

    DirectX::ScratchImage mipImage {};
    hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_SRGB, 0, mipImage);
    assert(SUCCEEDED(hr));

    return mipImage;

}

// DirectX12のテクスチャリソースを作る
 
ID3D12Resource* CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata)
{
    D3D12_RESOURCE_DESC resourceDesc {};
    resourceDesc.Width = UINT(metadata.width);
    resourceDesc.Height = UINT(metadata.height);
    resourceDesc.MipLevels = UINT16(metadata.mipLevels);
    resourceDesc.DepthOrArraySize = UINT16(metadata.arraySize);
    resourceDesc.Format = metadata.format;
    resourceDesc.SampleDesc.Count = 1; // サンプル数は1
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

    D3D12_HEAP_PROPERTIES heapProperties {};
    heapProperties.Type = D3D12_HEAP_TYPE_CUSTOM; 
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;

    ID3D12Resource* resource = nullptr;
    HRESULT hr = device->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ, 
        nullptr,
        IID_PPV_ARGS(&resource));
    assert(SUCCEEDED(hr));
    return resource;
}

// テクスチャリソースにデータを転送する
void UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages)
{
    const DirectX::TexMetadata& metadata = mipImages.GetMetadata();

    for (size_t mipLevel = 0; mipLevel < metadata.mipLevels; ++mipLevel){
        const DirectX::Image* img = mipImages.GetImage(mipLevel, 0, 0);
        HRESULT hr = texture->WriteToSubresource(
            UINT(mipLevel), 
            nullptr, 
            img->pixels, 
            UINT(img->rowPitch), 
            UINT(img->slicePitch)
        );
        assert(SUCCEEDED(hr));
    }

} 

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam)) {
        return true;

    }
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

IDxcBlob* CompileShader(
    const std::wstring& filePath,
    const wchar_t* profile,

    IDxcUtils* dxcUtils,
    IDxcCompiler3* dxcCompiler,
    IDxcIncludeHandler* includeHandler)
{
    Log(ConvertString(std::format(L"Begin CompileShader, path:{},profile:{}\n", filePath, profile)));
    IDxcBlobEncoding* shaderSource = nullptr;
    HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
    assert(SUCCEEDED(hr));

    DxcBuffer shaderSourceBuffer;
    shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
    shaderSourceBuffer.Size = shaderSource->GetBufferSize();
    shaderSourceBuffer.Encoding = DXC_CP_UTF8; // UTF-8エンコーディング

    LPCWSTR arguments[] = {
        filePath.c_str(), // シェーダーファイルのパス
        L"-E",
        L"main", // エントリーポイント関数名
        L"-T",
        profile, // シェーダープロファイル
        L"-Zi",
        L"-Qembed_debug"
        L"-Od", // 最適化なし
        L"-Zpr",
    };

    IDxcResult* shaderResult = nullptr;
    hr = dxcCompiler->Compile(
        &shaderSourceBuffer,
        arguments, // コンパイル時の引数
        _countof(arguments),
        includeHandler, // インクルードハンドラー
        IID_PPV_ARGS(&shaderResult));
    assert(SUCCEEDED(hr));
    IDxcBlobUtf8* shaderErrors = nullptr;
    shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderErrors), nullptr);
    if (shaderErrors != nullptr && shaderErrors->GetStringLength() != 0) {
        Log(shaderErrors->GetStringPointer());
        assert(false);
    }
    IDxcBlob* shaderBlob = nullptr;
    hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
    assert(SUCCEEDED(hr));
    Log(ConvertString(std::format(L"Compile Succeeded,path:{},profile:{}\n", filePath, profile)));

    shaderSource->Release();
    shaderResult->Release();
    return shaderBlob;
}

IDxcBlob* ComileShader(
    const std::wstring& filePath,
    const wchar_t* profile,
    IDxcUtils* dxcUtils,
    IDxcCompiler3* dxcCompiler,
    IDxcIncludeHandler* includeHandler)
{
    Log(ConvertString(std::format(L"Begin CompileShader, path:{},profile:{}\n", filePath, profile)));
    IDxcBlobEncoding* shaderSource = nullptr;
    HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
    assert(SUCCEEDED(hr));
    DxcBuffer shaderSourceBuffer;
    shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
    shaderSourceBuffer.Size = shaderSource->GetBufferSize();
    shaderSourceBuffer.Encoding = DXC_CP_UTF8;

    LPCWSTR arguments[] = {
        filePath.c_str(),
        L"-E",
        L"main",
        L"-T",
        profile,
        L"-Zi",
        L"-Qembed_debug",
        L"-Od",
        L"-Zpr",
    };

    IDxcResult* shaderResult = nullptr;
    hr = dxcCompiler->Compile(
        &shaderSourceBuffer,
        arguments,
        _countof(arguments),
        includeHandler,
        IID_PPV_ARGS(&shaderResult));
    assert(SUCCEEDED(hr));

    IDxcBlobUtf8* shaderErrors = nullptr;
    shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderErrors), nullptr);
    if (shaderErrors != nullptr && shaderErrors->GetStringLength() != 0) {
        Log(shaderErrors->GetStringPointer());
        assert(false);
    }

    IDxcBlob* shaderBlob = nullptr;
    hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
    assert(SUCCEEDED(hr));
    Log(ConvertString(std::format(L"Compile Succeeded,path:{},profile:{}\n", filePath, profile)));
    shaderSource->Release();
    shaderResult->Release();
    return shaderBlob;
}

// Windoesアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{

    CoInitializeEx(0, COINIT_MULTITHREADED);

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

#ifdef _DEBUG

    ID3D12Debug1* debugController = nullptr;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        debugController->SetEnableGPUBasedValidation(TRUE);
    }

#endif

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

#ifdef _DEBUG

    ID3D12InfoQueue* infoQueue = nullptr;
    if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        // infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

        D3D12_MESSAGE_ID denyIds[] = {
            D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
        };

        D3D12_MESSAGE_SEVERITY severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
        D3D12_INFO_QUEUE_FILTER filter {};
        filter.DenyList.NumIDs = _countof(denyIds);
        filter.DenyList.pIDList = denyIds;
        filter.DenyList.NumSeverities = _countof(severities);
        filter.DenyList.pSeverityList = severities;

        infoQueue->PushStorageFilter(&filter);

        infoQueue->Release();
    }
#endif

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
    swapChainDesc.SampleDesc.Count = 1; // マルチサンプルしない
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // 描画のターゲットとして利用する
    swapChainDesc.BufferCount = 2; // バッファの数
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // 　モニターにうつしたら中身を破棄

    hr = dxgiFactory->CreateSwapChainForHwnd(
        commandQueue,
        hwnd,
        &swapChainDesc,
        nullptr,
        nullptr,
        reinterpret_cast<IDXGISwapChain1**>(&swapChain)); // スワップチェーンの取得
    assert(SUCCEEDED(hr));

    ID3D12DescriptorHeap* rtvDescriptorHeap = CreateDescriptorHeap(
        device,
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV, // レンダーターゲットビュー用
        2, // ダブルバッファ用に2つ
        false); // シェーダーからは使わない

     ID3D12DescriptorHeap* srvDescriptorHeap = CreateDescriptorHeap(
        device,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, // シェーダーリソースビュー用
        128, 
        true); // シェーダーから使う

    // SwapChainからResourceを取得する
    ID3D12Resource* swapChainResources[2] = { nullptr };
    hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&swapChainResources[0]));
    assert(SUCCEEDED(hr));
    hr = swapChain->GetBuffer(1, IID_PPV_ARGS(&swapChainResources[1]));
    assert(SUCCEEDED(hr));

    // RTVの設定
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc {};
    rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB; // 出力結果をSRGBに変換して書き込む
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
    // ディスクリプタの先頭を取得する
    D3D12_CPU_DESCRIPTOR_HANDLE rtvStartHandle = rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    // 2つ用意
    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandles[2];
    // 1つめを作る
    rtvHandles[0] = rtvStartHandle;
    device->CreateRenderTargetView(swapChainResources[0], &rtvDesc, rtvHandles[0]);
    // 2つめのディスクリプタハンドルを得る
    rtvHandles[1].ptr = rtvHandles[0].ptr + device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    // 2つめを作る
    device->CreateRenderTargetView(swapChainResources[1], &rtvDesc, rtvHandles[1]);

    ID3D12Fence* fence = nullptr;
    uint64_t fenceValue = 0;
    hr = device->CreateFence(
        fenceValue,
        D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(&fence));

    assert(SUCCEEDED(hr));
    HANDLE fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    assert(fenceEvent != nullptr);

    // dxcComilerを初期化する
    IDxcUtils* dxcUtils = nullptr;
    IDxcCompiler3* dxcCompiler = nullptr;
    hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&dxcUtils));
    assert(SUCCEEDED(hr));
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&dxcCompiler));
    assert(SUCCEEDED(hr));
    // dxcCompilerの初期化がうまくいかなかったので起動出来ない
    IDxcIncludeHandler* includeHandler = nullptr;
    hr = dxcUtils->CreateDefaultIncludeHandler(&includeHandler);
    assert(SUCCEEDED(hr));

    // RootSignatureの作成
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature {};
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // ルートパラメータの設定
    D3D12_ROOT_PARAMETER rootParameters[1] = {};
    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParameters[0].Descriptor.ShaderRegister = 0;
    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumParameters = _countof(rootParameters);

    ID3DBlob* signatureBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    hr = D3D12SerializeRootSignature(
        &descriptionRootSignature,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signatureBlob,
        &errorBlob);
    if (FAILED(hr)) {
        Log(reinterpret_cast<char*>(errorBlob->GetBufferPointer()));
        assert(false);
    }

    ID3D12RootSignature* rootSignature = nullptr;
    hr = device->CreateRootSignature(0,
        signatureBlob->GetBufferPointer(),
        signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature));
    assert(SUCCEEDED(hr));

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[2] = {};
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDescs[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    inputElementDescs[1].SemanticName = "TEXCOORD";
    inputElementDescs[1].SemanticIndex = 0;
    inputElementDescs[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    inputElementDescs[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    D3D12_INPUT_LAYOUT_DESC inputLayoutDesc {};
    inputLayoutDesc.pInputElementDescs = inputElementDescs;
    inputLayoutDesc.NumElements = _countof(inputElementDescs);

    D3D12_BLEND_DESC blendDesc {};
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_RASTERIZER_DESC rasterizerDesc {};

    rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;

    rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;

    IDxcBlob* vertexShaderBlob = ComileShader(
        L"Object3D.VS.hlsl",
        L"vs_6_0",
        dxcUtils,
        dxcCompiler,
        includeHandler);
    assert(vertexShaderBlob != nullptr);

    IDxcBlob* pixelShaderBlob = ComileShader(
        L"Object3D.PS.hlsl",
        L"ps_6_0",
        dxcUtils,
        dxcCompiler,
        includeHandler);
    assert(pixelShaderBlob != nullptr);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC graphicsPipelineStateDesc {};
    graphicsPipelineStateDesc.pRootSignature = rootSignature;
    graphicsPipelineStateDesc.InputLayout = inputLayoutDesc;
    graphicsPipelineStateDesc.VS = { vertexShaderBlob->GetBufferPointer(),
        vertexShaderBlob->GetBufferSize() };
    graphicsPipelineStateDesc.PS = { pixelShaderBlob->GetBufferPointer(),
        pixelShaderBlob->GetBufferSize() };
    graphicsPipelineStateDesc.BlendState = blendDesc;
    graphicsPipelineStateDesc.RasterizerState = rasterizerDesc;
    graphicsPipelineStateDesc.NumRenderTargets = 1;
    graphicsPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    graphicsPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    graphicsPipelineStateDesc.SampleDesc.Count = 1;
    graphicsPipelineStateDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    ID3D12PipelineState* graphicsPipelineState = nullptr;

    hr = device->CreateGraphicsPipelineState(
        &graphicsPipelineStateDesc,
        IID_PPV_ARGS(&graphicsPipelineState));
    assert(SUCCEEDED(hr));

    ID3D12Resource* vertexResoursc = CreateBufferResource(device, sizeof(VertexData) * 3); // 3頂点分のサイズ

    // マテリアル用のリソースを作成する
    ID3D12Resource* materialResource = CreateBufferResource(device, sizeof(Vector4));

    Vector4* materialData = nullptr;

    materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));

    // マテリアルデータを設定する
    *materialData = Vector4 { 1.0f, 0.0f, 0.0f, 1.0f };

    D3D12_VERTEX_BUFFER_VIEW vertexBufferView {};
    vertexBufferView.BufferLocation = vertexResoursc->GetGPUVirtualAddress();
    vertexBufferView.SizeInBytes = sizeof(VertexData) * 3; // 3頂点分のサイズ
    vertexBufferView.StrideInBytes = sizeof(VertexData); // 1頂点分のサイズ

    VertexData* vertexData = nullptr;

    vertexResoursc->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

    // 頂点データを設定する
    vertexData[0] = { -0.5f, -0.5f, 0.0f, 1.0f };
    vertexData[0].texcoord = { 0.0f, 1.0f };

    vertexData[1] = { 0.0f, 0.5f, 0.0f, 1.0f };
    vertexData[1].texcoord = { 0.5f, 0.0f };

    vertexData[2] = { 0.5f, -0.5f, 0.0f, 1.0f };
    vertexData[2].texcoord = { 1.0f, 1.0f };

    D3D12_VIEWPORT viewport {};

    viewport.Width = kClinetWidth;

    viewport.Height = kClineHeigth;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;

    D3D12_RECT scissorRect {};

    scissorRect.left = 0;
    scissorRect.top = 0;
    scissorRect.right = kClinetWidth;
    scissorRect.bottom = kClineHeigth;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX12_Init(
        device,
        swapChainDesc.BufferCount, // スワップチェーンのバッファ数
        rtvDesc.Format, // レンダーターゲットのフォーマップ
        srvDescriptorHeap, // シェーダーリソースビュー用のディスクリプタヒープ
        srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), // シェーダーリソースビュー用のCPU��ィスクリプタハンドル
        srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart() // シェーダーリソースビュー用のGPUディスクリプタハンドル
    );

    // Textureを読んで転送する

    DirectX::ScratchImage mipImages = LoadTexture("Resources/uvChecker.png");
    const DirectX::TexMetadata& metadata = mipImages.GetMetadata();
    ID3D12Resource* textureResource = CreateTextureResource(device, metadata);
    UploadTextureData(textureResource, mipImages);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc {};
    srvDesc.Format = metadata.format; // テクスチャのフォーマット
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // シェーダーのコンポーネントマッピング   
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // 2Dテクスチャ
    srvDesc.Texture2D.MipLevels = UINT(metadata.mipLevels); // 最初のミップマップレベル

    D3D12_CPU_DESCRIPTOR_HANDLE textureSrvHandleCPU = srvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU = srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
   
    textureSrvHandleCPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    textureSrvHandleGPU.ptr += device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // テクスチャのシェーダーリソースビューを作成する

    device->CreateShaderResourceView(textureResource,&srvDesc,textureSrvHandleCPU);




    MSG msg {};
    // ウィンドウのxボタンが押されるまでループ
    while (msg.message != WM_QUIT) {
        // Windowにメッセージが来てたら最優先で処理させる

        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            // メッセージを処理する
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {

            ImGui_ImplDX12_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            //開発用UIの処理
            ImGui::ShowDemoWindow();

            ImGui::Begin("Settings");
            ImGui::ColorEdit4("material", &materialData->x, ImGuiColorEditFlags_AlphaPreview);
            ImGui::End();


            // ImGuiの内部コマンド
            ImGui::Render();

           


            // これから書き込むバックバファの禁書目録を取得
            UINT backBufferindex = swapChain->GetCurrentBackBufferIndex();

            // TransitionBarrierを作成する
            D3D12_RESOURCE_BARRIER barrier {};

            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;

            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

            barrier.Transition.pResource = swapChainResources[backBufferindex];

            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;

            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;



            commandList->ResourceBarrier(1, &barrier);

            

            // 　描画先のRTVを設定する
            commandList->OMSetRenderTargets(1, &rtvHandles[backBufferindex], false, nullptr);
            // 指定した色で画面全体をクリアする
            float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
            commandList->ClearRenderTargetView(rtvHandles[backBufferindex], clearColor, 0, nullptr);

            ID3D12DescriptorHeap* descriptorHeaps[] = { srvDescriptorHeap };
            commandList->SetDescriptorHeaps(1, descriptorHeaps);


            commandList->RSSetViewports(1, &viewport);
            commandList->RSSetScissorRects(1, &scissorRect);
            commandList->SetGraphicsRootSignature(rootSignature);
            commandList->SetPipelineState(graphicsPipelineState);
            commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            commandList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
            commandList->DrawInstanced(3, 1, 0, 0);

             ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList);

            barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
            barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

            commandList->ResourceBarrier(1, &barrier);

            // コマンドリストの内容を確定させる。全てのコマンドを積んでからCloseすること
            hr = commandList->Close();

            assert(SUCCEEDED(hr));
            // GPUにコマンドリストを実行させる
            ID3D12CommandList* commandLists[] = { commandList };
            commandQueue->ExecuteCommandLists(1, commandLists);
            // GPUとosに画面の交換を行うように通知する
            swapChain->Present(1, 0);
            fenceValue++;
            commandQueue->Signal(fence, fenceValue);

            if (fence->GetCompletedValue() < fenceValue) {
                fence->SetEventOnCompletion(fenceValue, fenceEvent);
                WaitForSingleObject(fenceEvent, INFINITE);
            }
            // 次のフレーム用のコマンドリストを準備
            hr = commandAllocator->Reset();
            assert(SUCCEEDED(hr));
            hr = commandList->Reset(commandAllocator, nullptr);
            assert(SUCCEEDED(hr));

            // ゲームの更新処理を行う
            // ここにゲームの更新処理を書く
        }
    }

    Log(ConvertString(std::format(L"-------------------------------WSTRING{}\n", L"abc")));

    // 解放処理
    CloseHandle(fenceEvent);

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    textureResource->Release();


    srvDescriptorHeap->Release();
    fence->Release();
    rtvDescriptorHeap->Release();
    swapChainResources[0]->Release();
    swapChainResources[1]->Release();
    swapChain->Release();
    commandList->Release();
    commandAllocator->Release();
    commandQueue->Release();
    device->Release();
    uesAdapter->Release();
    dxgiFactory->Release();
    vertexResoursc->Release();
    graphicsPipelineState->Release();
    signatureBlob->Release();
    if (errorBlob) {
        errorBlob->Release();
    }
    vertexShaderBlob->Release();
    pixelShaderBlob->Release();
    rootSignature->Release();
    materialResource->Release();

#ifdef _DEBUG

    debugController->Release();

#endif // DEBUG

    CloseWindow(hwnd);

    IDXGIDebug1* debug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
        debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
        debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
        debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
        debug->Release();
    }

    CoUninitialize();

    return 0;
}
