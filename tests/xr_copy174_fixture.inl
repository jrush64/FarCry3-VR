void TestXrCopy174(ID3D11Device* device,ID3D11DeviceContext* context)
{
    auto* priorContext=g_ctx;const auto priorCopy=g_oCopyResource;
    const UINT pixels[4]={0xff102030,0xff405060,0xff708090,0xffa0b0c0};
    D3D11_TEXTURE2D_DESC desc={};desc.Width=4;desc.Height=1;desc.MipLevels=desc.ArraySize=desc.SampleDesc.Count=1;desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
    D3D11_SUBRESOURCE_DATA data={pixels,sizeof(pixels),0};ID3D11Texture2D* source=nullptr;ID3D11Texture2D* destination=nullptr;
    Check(SUCCEEDED(device->CreateTexture2D(&desc,&data,&source)),"174 create distinct copy source");
    desc.Usage=D3D11_USAGE_STAGING;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    Check(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&destination)),"174 create copy readback");
    g_ctx=context;g_oCopyResource=nullptr;
    Check(!fc3stereo::CopyTexture(destination,source),"174 absent native copy entry cannot claim issued receipt");
    g_oCopyResource=ProbeCopy;
    Check(!fc3stereo::CopyTexture(destination,nullptr)&&!fc3stereo::CopyTexture(nullptr,source),"174 missing resource never claims copy");
    Check(fc3stereo::CopyTexture(destination,source),"174 production copy entry reports command issuance");
    D3D11_MAPPED_SUBRESOURCE mapped={};Check(SUCCEEDED(context->Map(destination,0,D3D11_MAP_READ,0,&mapped)),"174 wait/read copied output in test only");
    Check(!memcmp(mapped.pData,pixels,sizeof(pixels)),"174 production copy preserves exact source pixels");context->Unmap(destination,0);
    source->Release();destination->Release();g_ctx=priorContext;g_oCopyResource=priorCopy;
    puts("PASS:174 actual CopyTexture reports issued commands, refuses missing entry/resources and preserves GPU pixels; no XR runtime acceptance claim");
}
