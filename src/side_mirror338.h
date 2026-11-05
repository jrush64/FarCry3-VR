#pragma once
namespace fc3mirror338 {
// Side-mounted charging handles / bolt actions, including shared signature meshes.
// F1/P416 centre-line controls and the already accepted .44 cylinder remain separate.
constexpr int kProfiles[]={7,8,41,28,9,25,21,17,35,36,22,2,23,43,39};
inline bool Supported(int p){for(int q:kProfiles)if(p==q)return true;return false;}
}
