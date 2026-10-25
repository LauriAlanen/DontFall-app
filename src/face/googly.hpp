// Googly eyes: where the eyes look is a mass on a spring. The spring pulls
// it towards the gaze the idle life wants; the IMU pushes it like a real
// googly eye's pupil (tilt: it slides downhill, shake: it rattles). It is
// damped, stops at a round edge with a little bounce, and squashes when it
// hits the edge hard.
#pragma once

#include "face.hpp"

namespace dontfall::face {

// The push on the eyes in screen axes (x right, y down), in m/s^2, from
// the accelerometer reading in the IMU's axes. A free mass feels the
// opposite of what the accelerometer reads (at rest: gravity, downhill).
// How the IMU sits relative to the screen: on the bench the module lies
// turned 90 degrees to the panel's landscape view, so screen x follows the
// IMU's y and screen y its x (signs checked by tilting, face plan step 4).
// The one place to change once the robot exists, like the M4's mount matrix.
inline void imu_push(float ax, float ay, float &push_x, float &push_y)
{
    push_x = ay;
    push_y = ax;
}

class Googly {
public:
    // dt: seconds since the last update. target: where the idle life looks.
    // push_x/y: from imu_push(), 0 without an IMU.
    void update(float dt, const Gaze &target, float push_x, float push_y);

    const Gaze &gaze() const { return pos_; }
    float squash_x() const { return squash_x_; }
    float squash_y() const { return squash_y_; }
    // How hard the eyes hit the edge in the last update, 0..1 (0: no hit).
    float hit() const { return hit_; }

private:
    float push_x_ = 0;  // the push after smoothing
    float push_y_ = 0;
    Gaze pos_;
    Gaze vel_;
    float hit_ = 0;
    float squash_x_ = 0;
    float squash_y_ = 0;
};

} // namespace dontfall::face
