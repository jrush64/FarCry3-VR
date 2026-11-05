#include "../src/stereo.cpp"
#include <cstdlib>

void Check(bool ok, const char* message)
{
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
void Palette(float* bones, bool rootOnly, float bodyShift = 0, float pumpShift = 0)
{
    std::memset(bones, 0, 48 * 12 * sizeof(float));
    for (int b = rootOnly ? 0 : 1; b <= (rootOnly ? 0 : 3); ++b)
    {
        bones[b * 12] = bones[b * 12 + 5] = bones[b * 12 + 10] = 1;
        bones[b * 12 + 3] = b * 0.1f + bodyShift + (b == 3 ? pumpShift : 0);
    }
}
int main()
{
    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    wchar_t runtimePath[MAX_PATH];
    GetSystemDirectoryW(runtimePath, MAX_PATH);
    wcscat_s(runtimePath, L"\\d3d11.dll");
    HMODULE runtime = LoadLibraryW(runtimePath);
    auto createDevice = reinterpret_cast<decltype(&D3D11CreateDevice)>(GetProcAddress(runtime, "D3D11CreateDevice"));
    Check(createDevice && SUCCEEDED(createDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0,
        nullptr, 0, D3D11_SDK_VERSION, &device, nullptr, &context)), "WARP device");
    ID3D11Buffer* buffers[80] = {};
    Mat4 view = {}; view.m[0] = view.m[5] = view.m[10] = view.m[15] = 1;
    float bones[48 * 12];
    UINT stride = 40, offset = 0;
    g_frame = 1000;
    g_lastShotFrame = -100000;
    for (int i = 0; i < 80; ++i)
    {
        D3D11_BUFFER_DESC desc = {};
        desc.ByteWidth = 1024 + i * 16;
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        Check(SUCCEEDED(device->CreateBuffer(&desc, nullptr, &buffers[i])), "vertex buffer");
        context->IASetVertexBuffers(0, 1, &buffers[i], &stride, &offset);
        Palette(bones, false);
        DriveForend(context, bones, view, 0, 100);
    }
    Palette(bones, true);
    DriveForend(context, bones, view, 100, 10);
    ForendRecord* record = nullptr;
    UINT size = sizeof(record);
    Check(SUCCEEDED(buffers[79]->GetPrivateData(kForendStateGuid, &size, &record)), "80th mesh has a learner");
    Check(record->state.restUsable[1] && record->state.restUsable[2] && record->state.restUsable[3],
        "partial palette preserves rest bones from other pieces");
    // The root-only piece never supplies bone 0 together with the others.
    // That unobserved pair must not be mistaken for a rigid body reference.
    g_lastShotFrame = 1000;
    g_frame = 1001;
    Palette(bones, false, 0.2f, -0.1f);
    DriveForend(context, bones, view, 0, 100);
    g_frame = 1045;
    g_twoHand = true;
    g_slideGrab = 0.1;
    g_slide = 0.06;
    Palette(bones, false, 0.2f, -0.1f);
    DriveForend(context, bones, view, 0, 100);
    ForendRecord* pump = nullptr; size = sizeof(pump);
    Check(SUCCEEDED(buffers[79]->GetPrivateData(kForendStateGuid, &size, &pump)), "measured mesh record");
    Check(pump->state.learned && pump->state.bone == 3 && pump->state.reference == 1,
        "pump identified relative to rigid body after 78 earlier meshes");
    Check(std::fabs(bones[3 * 12 + 3] - 0.46f) < 0.0001f, "pump follows 4 cm hand pull instead of game animation");
    pump->Release();
    context->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    context->ClearState();
    for (auto* buffer : buffers) buffer->Release();
    context->Flush();
    context->Release(); device->Release();
    Check(record->refs == 1, "buffer destruction released owned learner");
    record->Release();
    std::puts("PASS: 80 meshes, partial palettes, body/pump identification, hand-driven slide, resource lifetime.");
}
