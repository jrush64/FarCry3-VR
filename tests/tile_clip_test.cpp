// [TILES] end-to-end check of the lighting-tile fix (renderfix5), on WARP.
// The game's deferred lighting draws a grid of game-screen tiles; each tile's
// lamps were chosen for the GAME camera. The mod redraws every tile through the
// eye (tile_shader.h) and the real pixel patcher (dxbc_patch_pixel.cpp) adds a
// test that keeps only pixels whose surface point lands inside that tile on the
// game's screen. Scene: a slanted wall 0.5-0.8 m away and a far backdrop; the
// eye is turned 20 degrees and sits 12 cm from the game camera. Every pixel must
// be lit exactly once, by the tile the game camera assigns its point to.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3d11shader.h>
#include "../src/scene_grid258.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include "../src/dxbc_patch.h"
#include "../src/tile_shader.h"
#include "../src/shadow_readback.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")

static const char tileSource[] = FC3_TILE_SHADER_SOURCE;

// stands in for the game's lighting shader: uv -> clip -> InvViewRotProjectionMatrix
static const char lightSource[] =
    "#pragma pack_matrix(row_major)\n"
    "float4x4 InvViewRotProjectionMatrix; float4 TileId;\n"
    "Texture2D<float> D:register(t0);\n"
    "struct O{float4 a:SV_Target0;float4 b:SV_Target1;};\n"
    "O main(float4 pos:SV_Position,float4 t:TEXCOORD0){\n"
    " float z=D.Load(int3(pos.xy,0));\n"
    " float4 c=float4(t.xy*float2(2,-2)+float2(-1,1),z,1);\n"
    " float4 w=mul(InvViewRotProjectionMatrix,c);\n"
    " O o;o.a=float4(1,TileId.x,0,1);o.b=w;return o;}\n";

struct M4 { double m[16]; };
static M4 Mul(const M4& a, const M4& b)
{
    M4 r = {};
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            for (int k = 0; k < 4; ++k) r.m[i * 4 + j] += a.m[i * 4 + k] * b.m[k * 4 + j];
    return r;
}
static bool Inverse(const M4& in, M4& out)
{
    double a[4][8];
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 8; ++j) a[i][j] = j < 4 ? in.m[i * 4 + j] : (j - 4 == i ? 1 : 0);
    for (int c = 0; c < 4; ++c)
    {
        int p = c;
        for (int r = c + 1; r < 4; ++r) if (std::fabs(a[r][c]) > std::fabs(a[p][c])) p = r;
        if (std::fabs(a[p][c]) < 1e-12) return false;
        for (int j = 0; j < 8; ++j) { double t = a[c][j]; a[c][j] = a[p][j]; a[p][j] = t; }
        const double d = a[c][c];
        for (int j = 0; j < 8; ++j) a[c][j] /= d;
        for (int r = 0; r < 4; ++r)
            if (r != c) { const double f = a[r][c]; for (int j = 0; j < 8; ++j) a[r][j] -= f * a[c][j]; }
    }
    for (int i = 0; i < 4; ++i) for (int j = 0; j < 4; ++j) out.m[i * 4 + j] = a[i][j + 4];
    return true;
}
static int Fail(const char* why) { std::printf("FAIL: %s\n", why); return 1; }
static ID3D11Buffer* MakeCb(ID3D11Device* dev, const void* data, UINT bytes)
{
    D3D11_BUFFER_DESC bd = { (bytes + 15) & ~15u, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
    std::vector<unsigned char> padded(bd.ByteWidth, 0);
    std::memcpy(padded.data(), data, bytes);
    D3D11_SUBRESOURCE_DATA sd = { padded.data(), 0, 0 };
    ID3D11Buffer* b = nullptr; dev->CreateBuffer(&bd, &sd, &b);
    return b;
}

// tile_clip_test.exe <in.bin> <out.bin>: patch one captured pixel shader (diagnosis)
static int PatchFile(const char* in, const char* out)
{
    FILE* f = nullptr; if (fopen_s(&f, in, "rb") || !f) return Fail("open input");
    std::vector<unsigned char> bytes; int ch; while ((ch = fgetc(f)) != EOF) bytes.push_back((unsigned char)ch); fclose(f);
    std::vector<uint8_t> patched; bool tile = false;
    const auto r = dxbc::PatchPixelShader(bytes.data(), bytes.size(), patched, &tile, true);
    std::printf("patch result %d tile test %d (%zu -> %zu bytes)\n", (int)r, tile ? 1 : 0, bytes.size(), patched.size());
    if (fopen_s(&f, out, "wb") || !f) return Fail("open output");
    fwrite(patched.data(), 1, patched.size(), f); fclose(f);
    return 0;
}

// tile_clip_test.exe --real <captured game lighting shader .bin>
// Runs the GAME's own lighting shader, patched, on WARP: constant view depth
// (cb1[26] / cb1[36] chosen so the shader rebuilds eye NDC z for 2 m), the real
// tile layout, the eye turned and offset. Occlusion queries count the samples
// that survive per light group; each group must keep exactly the pixels whose
// point the game camera puts in its cells.
static int RealShader(const char* path)
{
    FILE* f = nullptr; if (fopen_s(&f, path, "rb") || !f) return Fail("open shader");
    std::vector<unsigned char> bytes; int ch; while ((ch = fgetc(f)) != EOF) bytes.push_back((unsigned char)ch); fclose(f);
    std::vector<uint8_t> patched; bool tileTest = false;
    if (dxbc::PatchPixelShader(bytes.data(), bytes.size(), patched, &tileTest, true) != dxbc::PixelResult::Patched || !tileTest)
        return Fail("real shader: patch or tile test missing");
    ID3D11Device* dev = nullptr; ID3D11DeviceContext* ctx = nullptr;
    if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &dev, nullptr, &ctx))) return Fail("WARP");
    ID3DBlob *vsb = nullptr, *gsb = nullptr, *err = nullptr;
    D3DCompile(tileSource, sizeof(tileSource) - 1, nullptr, nullptr, nullptr, "main", "vs_4_0", 0, 0, &vsb, &err);
    D3DCompile(tileSource, sizeof(tileSource) - 1, nullptr, nullptr, nullptr, "gs", "gs_4_0", 0, 0, &gsb, &err);
    ID3D11VertexShader* vs = nullptr; ID3D11GeometryShader* gs = nullptr; ID3D11PixelShader* ps = nullptr;
    dev->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, &vs);
    dev->CreateGeometryShader(gsb->GetBufferPointer(), gsb->GetBufferSize(), nullptr, &gs);
    if (FAILED(dev->CreatePixelShader(patched.data(), patched.size(), nullptr, &ps)) || !ps) return Fail("real patched shader rejected by the runtime");
    D3D11_INPUT_ELEMENT_DESC el = { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
    ID3D11InputLayout* layout = nullptr;
    if (FAILED(dev->CreateInputLayout(&el, 1, vsb->GetBufferPointer(), vsb->GetBufferSize(), &layout))) return Fail("layout");

    // real tile layout, stride 16 like the game's
    const int gx = 30, gy = 17, groups = 9;
    std::vector<float> verts; std::vector<int> owner(gx * gy, -1); std::vector<UINT> start, count;
    {
        unsigned seed = 777;
        auto rnd = [&](int n) { seed = seed * 1103515245u + 12345u; return (int)((seed >> 16) % (unsigned)n); };
        std::vector<std::vector<float>> gv(groups);
        for (int y = 0; y < gy; ++y)
            for (int x = 0; x < gx;)
            {
                const int x1i = (std::min)(gx, x + 1 + rnd(8)); const int g = rnd(groups);
                const float x0 = -1 + 2.0f * x / gx, x1 = -1 + 2.0f * x1i / gx, y0 = -1 + 2.0f * y / gy, y1 = -1 + 2.0f * (y + 1) / gy;
                const float q[24] = { x0,y0,0,1, x0,y1,0,1, x1,y0,0,1, x0,y1,0,1, x1,y1,0,1, x1,y0,0,1 };
                gv[g].insert(gv[g].end(), q, q + 24);
                for (int c = x; c < x1i; ++c) owner[y * gx + c] = g;
                x = x1i;
            }
        for (auto& v : gv) { start.push_back((UINT)(verts.size() / 4)); count.push_back((UINT)(v.size() / 4)); verts.insert(verts.end(), v.begin(), v.end()); }
    }
    D3D11_BUFFER_DESC bd = { (UINT)(verts.size() * 4), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
    D3D11_SUBRESOURCE_DATA sd = { verts.data(), 0, 0 };
    ID3D11Buffer* vb = nullptr; dev->CreateBuffer(&bd, &sd, &vb);

    // cameras
    const double xs = 0.368, ys = 0.368 * 1.795, a = 1.0, b = -0.1, zv = 2.0;
    M4 invGame = {}; invGame.m[0] = 1 / xs; invGame.m[5] = 1 / ys; invGame.m[11] = 1; invGame.m[14] = 1 / b; invGame.m[15] = -a / b;
    const double yaw = 20 * 0.0174533, cy = std::cos(yaw), sy = std::sin(yaw);
    M4 view = {}; view.m[0] = cy; view.m[2] = -sy; view.m[5] = 1; view.m[8] = sy; view.m[10] = cy; view.m[15] = 1;
    view.m[3] = 0.11; view.m[7] = 0.05; view.m[11] = 0.03;
    const double tl = -1.15, tr = 0.95, tu = 1.2, td = -1.15;
    M4 eye = {}; eye.m[0] = 2 / (tr - tl); eye.m[2] = -(tr + tl) / (tr - tl); eye.m[5] = 2 / (tu - td); eye.m[6] = -(tu + td) / (tu - td);
    eye.m[10] = 1; eye.m[11] = -0.1; eye.m[14] = 1;
    const M4 world = Mul(eye, Mul(view, invGame)); M4 X = {}; Inverse(world, X);
    float t[12] = {};
    { const int rows[3] = { 0, 1, 3 }; for (int r = 0; r < 3; ++r) { const double* m = &world.m[rows[r] * 4]; t[r*4] = (float)(m[0]*30); t[r*4+1] = (float)(m[1]*30); t[r*4+2] = (float)(m[2]*(a*30+b) + m[3]*30); }
      const double tt = std::sqrt(0.11*0.11 + 0.05*0.05 + 0.03*0.03); const double ang = std::fmin(0.6, tt / 0.35); t[3] = (float)(0.02 + 1.6*ang*xs); t[7] = (float)(0.02 + 1.6*ang*ys); }
    float pcb[20] = {}; for (int c = 0; c < 4; ++c) for (int r = 0; r < 4; ++r) pcb[c*4+r] = (float)(X.m[r*4+c] - (r == c ? 1 : 0)); pcb[16] = 1;
    float g0[32] = {}; g0[28] = 1; g0[29] = -1;                       // VS $Globals: QuadParams
    std::vector<float> ps0(81 * 4, 0.0f); ps0[0] = ps0[5] = ps0[10] = ps0[15] = 1;   // PS $Globals: identity matrix
    std::vector<float> ps1(56 * 4, 0.0f); ps1[26*4+0] = 2.0f;           // depth 0 -> L = 0.5 (< 0.99, kept)
    ps1[36*4+0] = 0.05f; ps1[36*4+1] = 1; ps1[36*4+2] = 2; ps1[36*4+3] = 1;   // r0.z = 1 - 0.1/2 = eye NDC z at 2 m
    std::vector<float> ps2(8 * 4, 0.0f);
    ID3D11Buffer* cbG = MakeCb(dev, g0, sizeof(g0)); ID3D11Buffer* cbT = MakeCb(dev, t, sizeof(t)); ID3D11Buffer* cbP = MakeCb(dev, pcb, sizeof(pcb));
    ID3D11Buffer* c0 = MakeCb(dev, ps0.data(), (UINT)ps0.size() * 4); ID3D11Buffer* c1 = MakeCb(dev, ps1.data(), (UINT)ps1.size() * 4); ID3D11Buffer* c2 = MakeCb(dev, ps2.data(), (UINT)ps2.size() * 4);

    // expected pixels per group: eye ray at view depth 2 m -> game NDC -> cell owner
    const UINT W = 192, H = 192;
    std::vector<int> expectCount(groups, 0); int ambiguous = 0;
    for (UINT y = 0; y < H; ++y) for (UINT x = 0; x < W; ++x)
    {
        const double nx = (x + 0.5) / W * 2 - 1, ny = 1 - (y + 0.5) / H * 2;
        const double pe[3] = { (tl + (nx + 1) * 0.5 * (tr - tl)) * zv, (td + (ny + 1) * 0.5 * (tu - td)) * zv, zv };
        const double T3[3] = { view.m[3], view.m[7], view.m[11] }; double pg[3] = {};
        for (int i = 0; i < 3; ++i) for (int k = 0; k < 3; ++k) pg[i] += view.m[k*4+i] * (pe[k] - T3[k]);
        const double gnx = xs * pg[0] / pg[2], gny = ys * pg[1] / pg[2];
        const double fx = (gnx + 1) * 0.5 * gx, fy = (-gny + 1) * 0.5 * gy;
        if (std::fabs(fx - std::round(fx)) < 0.02 || std::fabs(fy - std::round(fy)) < 0.02) { ++ambiguous; continue; }
        int tx = (int)std::floor(fx), ty = (int)std::floor(fy); tx = tx < 0 ? 0 : tx >= gx ? gx - 1 : tx; ty = ty < 0 ? 0 : ty >= gy ? gy - 1 : ty;
        ++expectCount[owner[ty * gx + tx]];
    }
    D3D11_TEXTURE2D_DESC td2 = { W, H, 1, 1, DXGI_FORMAT_R16G16B16A16_FLOAT, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_RENDER_TARGET, 0, 0 };
    ID3D11Texture2D* rt = nullptr; ID3D11RenderTargetView* rtv = nullptr; dev->CreateTexture2D(&td2, nullptr, &rt); dev->CreateRenderTargetView(rt, nullptr, &rtv);
    D3D11_RASTERIZER_DESC rs = {}; rs.FillMode = D3D11_FILL_SOLID; rs.CullMode = D3D11_CULL_NONE; rs.DepthClipEnable = TRUE;
    ID3D11RasterizerState* raster = nullptr; dev->CreateRasterizerState(&rs, &raster);
    ctx->OMSetRenderTargets(1, &rtv, nullptr); ctx->RSSetState(raster);
    D3D11_VIEWPORT vp = { 0, 0, (float)W, (float)H, 0, 1 }; ctx->RSSetViewports(1, &vp);
    UINT stride = 16, offset = 0; ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset); ctx->IASetInputLayout(layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(vs, nullptr, 0); ctx->GSSetShader(gs, nullptr, 0); ctx->PSSetShader(ps, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &cbG); ctx->GSSetConstantBuffers(12, 1, &cbT);
    ID3D11Buffer* pcbs[3] = { c0, c1, c2 }; ctx->PSSetConstantBuffers(0, 3, pcbs); ctx->PSSetConstantBuffers(13, 1, &cbP);
    int total = 0, totalWant = 0, worst = 0;
    for (int g = 0; g < groups; ++g)
    {
        D3D11_QUERY_DESC qd = { D3D11_QUERY_OCCLUSION, 0 }; ID3D11Query* q = nullptr; dev->CreateQuery(&qd, &q);
        ctx->Begin(q); ctx->Draw(count[g], start[g]); ctx->End(q);
        UINT64 passed = 0; while (ctx->GetData(q, &passed, sizeof(passed), 0) == S_FALSE) {}
        q->Release();
        const int diff = (int)passed - expectCount[g];
        std::printf("  group %d: samples kept %llu, expected %d (+/- boundary)\n", g, passed, expectCount[g]);
        total += (int)passed; totalWant += expectCount[g]; if (std::abs(diff) > worst) worst = std::abs(diff);
    }
    std::printf("REAL game shader %s: kept %d samples, expected %d (+%d on cell boundaries), worst group off by %d\n", path, total, totalWant, ambiguous, worst);
    if (total < totalWant || total > totalWant + ambiguous + 16 || worst > ambiguous) return Fail("real shader: tile test keeps the wrong pixels");   // +16: shared seam pixels
    std::printf("PASS: real game lighting shader keeps exactly its own pixels per light group\n");
    return 0;
}

#include "shadow_readback_test.inl"
#include "screen_outputs_test.inl"
#include "particle_screen_probe.inl"
#include "river_depth_test.inl"
#include "particle_depth254_test.inl"
#include "motion_blur_test.inl"

#include "scene_grid258_test.inl"
int main(int argc, char** argv)
{
    if(argc==2&&std::strcmp(argv[1],"--scene-grid258")==0)return SceneGridTest258();
    if(argc==4&&std::strcmp(argv[1],"--fog257")==0)return ParticleDepthTest254(argv[2],argv[3],true);
    if(argc==4&&std::strcmp(argv[1],"--cave256")==0){int a=RiverDepthTest(argv[2],argv[3],false,"F8F31518956C98DB");return a?a:RiverDepthTest(argv[2],argv[3],true,"F8F31518956C98DB");}
    if(argc==4&&std::strcmp(argv[1],"--water254")==0){for(const char* h:{"919B2333D357EBB2","FFD1D36205DC7757","22ECA6952F53DBD7","3E11D1C4732A23E3","6CEEF5F8E48054A5"}){char p[1024];sprintf_s(p,"%s_%s",argv[3],h);int a=RiverDepthTest(argv[2],p,false,h);if(a)return a;if(strcmp(h,"22ECA6952F53DBD7")){a=RiverDepthTest(argv[2],p,true,h);if(a)return a;}}return 0;}
    if(argc==4&&std::strcmp(argv[1],"--particle-depth")==0)return ParticleDepthTest254(argv[2],argv[3]);
    if(argc==4&&std::strcmp(argv[1],"--ocean-depth")==0){int a=RiverDepthTest(argv[2],argv[3],false,"919B2333D357EBB2");return a?a:RiverDepthTest(argv[2],argv[3],false,"FFD1D36205DC7757");}
    if(argc==4&&std::strcmp(argv[1],"--ocean-surface")==0){int a=RiverDepthTest(argv[2],argv[3],true,"919B2333D357EBB2");return a?a:RiverDepthTest(argv[2],argv[3],true,"FFD1D36205DC7757");}
    if (argc == 3 && std::strcmp(argv[1], "--motion-blur") == 0) return MotionBlurTest(argv[2]);
    if (argc == 4 && std::strcmp(argv[1], "--river-diffuse") == 0) {
        int a=RiverDepthTest(argv[2],argv[3],false,"B5850815B1E93395");return a?a:RiverDepthTest(argv[2],argv[3],false,"F9BD64B91671E5FE");
    }
    if (argc == 4 && std::strcmp(argv[1], "--river-depth") == 0) {
        int a=RiverDepthTest(argv[2],argv[3]);return a?a:RiverDepthTest(argv[2],argv[3],false,"A1F37111CC1926D5");
    }
    if (argc == 4 && std::strcmp(argv[1], "--river-reflection") == 0) {
        int a=RiverDepthTest(argv[2],argv[3],true);return a?a:RiverDepthTest(argv[2],argv[3],true,"A1F37111CC1926D5");
    }
    if (argc == 4 && std::strcmp(argv[1], "--particle-screen") == 0) return ParticleScreenProbe(argv[2],argv[3]);
    if (argc == 4 && std::strcmp(argv[1], "--screen-outputs") == 0) return ScreenOutputsTest(argv[2],argv[3]);
    if (argc == 2 && std::strcmp(argv[1], "--shadow-readback") == 0) return ShadowReadbackTest();
    if (argc == 3 && std::strcmp(argv[1], "--real") == 0) return RealShader(argv[2]);
    if (argc == 3) return PatchFile(argv[1], argv[2]);
    ID3D11Device* dev = nullptr; ID3D11DeviceContext* ctx = nullptr;
    if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &dev, nullptr, &ctx)))
        return Fail("WARP device");
    ID3DBlob *vsb = nullptr, *gsb = nullptr, *psb = nullptr, *err = nullptr;
    if (FAILED(D3DCompile(tileSource, sizeof(tileSource) - 1, nullptr, nullptr, nullptr, "main", "vs_4_0", 0, 0, &vsb, &err)))
    { if (err) std::printf("%s\n", (const char*)err->GetBufferPointer()); return Fail("tile vs compile"); }
    if (FAILED(D3DCompile(tileSource, sizeof(tileSource) - 1, nullptr, nullptr, nullptr, "gs", "gs_4_0", 0, 0, &gsb, &err)))
    { if (err) std::printf("%s\n", (const char*)err->GetBufferPointer()); return Fail("tile gs compile"); }
    if (FAILED(D3DCompile(lightSource, sizeof(lightSource) - 1, nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &psb, &err)))
    { if (err) std::printf("%s\n", (const char*)err->GetBufferPointer()); return Fail("light ps compile"); }
    std::vector<uint8_t> patchedPs;
    bool tileTest = false;
    if (dxbc::PatchPixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), patchedPs, &tileTest, true) != dxbc::PixelResult::Patched)
        return Fail("pixel patcher refused the stand-in lighting shader");
    if (!tileTest) return Fail("pixel patcher did not add the tile test");
    ID3D11VertexShader* vs = nullptr; ID3D11GeometryShader* gs = nullptr; ID3D11PixelShader* ps = nullptr;
    dev->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, &vs);
    dev->CreateGeometryShader(gsb->GetBufferPointer(), gsb->GetBufferSize(), nullptr, &gs);
    if (FAILED(dev->CreatePixelShader(patchedPs.data(), patchedPs.size(), nullptr, &ps)) || !vs || !gs)
        return Fail("creating the shaders (patched pixel shader rejected)");
    D3D11_INPUT_ELEMENT_DESC el = { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 };
    ID3D11InputLayout* layout = nullptr;
    dev->CreateInputLayout(&el, 1, vsb->GetBufferPointer(), vsb->GetBufferSize(), &layout);

    // game tiles as [TILEDIAG] captured them (10-10): a 30 x 17 grid of screen
    // cells; each draw (one light group) is a list of horizontal runs of cells,
    // two triangles per run: (x0,y0) (x0,y1) (x1,y0) / (x0,y1) (x1,y1) (x1,y0).
    const int gx = 30, gy = 17;
    std::vector<float> verts;
    std::vector<int> cellOwner(gx * gy, -1);
    std::vector<UINT> drawStart, drawCount;
    {
        unsigned seed = 12345;
        auto rnd = [&](int n) { seed = seed * 1103515245u + 12345u; return (int)((seed >> 16) % (unsigned)n); };
        // runs: walk every row, cut it into random-length runs, give each run to one of 9 groups
        std::vector<std::vector<float>> groups(9);
        for (int y = 0; y < gy; ++y)
            for (int x = 0; x < gx;)
            {
                const int len = 1 + rnd(8);
                const int x1i = x + len > gx ? gx : x + len;
                const int grp = rnd(9);
                const float x0 = -1 + 2.0f * x / gx, x1 = -1 + 2.0f * x1i / gx;
                const float y0 = -1 + 2.0f * y / gy, y1 = -1 + 2.0f * (y + 1) / gy;
                const float q[12] = { x0, y0, x0, y1, x1, y0, x0, y1, x1, y1, x1, y0 };
                groups[grp].insert(groups[grp].end(), q, q + 12);
                for (int c = x; c < x1i; ++c) cellOwner[y * gx + c] = grp;
                x = x1i;
            }
        for (auto& gv : groups)
        {
            drawStart.push_back((UINT)(verts.size() / 2));
            drawCount.push_back((UINT)(gv.size() / 2));
            verts.insert(verts.end(), gv.begin(), gv.end());
        }
    }
    D3D11_BUFFER_DESC bd = { static_cast<UINT>(verts.size() * 4), D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
    D3D11_SUBRESOURCE_DATA sd = { verts.data(), 0, 0 };
    ID3D11Buffer* vb = nullptr; dev->CreateBuffer(&bd, &sd, &vb);

    float g[32] = {}; g[28] = 1; g[29] = -1;
    ID3D11Buffer* cbG = MakeCb(dev, g, sizeof(g));

    // cameras: game projection (wide, as with the FOV boost), eye view and projection
    const double xs = 0.368, ys = 0.368 * 1.795, a = 1.0, b = -0.1;
    M4 invGame = {}; invGame.m[0] = 1 / xs; invGame.m[5] = 1 / ys; invGame.m[11] = 1; invGame.m[14] = 1 / b; invGame.m[15] = -a / b;
    const double yaw = 20 * 0.0174533, pitch = -5 * 0.0174533;
    const double cy = std::cos(yaw), sy = std::sin(yaw), cp = std::cos(pitch), sp = std::sin(pitch);
    M4 ry = {}; ry.m[0] = cy; ry.m[2] = -sy; ry.m[5] = 1; ry.m[8] = sy; ry.m[10] = cy; ry.m[15] = 1;
    M4 rx = {}; rx.m[0] = 1; rx.m[5] = cp; rx.m[6] = -sp; rx.m[9] = sp; rx.m[10] = cp; rx.m[15] = 1;
    M4 view = Mul(rx, ry);
    view.m[3] = 0.032 + 0.08; view.m[7] = 0.05; view.m[11] = 0.03;   // eye offset + head off the game camera
    const double tl = -1.15, tr = 0.95, tu = 1.2, td = -1.15;
    M4 eye = {};
    eye.m[0] = 2 / (tr - tl); eye.m[2] = -(tr + tl) / (tr - tl);
    eye.m[5] = 2 / (tu - td); eye.m[6] = -(tu + td) / (tu - td);
    eye.m[10] = 1; eye.m[11] = -0.1; eye.m[14] = 1;
    const M4 world = Mul(eye, Mul(view, invGame));
    M4 X = {};
    if (!Inverse(world, X)) return Fail("inverse");

    // tile cb (as UpdateEyeBuffers writes it)
    float t[12] = {};
    {
        const double d = 30.0;
        const int rows[3] = { 0, 1, 3 };
        for (int r = 0; r < 3; ++r)
        {
            const double* m = &world.m[rows[r] * 4];
            t[r * 4 + 0] = (float)(m[0] * d); t[r * 4 + 1] = (float)(m[1] * d); t[r * 4 + 2] = (float)(m[2] * (a * d + b) + m[3] * d);
        }
        const double tt = std::sqrt(view.m[3] * view.m[3] + view.m[7] * view.m[7] + view.m[11] * view.m[11]);
        const double angle = std::fmin(0.6, tt / 0.35);
        t[3] = (float)(0.02 + 1.6 * angle * xs);
        t[7] = (float)(0.02 + 1.6 * angle * ys);
    }
    ID3D11Buffer* cbT = MakeCb(dev, t, sizeof(t));
    // pixel eye cb: columns of (X - I), row 4 = tile test on
    float pcb[20] = {};
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r) pcb[c * 4 + r] = (float)(X.m[r * 4 + c] - (r == c ? 1 : 0));
    pcb[16] = GetEnvironmentVariableA("TILE_TEST_OFF", nullptr, 0) ? 0.0f : 1.0f;
    ID3D11Buffer* cbP = MakeCb(dev, pcb, sizeof(pcb));

    // scene depth per eye pixel (eye clip z/w), and the tile the game assigns each point to
    const UINT W = 192, H = 192;
    std::vector<float> depth(W * H);
    std::vector<int> expect(W * H, -1);
    for (UINT y = 0; y < H; ++y)
        for (UINT x = 0; x < W; ++x)
        {
            const double nx = (x + 0.5) / W * 2 - 1, ny = 1 - (y + 0.5) / H * 2;
            const double dir[3] = { tl + (nx + 1) * 0.5 * (tr - tl), td + (ny + 1) * 0.5 * (tu - td), 1 };
            // p_game = R^T (s dir - T)
            double o[3] = {}, dg[3] = {};
            const double T3[3] = { view.m[3], view.m[7], view.m[11] };
            for (int i = 0; i < 3; ++i)
                for (int k = 0; k < 3; ++k) { o[i] += view.m[k * 4 + i] * -T3[k]; dg[i] += view.m[k * 4 + i] * dir[k]; }
            // wall z = 0.5 + 0.3 (x + 0.5) ; backdrop z = 40
            double s = -1;
            const double denom = dg[2] - 0.3 * dg[0];
            if (std::fabs(denom) > 1e-9)
            {
                const double sw = (0.5 + 0.15 + 0.3 * o[0] - o[2]) / denom;
                double pg[3] = { o[0] + sw * dg[0], o[1] + sw * dg[1], o[2] + sw * dg[2] };
                if (sw > 0 && std::fabs(pg[1]) < 0.4) s = sw;
            }
            if (s < 0 && dg[2] > 1e-6) s = (40 - o[2]) / dg[2];
            if (s < 0) continue;
            const double pg[3] = { o[0] + s * dg[0], o[1] + s * dg[1], o[2] + s * dg[2] };
            const double ze = s;   // dir.z = 1
            depth[y * W + x] = (float)(1 - 0.1 / ze);
            const double gnx = xs * pg[0] / pg[2], gny = ys * pg[1] / pg[2];
            const double fx = (gnx + 1) * 0.5 * gx, fy = (-gny + 1) * 0.5 * gy;   // tile grid is in v = (c.x, -c.y)
            if (std::fabs(fx - std::round(fx)) < 0.02 || std::fabs(fy - std::round(fy)) < 0.02) { expect[y * W + x] = -2; continue; }
            int tx = (int)std::floor(fx), ty = (int)std::floor(fy);
            tx = tx < 0 ? 0 : tx >= gx ? gx - 1 : tx;
            ty = ty < 0 ? 0 : ty >= gy ? gy - 1 : ty;
            expect[y * W + x] = cellOwner[ty * gx + tx];
        }
    D3D11_TEXTURE2D_DESC dd = { W, H, 1, 1, DXGI_FORMAT_R32_FLOAT, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 };
    D3D11_SUBRESOURCE_DATA ds = { depth.data(), W * 4, 0 };
    ID3D11Texture2D* dtex = nullptr; dev->CreateTexture2D(&dd, &ds, &dtex);
    ID3D11ShaderResourceView* dsrv = nullptr; dev->CreateShaderResourceView(dtex, nullptr, &dsrv);

    D3D11_TEXTURE2D_DESC td2 = { W, H, 1, 1, DXGI_FORMAT_R32G32B32A32_FLOAT, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_RENDER_TARGET, 0, 0 };
    ID3D11Texture2D* rt[2] = {}; ID3D11RenderTargetView* rtv[2] = {};
    for (int i = 0; i < 2; ++i) { dev->CreateTexture2D(&td2, nullptr, &rt[i]); dev->CreateRenderTargetView(rt[i], nullptr, &rtv[i]); }
    D3D11_BLEND_DESC bl = {}; bl.IndependentBlendEnable = FALSE; bl.RenderTarget[0].BlendEnable = TRUE;
    bl.RenderTarget[0].SrcBlend = bl.RenderTarget[0].DestBlend = D3D11_BLEND_ONE; bl.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    bl.RenderTarget[0].SrcBlendAlpha = bl.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE; bl.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bl.RenderTarget[0].RenderTargetWriteMask = 0xf;
    ID3D11BlendState* blend = nullptr; dev->CreateBlendState(&bl, &blend);
    D3D11_RASTERIZER_DESC rs = {}; rs.FillMode = D3D11_FILL_SOLID; rs.CullMode = D3D11_CULL_NONE; rs.DepthClipEnable = TRUE;
    ID3D11RasterizerState* raster = nullptr; dev->CreateRasterizerState(&rs, &raster);

    const float zero[4] = {};
    for (int i = 0; i < 2; ++i) ctx->ClearRenderTargetView(rtv[i], zero);
    ctx->OMSetRenderTargets(2, rtv, nullptr);
    ctx->OMSetBlendState(blend, nullptr, 0xffffffff);
    ctx->RSSetState(raster);
    D3D11_VIEWPORT vp = { 0, 0, (float)W, (float)H, 0, 1 }; ctx->RSSetViewports(1, &vp);
    UINT stride = 8, offset = 0;
    ctx->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    ctx->IASetInputLayout(layout);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ctx->VSSetShader(vs, nullptr, 0); ctx->GSSetShader(gs, nullptr, 0); ctx->PSSetShader(ps, nullptr, 0);
    ctx->VSSetConstantBuffers(0, 1, &cbG); ctx->GSSetConstantBuffers(12, 1, &cbT);
    ctx->PSSetConstantBuffers(13, 1, &cbP);
    ctx->PSSetShaderResources(0, 1, &dsrv);
    float game[32] = {};   // $Globals: identity matrix + tile id
    game[0] = game[5] = game[10] = game[15] = 1;
    for (size_t grp = 0; grp < drawStart.size(); ++grp)
    {
        if (!drawCount[grp]) continue;
        game[16] = (float)grp;
        ID3D11Buffer* cb0 = MakeCb(dev, game, sizeof(game));
        ctx->PSSetConstantBuffers(0, 1, &cb0);
        ctx->Draw(drawCount[grp], drawStart[grp]);
        cb0->Release();
    }

    td2.Usage = D3D11_USAGE_STAGING; td2.BindFlags = 0; td2.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Texture2D* st = nullptr; dev->CreateTexture2D(&td2, nullptr, &st);
    ctx->CopyResource(st, rt[0]);
    D3D11_MAPPED_SUBRESOURCE map = {};
    if (FAILED(ctx->Map(st, 0, D3D11_MAP_READ, 0, &map))) return Fail("map");
    int unlit = 0, twice = 0, wrongTile = 0, checkedPixels = 0;
    for (UINT y = 0; y < H; ++y)
        for (UINT x = 0; x < W; ++x)
        {
            const int want = expect[y * W + x];
            if (want == -1 || want == -2) continue;
            ++checkedPixels;
            const float* p = reinterpret_cast<const float*>(static_cast<const char*>(map.pData) + map.RowPitch * y) + 4 * x;
            if (p[0] < 0.5f) { ++unlit; continue; }
            if (p[0] > 1.5f) { ++twice; continue; }
            if (std::fabs(p[1] - want) > 0.01f) ++wrongTile;
        }
    ctx->Unmap(st, 0);
    std::printf("tiles (30x17 cell runs, 9 light groups), eye turned 20 deg and 12 cm off the game camera, wall at 0.5-0.8 m: %d pixels checked, unlit %d, lit twice %d, wrong tile %d\n",
                checkedPixels, unlit, twice, wrongTile);
    if (checkedPixels < (int)(W * H / 2) || unlit || twice || wrongTile)
        return Fail("lighting tiles: every pixel must be lit once, by the tile the game assigns its point to");
    std::printf("PASS: tile coverage + per-pixel tile test (real pixel patcher): each pixel lit once by its own game tile\n");

    // renderfix5 regression: a NON-tile draw through a patched shader (here a
    // full-screen triangle whose TEXCOORD0.zw is 0) with the normal 80-byte eye
    // buffer, cb13[4] = 0, must keep every pixel.
    {
        static const char fullSource[] =
            "struct O{float4 p:SV_Position;float4 t:TEXCOORD0;};"
            "O main(uint id:SV_VertexID){O o;float2 u=float2((id<<1)&2,id&2);o.p=float4(u*float2(2,-2)+float2(-1,1),0,1);o.t=float4(u,0,0);return o;}";
        ID3DBlob* fvs = nullptr;
        if (FAILED(D3DCompile(fullSource, sizeof(fullSource) - 1, nullptr, nullptr, nullptr, "main", "vs_4_0", 0, 0, &fvs, nullptr))) return Fail("full vs");
        ID3D11VertexShader* fullVs = nullptr;
        dev->CreateVertexShader(fvs->GetBufferPointer(), fvs->GetBufferSize(), nullptr, &fullVs);
        float normal[20] = {};
        std::memcpy(normal, pcb, sizeof(float) * 16);   // row 4 stays 0
        ID3D11Buffer* cbN = MakeCb(dev, normal, sizeof(normal));
        for (int i = 0; i < 2; ++i) ctx->ClearRenderTargetView(rtv[i], zero);
        ctx->VSSetShader(fullVs, nullptr, 0);
        ctx->GSSetShader(nullptr, nullptr, 0);
        ctx->IASetInputLayout(nullptr);
        ctx->PSSetConstantBuffers(13, 1, &cbN);
        ID3D11Buffer* cb0 = MakeCb(dev, game, sizeof(game));
        ctx->PSSetConstantBuffers(0, 1, &cb0);
        ctx->Draw(3, 0);
        ctx->CopyResource(st, rt[0]);
        if (FAILED(ctx->Map(st, 0, D3D11_MAP_READ, 0, &map))) return Fail("map 2");
        int dropped = 0, seen = 0;
        for (UINT y = 0; y < H; ++y)
            for (UINT x = 0; x < W; ++x)
            {
                if (depth[y * W + x] <= 0) continue;
                ++seen;
                const float* px = reinterpret_cast<const float*>(static_cast<const char*>(map.pData) + map.RowPitch * y) + 4 * x;
                if (px[0] < 0.5f) ++dropped;
            }
        ctx->Unmap(st, 0);
        std::printf("non-tile draw through a patched shader, tile test switched off: %d of %d pixels dropped\n", dropped, seen);
        if (dropped) return Fail("the tile test must not drop pixels of other draws");
        std::printf("PASS: other draws keep every pixel (cb13[4] = 0)\n");
    }
    return 0;
}
