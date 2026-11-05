// Offline check of the head-tracking controller against a simulated game:
// a camera that turns by an unknown amount per mouse count, with an unknown
// sign, applied the same frame or one frame late, in a left- or right-handed
// world. The head sweeps side to side and nods. The controller must lock on
// in every combination.
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include "../src/head_track.h"

namespace fc3 { void Log(const char* format, ...) { (void)format; } const wchar_t* DataFolder() { return L""; } }

int main()
{
    const float gains[] = { -0.0005f, -0.004f, 0.002f, -0.02f, -0.0001f };
    int failures = 0, cases = 0;
    for (float gainYaw : gains)
    for (int latency = 0; latency < 2; ++latency)
    for (int leftHanded = 0; leftHanded < 2; ++leftHanded)
    {
        ++cases;
        const float gainPitch = gainYaw * 0.8f;
        float yaw = 1.0f, pitch = 0.0f;     // game camera (yaw positive = turning left)
        long heldX = 0, heldY = 0;
        double errorSum = 0; int errorFrames = 0; double worst = 0;
        fc3head::Recenter();
        for (int frame = 0; frame < 1500; ++frame)
        {
            const float t = frame / 60.0f;
            // head: 0.5 Hz, +-35 degrees side to side, +-15 degrees nod
            const float headYaw = 0.61f * std::sin(t * 3.14159f);
            const float headPitch = 0.26f * std::sin(t * 2.1f);
            const float sy = std::sin(headYaw / 2), cy = std::cos(headYaw / 2);
            const float sx = std::sin(headPitch / 2), cx = std::cos(headPitch / 2);
            const float head[4] = { cy * sx, sy * cx, -sy * sx, cy * cx };

            fc3head::Command(head, true);
            long x = 0, y = 0;
            fc3head::TestPoll(x, y);                       // the game reads its mouse
            const long useX = latency ? heldX : x, useY = latency ? heldY : y;
            heldX = x; heldY = y;
            yaw += gainYaw * useX;
            pitch += gainPitch * useY;
            if (pitch > 1.5f) pitch = 1.5f;
            if (pitch < -1.5f) pitch = -1.5f;

            fc3head::Camera camera = {};
            const float m = leftHanded ? -1.0f : 1.0f;      // mirror the Y axis
            camera.forward[0] = std::cos(pitch) * std::cos(yaw);
            camera.forward[1] = std::cos(pitch) * std::sin(yaw) * m;
            camera.forward[2] = std::sin(pitch);
            camera.right[0] = std::sin(yaw);
            camera.right[1] = -std::cos(yaw) * m;
            camera.right[2] = 0;
            camera.up[0] = -std::sin(pitch) * std::cos(yaw);
            camera.up[1] = -std::sin(pitch) * std::sin(yaw) * m;
            camera.up[2] = std::cos(pitch);
            fc3head::FrameRendered(&camera);

            float q[4];
            if (frame > 300 && fc3head::TagOrientation(q))
            {
                // direction the frame is declared with, against where the head is
                const float fx = -(2 * (q[0] * q[2] + q[3] * q[1]));
                const float fy = -(2 * (q[1] * q[2] - q[3] * q[0]));
                const float fz = -(1 - 2 * (q[0] * q[0] + q[1] * q[1]));
                const float tagYaw = std::atan2(-fx, -fz), tagPitch = std::asin(fy);
                const double error = std::fabs(tagYaw - headYaw) + std::fabs(tagPitch - headPitch);
                errorSum += error; ++errorFrames;
                if (error > worst) worst = error;
            }
        }
        const double mean = errorFrames ? errorSum / errorFrames * 57.2958 : 999.0;
        const bool ok = errorFrames > 1000 && mean < 3.0 && worst * 57.2958 < 8.0;
        std::printf("%s response %+.4f rad/count, input %d frame late, %s-handed: lag behind the head %.2f deg mean, %.2f worst\n",
                    ok ? "ok  " : "FAIL", gainYaw, latency, leftHanded ? "left" : "right", mean, worst * 57.2958);
        if (!ok) ++failures;
    }
    std::printf(failures ? "FAIL: %d of %d cases\n" : "PASS: all %d cases lock on\n", failures ? failures : cases, cases);
    return failures ? 1 : 0;
}
