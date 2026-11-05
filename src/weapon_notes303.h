#pragma once
#include <string>
#include <cwctype>
namespace fc3hands303 {
inline std::wstring NoteWords303(const wchar_t* text){
 std::wstring s;if(!text)return s;for(size_t i=0;i<1023&&text[i];++i)s.push_back(text[i]);return s;
}
inline std::wstring Upper303(std::wstring s){for(auto& c:s)c=wchar_t(towupper(c));return s;}
inline size_t NextWord303(const std::wstring& s,size_t n){while(n<s.size()&&(iswspace(s[n])||s[n]==L'-'))++n;return n;}
inline size_t WordEnd303(const std::wstring& s,size_t n){while(n<s.size()&&iswalpha(s[n]))++n;return n;}
inline std::wstring WeaponNote303(const wchar_t* text,bool swapButtons308=false){
 const auto s=NoteWords303(text);std::wstring out;
 for(size_t n=0;n<s.size();){
  if(!iswalpha(s[n])){out+=s[n++];continue;}
  const size_t end=WordEnd303(s,n);const auto original=s.substr(n,end-n),word=Upper303(original);std::wstring replacement;
  if(swapButtons308&&(word==L"X"||word==L"B"))replacement=word==L"X"?L"B":L"X";
  if(word==L"LT"||word==L"RT")replacement=word==L"LT"?L"RT":L"LT";
  if(word==L"LEFT"||word==L"RIGHT"){
   const size_t next=NextWord303(s,end),nextEnd=WordEnd303(s,next);const auto noun=Upper303(s.substr(next,nextEnd-next));
   bool physical=false;size_t after=NextWord303(s,nextEnd);
   if(noun==L"GRIP"&&after<s.size()&&s[after]==L'+'){
    after=NextWord303(s,after+1);const auto button=Upper303(s.substr(after,WordEnd303(s,after)-after));
    physical=button==L"Y"||button==L"MENU"||button==L"A";
   }
   if(noun==L"HAND"&&Upper303(s.substr(after,WordEnd303(s,after)-after))==L"UI")physical=true;
   if(!physical&&(noun==L"GRIP"||noun==L"HAND"||noun==L"TRIGGER"))replacement=word==L"LEFT"?L"RIGHT":L"LEFT";
  }
  if(replacement.empty())out+=original;
  else{
   const bool lower=original[0]==towlower(original[0]);
   const bool title=original.size()>1&&original[1]==towlower(original[1]);
   if(lower)for(auto& c:replacement)c=wchar_t(towlower(c));
   else if(title)for(size_t i=1;i<replacement.size();++i)replacement[i]=wchar_t(towlower(replacement[i]));
   out+=replacement;
  }
  n=end;
 }
 return out;
}
}
