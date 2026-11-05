// Test-only access to the actual native pad encoder. Not linked into the DLL.
#include "../src/vr_pad.cpp"
unsigned short PadButtons333(const fc3pad::Controllers& c){return FromControllers(c).buttons;}
