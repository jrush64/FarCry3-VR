#pragma once
#include <windows.h>
#include <openxr/openxr.h>
#include <cmath>
#include <vector>
#pragma comment(lib,"gdi32.lib")
namespace testnotes {
constexpr int Width=1024,Height=640;
constexpr float PanelWidth=.62f,PanelHeight=.3875f;
inline XrQuaternionf Inverse(XrQuaternionf q){return {-q.x,-q.y,-q.z,q.w};}
inline XrQuaternionf Multiply(XrQuaternionf a,XrQuaternionf b)
{return {a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};}
inline XrVector3f Rotate(XrQuaternionf q,XrVector3f v)
{auto r=Multiply(Multiply(q,{v.x,v.y,v.z,0}),Inverse(q));return {r.x,r.y,r.z};}
inline XrVector3f Add(XrVector3f a,XrVector3f b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline XrVector3f Sub(XrVector3f a,XrVector3f b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline XrPosef Relative(XrPosef hand,XrPosef panel)
{return {Multiply(Inverse(hand.orientation),panel.orientation),Rotate(Inverse(hand.orientation),Sub(panel.position,hand.position))};}
inline XrPosef Follow(XrPosef hand,XrPosef relative)
{return {Multiply(hand.orientation,relative.orientation),Add(hand.position,Rotate(hand.orientation,relative.position))};}
inline bool Hit(XrPosef hand,XrPosef panel)
{
    const auto o=Rotate(Inverse(panel.orientation),Sub(hand.position,panel.position));
    const auto d=Rotate(Inverse(panel.orientation),Rotate(hand.orientation,{0,0,-1}));
    if(d.z>=-.001f)return false;const float t=-o.z/d.z;
    return t>=.05f&&t<=2.5f&&std::fabs(o.x+t*d.x)<=PanelWidth*.5f&&std::fabs(o.y+t*d.y)<=PanelHeight*.5f;
}
inline bool Pixels(const wchar_t* status,const wchar_t* instructions,bool hover,std::vector<unsigned>& output,const wchar_t* title=L"VR MOD NOTES")
{
    HDC dc=CreateCompatibleDC(nullptr);if(!dc)return false;
    BITMAPINFO info={};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=Width;info.bmiHeader.biHeight=-Height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    void* bits=nullptr;HBITMAP image=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0);
    if(!image){DeleteDC(dc);return false;}auto oldBitmap=SelectObject(dc,image);SetBkMode(dc,TRANSPARENT);
    RECT all={0,0,Width,Height};HBRUSH bg=CreateSolidBrush(RGB(15,22,31));FillRect(dc,&all,bg);DeleteObject(bg);
    HBRUSH border=CreateSolidBrush(hover?RGB(80,220,170):RGB(60,90,115));FrameRect(dc,&all,border);DeleteObject(border);
    auto text=[&](const wchar_t* s,int size,int weight,COLORREF color,RECT rect){HFONT font=CreateFontW(-size,0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Segoe UI");auto old=SelectObject(dc,font);SetTextColor(dc,color);DrawTextW(dc,s,-1,&rect,DT_LEFT|DT_WORDBREAK|DT_NOPREFIX);SelectObject(dc,old);DeleteObject(font);};
    text(title,34,FW_BOLD,RGB(115,230,190),{32,22,992,66});
    text(status,28,FW_BOLD,RGB(240,245,250),{32,84,992,204});
    text(instructions,26,FW_NORMAL,RGB(225,233,241),{32,224,992,526});
    text(wcscmp(title,L"1911 GUIDE SETTINGS")==0?L"Keep the 1911 out while adjusting. X saves your values.\nClose settings before moving the notes panel.":L"Point LEFT controller at this panel + hold GRIP to move.\nRelease to park. LEFT grip + Y hides/shows it.",24,FW_NORMAL,RGB(150,176,198),{32,545,992,624});
    GdiFlush();output.resize(Width*Height);const auto* src=static_cast<const unsigned*>(bits);
    for(size_t i=0;i<output.size();++i)output[i]=src[i]|0xff000000u;
    SelectObject(dc,oldBitmap);DeleteObject(image);DeleteDC(dc);return true;
}
}
