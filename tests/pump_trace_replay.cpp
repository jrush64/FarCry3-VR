#include "../src/stereo.cpp"
#include <cstdlib>
void Check(bool ok, const char* what) { if (!ok) { std::fprintf(stderr,"FAIL: %s\n",what); std::exit(1); } }
std::vector<unsigned char> Read(const char* name)
{
    FILE* file=nullptr; fopen_s(&file,name,"rb"); Check(file!=nullptr,"input file");
    std::fseek(file,0,SEEK_END); long size=std::ftell(file); std::rewind(file);
    std::vector<unsigned char> bytes(size); Check(std::fread(bytes.data(),1,size,file)==size,"read file"); std::fclose(file);return bytes;
}
int main(int argc,char** argv)
{
    Check(argc==3,"trace and shader paths");
    auto trace=Read(argv[1]), shaderBytes=Read(argv[2]);
    Check(trace.size()%sizeof(PumpTraceRecord)==0,"trace layout");
    wchar_t runtimePath[MAX_PATH]; GetSystemDirectoryW(runtimePath,MAX_PATH); wcscat_s(runtimePath,L"\\d3d11.dll");
    auto create=reinterpret_cast<decltype(&D3D11CreateDevice)>(GetProcAddress(LoadLibraryW(runtimePath),"D3D11CreateDevice"));
    ID3D11Device* device=nullptr; ID3D11DeviceContext* context=nullptr;
    Check(SUCCEEDED(create(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)),"WARP");
    ID3D11VertexShader* shader=nullptr;
    Check(SUCCEEDED(device->CreateVertexShader(shaderBytes.data(),shaderBytes.size(),nullptr,&shader)),"real weapon shader");
    const unsigned long long hash=0xA38F7CD233BCE12Bull;
    shader->SetPrivateData(kShaderHashGuid,sizeof(hash),&hash); context->VSSetShader(shader,nullptr,0);
    D3D11_BUFFER_DESC desc={}; desc.ByteWidth=257896; desc.BindFlags=D3D11_BIND_VERTEX_BUFFER;
    ID3D11Buffer* buffer=nullptr; Check(SUCCEEDED(device->CreateBuffer(&desc,nullptr,&buffer)),"weapon buffer");
    UINT stride=40,offset=336;context->IASetVertexBuffers(0,1,&buffer,&stride,&offset);
    int checked=0, untouched=0;
    for(size_t at=0;at<trace.size();at+=sizeof(PumpTraceRecord))
    {
        PumpTraceRecord r; std::memcpy(&r,trace.data()+at,sizeof(r));
        if(r.kind!=0 || r.vbBytes!=257896 || r.shaderHash!=hash)continue;
        Mat4 view={};for(int i=0;i<12;++i)view.m[i]=r.world[i];view.m[15]=1;
        const int bone=r.start==9633 && r.count==6795?3:r.start==4557 && r.count==5076?1:-1;
        for(int mode=0;mode<3;++mode)
        {
            float actual[576],expected[576];std::memcpy(actual,r.bones,sizeof(actual));std::memcpy(expected,r.bones,sizeof(expected));
            g_pumpEnabled=mode!=2;g_twoHand=mode==1;g_slideGrab=.1;g_slide=.06;
            if(bone>=0 && mode!=2)
            {
                Mat4 target=BoneMatrix(r.bones,0);const double pull=mode==1?.04:0;
                for(int axis=0;axis<3;++axis)target.m[axis*4+3]-=target.m[axis*4+1]*pull;
                StoreBone(expected,bone,target);
            }
            DriveMeasuredForend(context,actual,view,r.start,r.count);
            Check(std::memcmp(actual,expected,sizeof(actual))==0,"captured palette: controlled foregrip, all other bones unchanged");
        }
        if(bone>=0)++checked;else ++untouched;
    }
    Check(checked>=100 && untouched>=100,"full animation replay coverage");
    context->ClearState();context->Flush();buffer->Release();shader->Release();context->Release();device->Release();
    std::printf("PASS: %d real foregrip draws frozen one-handed and moved 4 cm two-handed; %d other draws untouched; toggle restores original animation.\n",checked,untouched);
}
