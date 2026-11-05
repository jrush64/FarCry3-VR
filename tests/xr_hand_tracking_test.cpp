#include "../src/xr_hand_tracking.h"
#include <cstdio>
#include <cstdlib>
#include <limits>
void Check(bool value, const char* reason)
{ if (!value) { std::fprintf(stderr,"FAIL: %s\n",reason); std::exit(1); } }
int main()
{
    XrSpaceLocation hand = {XR_TYPE_SPACE_LOCATION};
    hand.pose.orientation.w = 1;
    hand.pose.position = {0.2f, 1.5f, -0.4f};
    const XrSpaceLocationFlags flags[4] = {XR_SPACE_LOCATION_POSITION_VALID_BIT,
        XR_SPACE_LOCATION_ORIENTATION_VALID_BIT, XR_SPACE_LOCATION_POSITION_TRACKED_BIT,
        XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT};
    for (unsigned mask=0; mask<16; ++mask)
    {
        hand.locationFlags=0;
        for (int bit=0;bit<4;++bit) if(mask & (1<<bit)) hand.locationFlags |= flags[bit];
        Check(fc3xr::TrackedHandLocation(hand)==(mask==15),"sleeping/partial/estimated pose rejected");
    }
    hand.pose.orientation.w=0;
    Check(!fc3xr::TrackedHandLocation(hand),"zero startup quaternion rejected");
    hand.pose.orientation.w=1;
    hand.pose.position.x=std::numeric_limits<float>::quiet_NaN();
    Check(!fc3xr::TrackedHandLocation(hand),"NaN cannot reach wrist IK");
    hand.pose.position.x=0.2f;
    hand.pose.orientation.y=0.70710678f;hand.pose.orientation.w=0.70710678f;
    Check(fc3xr::TrackedHandLocation(hand),"valid rotated hand remains accepted");
    std::puts("PASS: sleeping, estimated, partial, invalid and restored tracked hand poses");
}
