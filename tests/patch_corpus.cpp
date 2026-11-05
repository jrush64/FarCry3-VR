// Offline check of the vertex shader patcher against every shader the game
// ships. Input: a file of [u32 size][DXBC blob] records unpacked from
// shadersobj.dat. Each vertex shader is patched and created on a WARP device.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <cstdio>
#include <cstring>
#include <vector>

#include "../src/dxbc_patch.h"

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::printf("usage: patch_corpus <all.bin> [show]\n");
        return 2;
    }
    const int show = argc >= 3 ? std::atoi(argv[2]) : 1;

    FILE* file = nullptr;
    if (fopen_s(&file, argv[1], "rb") != 0 || !file)
    {
        std::printf("cannot open corpus\n");
        return 2;
    }

    ID3D11Device* device = nullptr;
    ID3D11DeviceContext* context = nullptr;
    const D3D_FEATURE_LEVEL level = D3D_FEATURE_LEVEL_11_0;
    if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, &level, 1,
                                 D3D11_SDK_VERSION, &device, nullptr, &context)))
    {
        std::printf("no WARP device\n");
        return 2;
    }

    int total = 0, checksumOk = 0, vertex = 0, patched = 0, unsupported = 0;
    int armPatched = 0, armFailed = 0;
    int malformed = 0, createFailed = 0, originalCreateFailed = 0, shown = 0;
    int pixelPatched = 0, pixelBad = 0, pixelCreateFailed = 0, pixelShown = 0;
    std::vector<uint8_t> blob, out;
    for (;;)
    {
        uint32_t size = 0;
        if (std::fread(&size, 4, 1, file) != 1)
            break;
        blob.resize(size);
        if (std::fread(blob.data(), 1, size, file) != size)
            break;
        ++total;
        if (dxbc::VerifyChecksum(blob.data(), blob.size()))
            ++checksumOk;

        const dxbc::PixelResult pixel = dxbc::PatchPixelShader(blob.data(), blob.size(), out);
        if (pixel == dxbc::PixelResult::Unsupported || pixel == dxbc::PixelResult::Malformed)
            ++pixelBad;
        if (pixel == dxbc::PixelResult::Patched)
        {
            ++pixelPatched;
            ID3D11PixelShader* pixelShader = nullptr;
            if (FAILED(device->CreatePixelShader(out.data(), out.size(), nullptr, &pixelShader)))
                ++pixelCreateFailed;
            if (pixelShader) pixelShader->Release();
            if (pixelShown < show)
            {
                ++pixelShown;
                ID3DBlob* text = nullptr;
                if (SUCCEEDED(D3DDisassemble(out.data(), out.size(), 0, nullptr, &text)) && text)
                {
                    const char* code = std::strstr(static_cast<const char*>(text->GetBufferPointer()), "ps_5_0");
                    if (code) std::printf("---- patched pixel shader\n%.2600s\n", code);
                    text->Release();
                }
            }
        }

        const dxbc::Result result = dxbc::PatchVertexShader(blob.data(), blob.size(), out);
        if (result == dxbc::Result::NotVertexShader)
            continue;
        ++vertex;
        if (result == dxbc::Result::Unsupported) { ++unsupported; continue; }
        if (result == dxbc::Result::Malformed) { ++malformed; continue; }
        ++patched;

        ID3D11VertexShader* shader = nullptr;
        const HRESULT created = device->CreateVertexShader(out.data(), out.size(), nullptr, &shader);
        if (FAILED(created))
        {
            ++createFailed;
            ID3D11VertexShader* original = nullptr;
            if (FAILED(device->CreateVertexShader(blob.data(), blob.size(), nullptr, &original)))
                ++originalCreateFailed;
            if (original) original->Release();
            if (createFailed <= 3)
                std::printf("create failed hr=0x%08lX size=%u\n",
                            static_cast<unsigned long>(created), size);
        }
        if (shader) shader->Release();

        // The arm variant must also produce valid shaders.
        {
            std::vector<uint8_t> armOut;
            if (dxbc::PatchArmVertexShader(blob.data(), blob.size(), armOut) == dxbc::Result::Patched)
            {
                ++armPatched;
                ID3D11VertexShader* armShader = nullptr;
                if (FAILED(device->CreateVertexShader(armOut.data(), armOut.size(), nullptr, &armShader))) ++armFailed;
                if (armShader) armShader->Release();
                unsigned long long h = 14695981039346656037ull;
                for (uint8_t b : blob) { h ^= b; h *= 1099511628211ull; }
                if (h == 0xE8D9D3694E695620ull)
                {
                    ID3DBlob* text = nullptr;
                    if (SUCCEEDED(D3DDisassemble(armOut.data(), armOut.size(), 0, nullptr, &text)) && text)
                    {
                        const char* body = static_cast<const char*>(text->GetBufferPointer());
                        const char* tail = std::strstr(body, "mul o7.");
                        std::printf("---- arm shader found; patched tail:\n%s\n", tail ? tail : body);
                        text->Release();
                    }
                }
            }
        }

        if (shown < show)
        {
            ID3DBlob* text = nullptr;
            if (SUCCEEDED(D3DDisassemble(out.data(), out.size(), 0, nullptr, &text)) && text)
            {
                const char* begin = static_cast<const char*>(text->GetBufferPointer());
                const size_t length = text->GetBufferSize();
                // print the declarations and the last lines of code
                const char* code = std::strstr(begin, "vs_");
                if (code)
                {
                    const char* tail = begin + length;
                    int lines = 0;
                    while (tail > code && lines < 12)
                    {
                        --tail;
                        if (*tail == '\n') ++lines;
                    }
                    std::printf("---- patched shader %d (head)\n%.*s\n---- (tail)\n%s\n",
                                shown, 700, code, tail);
                }
                text->Release();
            }
            else
            {
                std::printf("disassemble failed for patched shader %d\n", shown);
            }
            ++shown;
        }
    }
    std::fclose(file);

    std::printf("blobs=%d checksum_ok=%d vertex=%d patched=%d unsupported=%d malformed=%d "
                "create_failed=%d original_create_failed=%d\n",
                total, checksumOk, vertex, patched, unsupported, malformed,
                createFailed, originalCreateFailed);
    std::printf("arm variant: patched=%d create_failed=%d\n", armPatched, armFailed);
    std::printf("pixel shaders: patched=%d rejected=%d create_failed=%d\n",
                pixelPatched, pixelBad, pixelCreateFailed);
    context->Release();
    device->Release();
    const bool pass = total > 0 && checksumOk == total && patched == vertex &&
                      createFailed == 0 && pixelBad == 0 && pixelCreateFailed == 0 && pixelPatched > 0;
    std::printf(pass ? "PASS\n" : "FAIL\n");
    return pass ? 0 : 1;
}
