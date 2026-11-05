#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <vector>
#include <cstring>

// Diagnostic only: read sample zero verbatim, never resolve/average depth.
// This is the same implementation used by the game hook and WARP test.
namespace fc3shadow {
template<class T> inline void Release(T*& p) { if (p) p->Release(); p = nullptr; }
inline HRESULT ReadSample0(ID3D11DeviceContext* ctx, ID3D11ShaderResourceView* source,
                           std::vector<float>& pixels, UINT& width, UINT& height)
{
    pixels.clear(); width = height = 0;
    if (!ctx || !source || ctx->GetType() != D3D11_DEVICE_CONTEXT_IMMEDIATE) return E_INVALIDARG;
    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {}; source->GetDesc(&sd);
    if (sd.ViewDimension != D3D11_SRV_DIMENSION_TEXTURE2DMS) return E_INVALIDARG;
    ID3D11Resource* resource = nullptr; source->GetResource(&resource);
    ID3D11Texture2D* input = nullptr;
    HRESULT hr = resource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&input));
    Release(resource); if (FAILED(hr)) return hr;
    D3D11_TEXTURE2D_DESC td = {}; input->GetDesc(&td); Release(input);
    ID3D11Device* dev = nullptr; ctx->GetDevice(&dev);
    const char code[] = "Texture2DMS<float4> S:register(t0); RWTexture2D<float> O:register(u0);"
        "[numthreads(8,8,1)] void main(uint3 p:SV_DispatchThreadID){uint w,h;O.GetDimensions(w,h);"
        "if(p.x<w&&p.y<h)O[p.xy]=S.Load(p.xy,0).x;}";
    ID3DBlob* blob = nullptr; ID3DBlob* errors = nullptr;
    hr = D3DCompile(code, sizeof(code)-1, nullptr, nullptr, nullptr, "main", "cs_5_0", 0, 0, &blob, &errors);
    Release(errors);
    ID3D11ComputeShader* shader = nullptr;
    if (SUCCEEDED(hr)) hr = dev->CreateComputeShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &shader);
    Release(blob);
    td.MipLevels = td.ArraySize = 1; td.SampleDesc.Count = 1; td.SampleDesc.Quality = 0;
    td.Format = DXGI_FORMAT_R32_FLOAT; td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_UNORDERED_ACCESS; td.CPUAccessFlags = td.MiscFlags = 0;
    ID3D11Texture2D* output = nullptr; ID3D11Texture2D* staging = nullptr;
    ID3D11UnorderedAccessView* uav = nullptr;
    if (SUCCEEDED(hr)) hr = dev->CreateTexture2D(&td, nullptr, &output);
    if (SUCCEEDED(hr)) hr = dev->CreateUnorderedAccessView(output, nullptr, &uav);
    td.Usage = D3D11_USAGE_STAGING; td.BindFlags = 0; td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    if (SUCCEEDED(hr)) hr = dev->CreateTexture2D(&td, nullptr, &staging);
    if (SUCCEEDED(hr)) {
        ID3D11ComputeShader* oldShader = nullptr; ID3D11ClassInstance* instances[256] = {}; UINT count = 256;
        ID3D11ShaderResourceView* oldSrv = nullptr; ID3D11UnorderedAccessView* oldUav = nullptr;
        ID3D11RenderTargetView* rt[8] = {}; ID3D11DepthStencilView* ds = nullptr;
        ctx->CSGetShader(&oldShader, instances, &count); ctx->CSGetShaderResources(0,1,&oldSrv);
        ctx->CSGetUnorderedAccessViews(0,1,&oldUav); ctx->OMGetRenderTargets(8,rt,&ds);
        // Source may be the just-written shadow RTV or currently bound DSV.
        ctx->OMSetRenderTargets(0,nullptr,nullptr);
        ctx->CSSetShader(shader,nullptr,0); ctx->CSSetUnorderedAccessViews(0,1,&uav,nullptr);
        ctx->CSSetShaderResources(0,1,&source); ctx->Dispatch((td.Width+7)/8,(td.Height+7)/8,1);
        ID3D11ShaderResourceView* noSrv = nullptr; ID3D11UnorderedAccessView* noUav = nullptr;
        ctx->CSSetShaderResources(0,1,&noSrv); ctx->CSSetUnorderedAccessViews(0,1,&noUav,nullptr);
        ctx->CSSetShader(oldShader,instances,count); ctx->CSSetUnorderedAccessViews(0,1,&oldUav,nullptr);
        ctx->CSSetShaderResources(0,1,&oldSrv); ctx->OMSetRenderTargets(8,rt,ds);
        Release(oldShader); Release(oldSrv); Release(oldUav); Release(ds);
        for (auto& p:rt) Release(p); for (UINT i=0;i<count;++i) Release(instances[i]);
        ctx->CopyResource(staging,output);
        D3D11_MAPPED_SUBRESOURCE map = {}; hr = ctx->Map(staging,0,D3D11_MAP_READ,0,&map);
        if (SUCCEEDED(hr)) {
            width=td.Width; height=td.Height; pixels.resize(static_cast<size_t>(width)*height);
            for (UINT y=0;y<height;++y) std::memcpy(pixels.data()+static_cast<size_t>(y)*width,
                static_cast<const char*>(map.pData)+static_cast<size_t>(y)*map.RowPitch,width*sizeof(float));
            ctx->Unmap(staging,0);
        }
    }
    Release(uav); Release(staging); Release(output); Release(shader); Release(dev); return hr;
}
}
