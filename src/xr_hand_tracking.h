#pragma once
#include <openxr/openxr.h>
#include <cmath>

namespace fc3xr
{
// VALID permits a retained/estimated pose. Do not drive arms or arm calibration
// from it unless the runtime also reports current position/orientation tracking.
inline bool TrackedHandLocation(const XrSpaceLocation& location)
{
    const XrSpaceLocationFlags required = XR_SPACE_LOCATION_POSITION_VALID_BIT |
        XR_SPACE_LOCATION_ORIENTATION_VALID_BIT | XR_SPACE_LOCATION_POSITION_TRACKED_BIT |
        XR_SPACE_LOCATION_ORIENTATION_TRACKED_BIT;
    if ((location.locationFlags & required) != required) return false;
    const auto& p = location.pose.position;
    const auto& q = location.pose.orientation;
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z) ||
        !std::isfinite(q.x) || !std::isfinite(q.y) || !std::isfinite(q.z) || !std::isfinite(q.w)) return false;
    const float norm = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
    return std::fabs(norm - 1.0f) < 0.02f;
}
}
