// Actual Molotov_Cloth draw is start0/count2799, with two skin slots.
// start2799/count2880 is Molotov_Glass. FX_FIRE is a bottle-neck model node,
// not the loose cloth tip. These two samples come from real cloth vertices
// 13 and259 (signed16 position scale1/32767, weights/indices at16/20).
struct MolotovRag177 {float contact[3]={},flame[3]={},radius=.025f;bool guide=false;int saved=0;}g_molotovRag177;
V3 MolotovRagSample177(const Mat4& frame,bool tip){
 const V3 p=tip?V3{.012176885269582272,-.03637806296348572,.17822809517383575}:V3{-.0005493331700563431,-.04354991391301155,.21066927164793015};
 const auto& key=kThrowMolotovClipB[kThrowMolotovBIdx_rest];
 const double weight=tip?254./255.:237./255.;
 return Add(Scale(Apply(Multiply(frame,ThrowMat144(key.part[1])),p),weight),Scale(Apply(Multiply(frame,ThrowMat144(key.part[2])),p),1-weight));
}
V3 MolotovRagOffset177(const Mat4& frame,const V3& point,const float* offset){
 const Mat4 cloth=Multiply(frame,ThrowMat144(kThrowMolotovClipB[kThrowMolotovBIdx_rest].part[1]));
 return Add(point,Sub(Apply(cloth,{offset[0],offset[1],offset[2]}),Apply(cloth,{0,0,0})));
}
V3 MolotovRagPoint177(const Mat4& frame,bool flame){return MolotovRagOffset177(frame,MolotovRagSample177(frame,true),flame?g_molotovRag177.flame:g_molotovRag177.contact);}
float* MolotovRagValue177(int n){return n<3?g_molotovRag177.contact+n:n<6?g_molotovRag177.flame+n-3:&g_molotovRag177.radius;}
const wchar_t* const kMolotovRagKeys177[]={L"ContactX",L"ContactY",L"ContactZ",L"FlameX",L"FlameY",L"FlameZ",L"ContactRadius"};
void LoadMolotovRag177(const wchar_t* path){for(int n=0;n<7;++n){wchar_t text[64]={};GetPrivateProfileStringW(L"MolotovRag",kMolotovRagKeys177[n],L"",text,64,path);wchar_t* end=nullptr;float value=text[0]?float(wcstod(text,&end)):(n==6?.025f:0.f);if(!std::isfinite(value)||(text[0]&&(!end||end==text||*end))||(n==6?(value<.005f||value>.08f):fabs(value)>.3f))value=n==6?.025f:0.f;*MolotovRagValue177(n)=value;}}
bool SaveMolotovRag177(const wchar_t* path){bool ok=true;for(int n=0;n<7;++n){wchar_t value[48];swprintf_s(value,L"%.9g",double(*MolotovRagValue177(n)));if(!WritePrivateProfileStringW(L"MolotovRag",kMolotovRagKeys177[n],value,path))ok=false;}return ok;}

// FX_FIRE node in the shipped XBG has quaternion (+sqrt(.5),0,0,+sqrt(.5)).
// Keep that authored +90 degree X basis; the cloth palette alone is a skin frame,
// not an emitter frame. Only the translation is replaced by the weighted rag point.
Mat4 MolotovEmitterBasis178(const Mat4& cloth){
 Mat4 fx=HandIdentity();fx.m[5]=0;fx.m[6]=-1;fx.m[9]=1;fx.m[10]=0;
 return Multiply(cloth,fx);
}
