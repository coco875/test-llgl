#pragma once

#include "math_types.h"

// Orbit camera controller for 3D model viewing
class OrbitCamera {
  public:
    void setTarget(const Math::Vec3& target, float distance);
    void reset();

    // Input handling
    void onMouseDown(int x, int y);
    void onMouseUp();
    void onMouseMove(int x, int y);
    void onMouseWheel(float delta);

    // Get matrices
    Math::Mat4 getViewMatrix() const;
    Math::Vec3 getPosition() const;

    // Properties
    float getDistance() const {
        return distance_;
    }
    void setDistance(float d) {
        distance_ = std::max(0.1f, d);
    }

    float getYaw() const {
        return yaw_;
    }
    void setYaw(float y) {
        yaw_ = y;
    }

    float getPitch() const {
        return pitch_;
    }
    void setPitch(float p) {
        pitch_ = Math::clamp(p, -maxPitch_, maxPitch_);
    }

    const Math::Vec3& getTarget() const {
        return target_;
    }

    // Settings
    float sensitivity = 0.005f;
    float zoomSensitivity = 0.1f;

  private:
    Math::Vec3 target_{ 0, 0, 0 };
    float distance_ = 5.0f;
    float yaw_ = 0.0f;
    float pitch_ = 0.0f;
    float maxPitch_ = 1.5f; // ~85 degrees

    bool isDragging_ = false;
    int lastMouseX_ = 0;
    int lastMouseY_ = 0;
};
