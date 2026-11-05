// End-to-end offline check of the stereo proxy. Renders a tiny scene the way a
// game would (shadow pass, depth-tested main pass into an offscreen target, a
// fullscreen composite onto the backbuffer, a flat UI quad) THROUGH the proxy,
// then reads both eye images back and measures the horizontal disparity of
// each object against the value the eye separation predicts.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

using CreateDeviceFn = HRESULT (WINAPI*)(
    IDXGIAdapter*, D3D_DRIVER_TYPE, HMODULE, UINT,
    const D3D_FEATURE_LEVEL*, UINT, UINT,
    ID3D11Device**, D3D_FEATURE_LEVEL*, ID3D11DeviceContext**);

// must match kTwinGuid in src/stereo.cpp
static const GUID kTwinGuid = { 0x7c0e5a10, 0x3f1b, 0x4e0c, { 0x9b, 0x61, 0xfc, 0x30, 0, 0, 0, 1 } };

static const UINT kWidth = 640;
static const UINT kHeight = 360;
static const float kSeparation = 0.032f;    // the mod's default
static const float kUiDepth = 2.0f;

struct Vertex { float x, y, z; float r, g, b, a; };

static int Fail(const char* what)
{
    std::fprintf(stderr, "FAIL: %s\n", what);
    return 1;
}

static ID3D11Texture2D* TwinOf(ID3D11Texture2D* texture)
{
    IUnknown* twin = nullptr;
    UINT size = sizeof(twin);
    if (SUCCEEDED(texture->GetPrivateData(kTwinGuid, &size, &twin)) && twin)
        return static_cast<ID3D11Texture2D*>(twin);
    return nullptr;
}

static bool ReadPixels(ID3D11Device* device, ID3D11DeviceContext* context, ID3D11Texture2D* texture,
                       std::vector<uint32_t>& pixels)
{
    D3D11_TEXTURE2D_DESC desc = {};
    texture->GetDesc(&desc);
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.MiscFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Texture2D* staging = nullptr;
    if (FAILED(device->CreateTexture2D(&desc, nullptr, &staging))) return false;
    context->CopyResource(staging, texture);
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    if (FAILED(context->Map(staging, 0, D3D11_MAP_READ, 0, &mapped)))
    {
        staging->Release();
        return false;
    }
    pixels.resize(static_cast<size_t>(desc.Width) * desc.Height);
    for (UINT y = 0; y < desc.Height; ++y)
        std::memcpy(&pixels[static_cast<size_t>(y) * desc.Width],
                    static_cast<const uint8_t*>(mapped.pData) + static_cast<size_t>(mapped.RowPitch) * y,
                    desc.Width * 4);
    context->Unmap(staging, 0);
    staging->Release();
    return true;
}

// kind: 0 red, 1 green, 2 white. Returns the mean x of matching pixels in
// columns [x0, x1), or -1 when there are none.
static double CentroidX(const std::vector<uint32_t>& pixels, int kind, UINT x0 = 0, UINT x1 = kWidth)
{
    double sum = 0.0;
    unsigned count = 0;
    for (UINT y = 0; y < kHeight; ++y)
    {
        for (UINT x = x0; x < x1; ++x)
        {
            const uint32_t p = pixels[static_cast<size_t>(y) * kWidth + x];
            const unsigned r = p & 0xff, g = (p >> 8) & 0xff, b = (p >> 16) & 0xff;
            bool match = false;
            if (kind == 0) match = r > 200 && g < 60;               // near quad
            if (kind == 1) match = g > 200 && r < 60;               // far quad
            if (kind == 2) match = r > 200 && g > 200 && b > 200;   // UI
            if (kind == 3) match = r > 200 && g < 60 && b > 200;    // near quad, marked band
            if (kind == 4) match = g > 200 && r < 60 && b > 200;    // far quad, marked band
            if (match)
            {
                sum += x + 0.5;
                ++count;
            }
        }
    }
    return count ? sum / count : -1.0;
}

// Bounding box of the pixels of one kind.
static bool Bounds(const std::vector<uint32_t>& pixels, int kind, double box[4])
{
    int minX = 1 << 30, minY = 1 << 30, maxX = -1, maxY = -1;
    for (UINT y = 0; y < kHeight; ++y)
        for (UINT x = 0; x < kWidth; ++x)
        {
            const uint32_t p = pixels[static_cast<size_t>(y) * kWidth + x];
            const unsigned r = p & 0xff, g = (p >> 8) & 0xff;
            const unsigned b = (p >> 16) & 0xff;
            const bool match = kind == 0 ? (r > 200 && g < 60) : kind == 1 ? (g > 200 && r < 60)
                                                                 : (r > 200 && g < 60 && b > 200);
            if (!match) continue;
            if (static_cast<int>(x) < minX) minX = x;
            if (static_cast<int>(x) > maxX) maxX = x;
            if (static_cast<int>(y) < minY) minY = y;
            if (static_cast<int>(y) > maxY) maxY = y;
        }
    if (maxX < 0) return false;
    box[0] = minX; box[1] = maxX + 1.0; box[2] = minY; box[3] = maxY + 1.0;
    return true;
}

// Where a quad's corners land for an eye turned 10 degrees left and 5 up,
// leaning, with an off-centre field of view (the mod's FC3_STEREO_TEST_VIEW).
static void ExpectedBounds(int eye, double x0, double y0, double x1, double y1, double z, double box[4])
{
    const double yaw = 10.0 * 0.0174532925, pitch = 5.0 * 0.0174532925;
    const double cy = std::cos(yaw), sy = std::sin(yaw), cp = std::cos(pitch), sp = std::sin(pitch);
    const double r[3][3] = { { cy, sy * sp, -sy * cp }, { 0, cp, sp }, { sy, -cy * sp, cy * cp } };
    const double side = eye == 0 ? -kSeparation : kSeparation;
    const double t[3] = { 0.10 + r[0][0] * side, 0.05 + r[1][0] * side, 0.20 + r[2][0] * side };
    const double tanL = -1.1, tanR = 0.9, tanU = 0.8, tanD = -1.0;
    box[0] = box[2] = 1e9; box[1] = box[3] = -1e9;
    const double corners[4][2] = { { x0, y0 }, { x1, y0 }, { x1, y1 }, { x0, y1 } };
    for (const double* corner : corners)
    {
        const double g[3] = { corner[0] - t[0], corner[1] - t[1], z - t[2] };
        const double v[3] = { r[0][0] * g[0] + r[1][0] * g[1] + r[2][0] * g[2],
                              r[0][1] * g[0] + r[1][1] * g[1] + r[2][1] * g[2],
                              r[0][2] * g[0] + r[1][2] * g[1] + r[2][2] * g[2] };
        const double nx = (2 * v[0] / v[2] - (tanR + tanL)) / (tanR - tanL);
        const double ny = (2 * v[1] / v[2] - (tanU + tanD)) / (tanU - tanD);
        const double px = (nx + 1) * 0.5 * kWidth, py = (1 - ny) * 0.5 * kHeight;
        if (px < box[0]) box[0] = px;
        if (px > box[1]) box[1] = px;
        if (py < box[2]) box[2] = py;
        if (py > box[3]) box[3] = py;
    }
}

int wmain(int argc, wchar_t** argv)
{
    if (argc != 2)
        return 2;
    HMODULE proxy = LoadLibraryW(argv[1]);
    if (!proxy)
    {
        std::fprintf(stderr, "FAIL: proxy load error %lu\n", GetLastError());
        return 4;
    }

    // every export the system d3d11.dll has must exist on the proxy
    wchar_t systemPath[MAX_PATH] = {};
    GetSystemDirectoryW(systemPath, MAX_PATH);
    lstrcatW(systemPath, L"\\d3d11.dll");
    HMODULE systemDll = LoadLibraryExW(systemPath, nullptr, DONT_RESOLVE_DLL_REFERENCES);
    int exportCount = 0;
    if (systemDll)
    {
        const BYTE* base = reinterpret_cast<const BYTE*>(systemDll);
        const IMAGE_DOS_HEADER* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        const IMAGE_NT_HEADERS* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        const IMAGE_DATA_DIRECTORY& dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
        const IMAGE_EXPORT_DIRECTORY* exports = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(base + dir.VirtualAddress);
        const DWORD* names = reinterpret_cast<const DWORD*>(base + exports->AddressOfNames);
        for (DWORD i = 0; i < exports->NumberOfNames; ++i)
        {
            const char* name = reinterpret_cast<const char*>(base + names[i]);
            ++exportCount;
            if (!GetProcAddress(proxy, name))
            {
                std::fprintf(stderr, "FAIL: proxy is missing export %s\n", name);
                return 5;
            }
        }
        FreeLibrary(systemDll);
    }
    if (exportCount == 0) return Fail("could not enumerate system d3d11 exports");

    const auto createDevice = reinterpret_cast<CreateDeviceFn>(GetProcAddress(proxy, "D3D11CreateDevice"));
    const D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_11_0;
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    if (FAILED(createDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, &level, 1,
                            D3D11_SDK_VERSION, &device, nullptr, &context)))
        return Fail("device creation");

    HWND window = CreateWindowExW(0, L"STATIC", L"FC3 stereo smoke", WS_OVERLAPPED, 0, 0, 64, 64,
                                  nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    IDXGIDevice* dxgiDevice = nullptr;
    IDXGIAdapter* adapter = nullptr;
    IDXGIFactory* factory = nullptr;
    IDXGISwapChain* swapChain = nullptr;
    device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDevice));
    dxgiDevice->GetAdapter(&adapter);
    adapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory));
    DXGI_SWAP_CHAIN_DESC chain = {};
    chain.BufferDesc.Width = kWidth;
    chain.BufferDesc.Height = kHeight;
    chain.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    chain.SampleDesc.Count = 1;
    chain.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    chain.BufferCount = 1;
    chain.OutputWindow = window;
    chain.Windowed = TRUE;
    chain.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    if (FAILED(factory->CreateSwapChain(device, &chain, &swapChain)) || !swapChain)
        return Fail("swapchain creation");

    // shaders
    static const char source[] =
        "cbuffer Cam : register(b1) { row_major float4x4 vrp; float4 pad[52]; };"
        "struct I { float3 p : POSITION; float4 c : COLOR; };"
        "struct V { float4 p : SV_Position; float4 c : COLOR; float2 uv : TEXCOORD0; };"
        "V vsScene(I i) { V o; o.p = mul(vrp, float4(i.p, 1)); o.c = i.c; o.uv = 0; return o; }"
        "V vsFlat(I i) { V o; o.p = float4(i.p, 1); o.c = i.c; o.uv = i.p.xy * float2(0.5, -0.5) + 0.5; return o; }"
        "row_major float4x4 InvViewRotProjectionMatrix;"
        "Texture2D t : register(t0); Texture2D<float> depthTex : register(t1); SamplerState s : register(s0);"
        "float4 psColor(V i) : SV_Target { return i.c; }"
        // Like the game's deferred passes: rebuild the position from depth and
        // mark a band of world space (|x| < 0.15). The band must stay centred
        // on the objects in BOTH eyes.
        "float4 psCopy(V i) : SV_Target { float4 c = t.Sample(s, i.uv);"
        " float d = depthTex.Load(int3(i.p.xy, 0));"
        " float4 clip = float4(i.uv.x * 2 - 1, 1 - i.uv.y * 2, d, 1);"
        " float4 p = mul(InvViewRotProjectionMatrix, clip); p.xyz /= p.w;"
        " if (d < 1 && abs(p.x) < 0.15) c.b = 1; return c; }";
    ID3DBlob* blobScene = nullptr; ID3DBlob* blobFlat = nullptr;
    ID3DBlob* blobColor = nullptr; ID3DBlob* blobCopy = nullptr;
    if (FAILED(D3DCompile(source, sizeof(source) - 1, nullptr, nullptr, nullptr, "vsScene", "vs_5_0", 0, 0, &blobScene, nullptr)) ||
        FAILED(D3DCompile(source, sizeof(source) - 1, nullptr, nullptr, nullptr, "vsFlat", "vs_5_0", 0, 0, &blobFlat, nullptr)) ||
        FAILED(D3DCompile(source, sizeof(source) - 1, nullptr, nullptr, nullptr, "psColor", "ps_5_0", 0, 0, &blobColor, nullptr)) ||
        FAILED(D3DCompile(source, sizeof(source) - 1, nullptr, nullptr, nullptr, "psCopy", "ps_5_0", 0, 0, &blobCopy, nullptr)))
        return Fail("shader compile");
    ID3D11VertexShader* vsScene = nullptr; ID3D11VertexShader* vsFlat = nullptr;
    ID3D11PixelShader* psColor = nullptr; ID3D11PixelShader* psCopy = nullptr;
    device->CreateVertexShader(blobScene->GetBufferPointer(), blobScene->GetBufferSize(), nullptr, &vsScene);
    device->CreateVertexShader(blobFlat->GetBufferPointer(), blobFlat->GetBufferSize(), nullptr, &vsFlat);
    device->CreatePixelShader(blobColor->GetBufferPointer(), blobColor->GetBufferSize(), nullptr, &psColor);
    device->CreatePixelShader(blobCopy->GetBufferPointer(), blobCopy->GetBufferSize(), nullptr, &psCopy);
    if (!vsScene || !vsFlat || !psColor || !psCopy) return Fail("shader creation");
    const D3D11_INPUT_ELEMENT_DESC elements[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };
    ID3D11InputLayout* layout = nullptr;
    device->CreateInputLayout(elements, 2, blobScene->GetBufferPointer(), blobScene->GetBufferSize(), &layout);
    if (!layout) return Fail("input layout");

    // geometry: [0..5] far green quad, [6..11] near red quad, [12..17] fullscreen, [18..23] UI
    auto quad = [](std::vector<Vertex>& v, float x0, float y0, float x1, float y1, float z, float r, float g, float b)
    {
        const Vertex a = { x0, y0, z, r, g, b, 1 }, bb = { x1, y0, z, r, g, b, 1 };
        const Vertex c = { x1, y1, z, r, g, b, 1 }, d = { x0, y1, z, r, g, b, 1 };
        v.push_back(a); v.push_back(d); v.push_back(c);
        v.push_back(a); v.push_back(c); v.push_back(bb);
    };
    std::vector<Vertex> vertices;
    quad(vertices, -3.0f, 1.0f, 3.0f, 3.0f, 10.0f, 0, 1, 0);
    quad(vertices, -0.3f, -0.8f, 0.3f, -0.2f, 2.0f, 1, 0, 0);
    quad(vertices, -1.0f, -1.0f, 1.0f, 1.0f, 0.5f, 1, 1, 1);
    quad(vertices, -0.9f, 0.7f, -0.7f, 0.9f, 0.5f, 1, 1, 1);
    D3D11_BUFFER_DESC vbDesc = {};
    vbDesc.ByteWidth = static_cast<UINT>(vertices.size() * sizeof(Vertex));
    vbDesc.Usage = D3D11_USAGE_DEFAULT;
    vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vbData = { vertices.data(), 0, 0 };
    ID3D11Buffer* vertexBuffer = nullptr;
    device->CreateBuffer(&vbDesc, &vbData, &vertexBuffer);

    D3D11_BUFFER_DESC camDesc = {};
    camDesc.ByteWidth = 896;
    camDesc.Usage = D3D11_USAGE_DYNAMIC;
    camDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    camDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    ID3D11Buffer* cameraBuffer = nullptr;
    device->CreateBuffer(&camDesc, nullptr, &cameraBuffer);
    if (!vertexBuffer || !cameraBuffer) return Fail("buffers");

    // targets
    D3D11_TEXTURE2D_DESC sceneDesc = {};
    sceneDesc.Width = kWidth; sceneDesc.Height = kHeight; sceneDesc.MipLevels = 1; sceneDesc.ArraySize = 1;
    sceneDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; sceneDesc.SampleDesc.Count = 1;
    sceneDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    ID3D11Texture2D* scene = nullptr; ID3D11RenderTargetView* sceneRtv = nullptr; ID3D11ShaderResourceView* sceneSrv = nullptr;
    device->CreateTexture2D(&sceneDesc, nullptr, &scene);
    device->CreateRenderTargetView(scene, nullptr, &sceneRtv);
    device->CreateShaderResourceView(scene, nullptr, &sceneSrv);
    D3D11_TEXTURE2D_DESC depthDesc = sceneDesc;
    depthDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
    ID3D11Texture2D* depth = nullptr; ID3D11DepthStencilView* depthDsv = nullptr;
    ID3D11ShaderResourceView* depthSrv = nullptr;
    device->CreateTexture2D(&depthDesc, nullptr, &depth);
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    device->CreateDepthStencilView(depth, &dsvDesc, &depthDsv);
    D3D11_SHADER_RESOURCE_VIEW_DESC depthSrvDesc = {};
    depthSrvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    depthSrvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    depthSrvDesc.Texture2D.MipLevels = 1;
    device->CreateShaderResourceView(depth, &depthSrvDesc, &depthSrv);
    // $Globals for the composite pixel shader: the inverse of the camera matrix
    const float inverse[16] = {
        1.0f, 0, 0, 0,
        0, static_cast<float>(kHeight) / kWidth, 0, 0,
        0, 0, 0, 1,
        0, 0, 1.0f / -0.1f, 1.001f / 0.1f };
    D3D11_BUFFER_DESC globalsDesc = {};
    globalsDesc.ByteWidth = 64;
    globalsDesc.Usage = D3D11_USAGE_DEFAULT;
    globalsDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    D3D11_SUBRESOURCE_DATA globalsData = { inverse, 0, 0 };
    ID3D11Buffer* globalsBuffer = nullptr;
    device->CreateBuffer(&globalsDesc, &globalsData, &globalsBuffer);
    if (!depthSrv || !globalsBuffer) return Fail("depth view / globals");
    D3D11_TEXTURE2D_DESC shadowDesc = depthDesc;
    shadowDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    shadowDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    shadowDesc.Width = 256; shadowDesc.Height = 256;
    ID3D11Texture2D* shadow = nullptr; ID3D11DepthStencilView* shadowDsv = nullptr;
    device->CreateTexture2D(&shadowDesc, nullptr, &shadow);
    device->CreateDepthStencilView(shadow, nullptr, &shadowDsv);
    ID3D11Texture2D* backbuffer = nullptr; ID3D11RenderTargetView* backbufferRtv = nullptr;
    swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backbuffer));
    device->CreateRenderTargetView(backbuffer, nullptr, &backbufferRtv);
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = samplerDesc.AddressV = samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    ID3D11SamplerState* sampler = nullptr;
    device->CreateSamplerState(&samplerDesc, &sampler);
    D3D11_RASTERIZER_DESC rasterDesc = {};
    rasterDesc.FillMode = D3D11_FILL_SOLID; rasterDesc.CullMode = D3D11_CULL_NONE; rasterDesc.DepthClipEnable = TRUE;
    ID3D11RasterizerState* raster = nullptr;
    device->CreateRasterizerState(&rasterDesc, &raster);
    if (!sceneRtv || !sceneSrv || !depthDsv || !shadowDsv || !backbufferRtv || !sampler || !raster)
        return Fail("targets");

    const float xScale = 1.0f;
    const float yScale = xScale * kWidth / kHeight;
    std::vector<uint32_t> left, right, presented;
    bool measured = false;
    for (int frame = 0; frame < 6; ++frame)
    {
        D3D11_MAPPED_SUBRESOURCE mapped = {};
        if (FAILED(context->Map(cameraBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return Fail("camera map");
        std::memset(mapped.pData, 0, 896);
        const float matrix[16] = {
            xScale, 0, 0, 0,
            0, yScale, 0, 0,
            0, 0, 1.001f, -0.1f,
            0, 0, 1, 0 };
        std::memcpy(mapped.pData, matrix, sizeof(matrix));
        context->Unmap(cameraBuffer, 0);

        const UINT stride = sizeof(Vertex), offset = 0;
        context->IASetInputLayout(layout);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        context->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
        context->RSSetState(raster);
        context->VSSetConstantBuffers(1, 1, &cameraBuffer);
        context->PSSetSamplers(0, 1, &sampler);

        // shadow-style pass: small square depth target, must stay single
        D3D11_VIEWPORT viewport = { 0, 0, 256, 256, 0, 1 };
        context->RSSetViewports(1, &viewport);
        context->ClearDepthStencilView(shadowDsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
        context->OMSetRenderTargets(0, nullptr, shadowDsv);
        context->VSSetShader(vsScene, nullptr, 0);
        context->PSSetShader(nullptr, nullptr, 0);
        context->Draw(12, 0);

        // main view
        const float black[4] = { 0, 0, 0, 1 };
        viewport.Width = static_cast<float>(kWidth);
        viewport.Height = static_cast<float>(kHeight);
        context->RSSetViewports(1, &viewport);
        context->ClearRenderTargetView(sceneRtv, black);
        context->ClearDepthStencilView(depthDsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
        context->OMSetRenderTargets(1, &sceneRtv, depthDsv);
        context->PSSetShader(psColor, nullptr, 0);
        context->Draw(12, 0);

        // composite onto the backbuffer
        context->OMSetRenderTargets(1, &backbufferRtv, nullptr);
        context->VSSetShader(vsFlat, nullptr, 0);
        context->PSSetShader(psCopy, nullptr, 0);
        ID3D11ShaderResourceView* inputs[2] = { sceneSrv, depthSrv };
        context->PSSetShaderResources(0, 2, inputs);
        context->PSSetConstantBuffers(0, 1, &globalsBuffer);
        context->Draw(6, 12);
        ID3D11ShaderResourceView* none[2] = { nullptr, nullptr };
        context->PSSetShaderResources(0, 2, none);

        // flat UI straight onto the backbuffer
        context->PSSetShader(psColor, nullptr, 0);
        context->Draw(6, 18);

        if (frame == 4)
        {
            ID3D11Texture2D* twin = TwinOf(backbuffer);
            if (!twin) return Fail("backbuffer has no right-eye image");
            const bool ok = ReadPixels(device, context, backbuffer, left) && ReadPixels(device, context, twin, right);
            twin->Release();
            if (!ok) return Fail("eye readback");
            measured = true;
        }
        if (FAILED(swapChain->Present(0, 0))) return Fail("present");
        if (frame == 4)
            ReadPixels(device, context, backbuffer, presented);
    }
    if (!measured) return Fail("never measured");

    if (GetEnvironmentVariableW(L"FC3_STEREO_TEST_VIEW", nullptr, 0) > 0)
    {
        // Head turned, leaning, off-centre field of view: every object must land
        // exactly where that eye would see it, and rebuilt positions must still
        // sit on the objects.
        const std::vector<uint32_t>* images[2] = { &left, &right };
        bool ok = true;
        for (int eye = 0; eye < 2; ++eye)
        {
            const double quads[2][5] = { { -0.3, -0.8, 0.3, -0.2, 2.0 }, { -3.0, 1.0, 3.0, 3.0, 10.0 } };
            for (int kind = 0; kind < 2; ++kind)
            {
                double seen[4], want[4];
                ExpectedBounds(eye, quads[kind][0], quads[kind][1], quads[kind][2], quads[kind][3], quads[kind][4], want);
                if (!Bounds(*images[eye], kind, seen)) { std::printf("object missing\n"); ok = false; continue; }
                double worst = 0;
                for (int i = 0; i < 4; ++i)
                    if (std::fabs(seen[i] - want[i]) > worst) worst = std::fabs(seen[i] - want[i]);
                std::printf("%s eye, %s object: edges at x %.0f..%.0f y %.0f..%.0f, predicted x %.1f..%.1f y %.1f..%.1f (worst %.2f px)\n",
                            eye ? "right" : "left", kind ? "far" : "near", seen[0], seen[1], seen[2], seen[3],
                            want[0], want[1], want[2], want[3], worst);
                if (worst > 1.6) ok = false;
            }
            // The band painted from rebuilt positions is the strip |x| < 0.15 of
            // the near object; it must cover exactly that strip in this eye too.
            double seenBand[4], wantBand[4];
            ExpectedBounds(eye, -0.15, -0.8, 0.15, -0.2, 2.0, wantBand);
            if (!Bounds(*images[eye], 2, seenBand)) { std::printf("rebuilt-position band missing\n"); ok = false; continue; }
            const double bandError = std::fabs(seenBand[0] - wantBand[0]) > std::fabs(seenBand[1] - wantBand[1])
                ? std::fabs(seenBand[0] - wantBand[0]) : std::fabs(seenBand[1] - wantBand[1]);
            std::printf("%s eye: rebuilt-position band at x %.0f..%.0f, predicted %.1f..%.1f (worst %.2f px)\n",
                        eye ? "right" : "left", seenBand[0], seenBand[1], wantBand[0], wantBand[1], bandError);
            if (bandError > 1.6) ok = false;
        }
        std::printf(ok ? "PASS: turned, leaning, off-centre eye cameras land every object where predicted.\n"
                       : "FAIL: eye camera placement is wrong.\n");
        return ok ? 0 : 1;
    }

    // expected disparities in pixels (left minus right)
    const float clipShift = kSeparation * xScale;
    const double expectNear = 2.0 * clipShift / 2.0f * (kWidth * 0.5);
    const double expectFar = 2.0 * clipShift / 10.0f * (kWidth * 0.5);
    const double expectUi = 2.0 * clipShift / kUiDepth * (kWidth * 0.5);
    const double nearGap = CentroidX(left, 0) - CentroidX(right, 0);
    const double farGap = CentroidX(left, 1) - CentroidX(right, 1);
    const double ui = CentroidX(left, 2) - CentroidX(right, 2);
    std::printf("disparity px (left - right): near %.2f (expect %.2f), far %.2f (expect %.2f), ui %.2f (expect %.2f)\n",
                nearGap, expectNear, farGap, expectFar, ui, expectUi);
    if (CentroidX(left, 0) < 0 || CentroidX(right, 0) < 0 || CentroidX(left, 1) < 0 ||
        CentroidX(right, 1) < 0 || CentroidX(left, 2) < 0 || CentroidX(right, 2) < 0)
        return Fail("an object is missing from one eye");
    if (std::fabs(nearGap - expectNear) > 1.01 || std::fabs(farGap - expectFar) > 1.01 || std::fabs(ui - expectUi) > 1.01)
        return Fail("disparity does not match the eye separation");

    // A band painted from rebuilt positions must sit on the middle of each
    // object in both eyes. Uncorrected, it drifts by the object's parallax.
    const std::vector<uint32_t>* eyes[2] = { &left, &right };
    for (int eye = 0; eye < 2; ++eye)
    {
        const double nearDrift = CentroidX(*eyes[eye], 3) - CentroidX(*eyes[eye], 0);
        const double farDrift = CentroidX(*eyes[eye], 4) - CentroidX(*eyes[eye], 1);
        std::printf("%s eye: rebuilt-position band off-centre by %.2f px (near), %.2f px (far)\n",
                    eye ? "right" : "left", nearDrift, farDrift);
        if (CentroidX(*eyes[eye], 3) < 0 || CentroidX(*eyes[eye], 4) < 0)
            return Fail("rebuilt-position band missing");
        if (std::fabs(nearDrift) > 0.8 || std::fabs(farDrift) > 0.8)
            return Fail("positions rebuilt from depth are not corrected per eye");
    }

    ID3D11Texture2D* sceneTwin = TwinOf(scene);
    ID3D11Texture2D* depthTwin = TwinOf(depth);
    ID3D11Texture2D* shadowTwin = TwinOf(shadow);
    std::printf("duplicates: scene=%s depth=%s shadow=%s\n", sceneTwin ? "yes" : "no",
                depthTwin ? "yes" : "no", shadowTwin ? "yes" : "no");
    if (!sceneTwin || !depthTwin) return Fail("main view targets were not duplicated");
    if (shadowTwin) return Fail("shadow target was duplicated");
    sceneTwin->Release();
    depthTwin->Release();

    // [SBSRESTORE] the side-by-side image is shown by Present; afterwards the
    // game's single buffer must hold the left eye again, or menus that draw
    // over the old contents feed the squeezed image back in (2026-10-10).
    if (!presented.empty())
    {
        const double after = CentroidX(presented, 0);
        const double want = CentroidX(left, 0);
        std::printf("after Present: near object at %.1f (left eye %.1f)\n", after, want);
        if (std::fabs(after - want) > 1.5)
            return Fail("side-by-side image left in the game's buffer after Present");
    }

    std::printf("PASS: %d exports; both eyes rendered in one frame; disparity matches; shadow target left single.\n",
                exportCount);
    return 0;
}
