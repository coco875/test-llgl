#include "camera.h"

void OrbitCamera::setTarget(const Math::Vec3& target, float distance) {
    target_ = target;
    distance_ = distance;
}

void OrbitCamera::reset() {
    yaw_ = 0.0f;
    pitch_ = 0.0f;
}

void OrbitCamera::onMouseDown(int x, int y) {
    isDragging_ = true;
    lastMouseX_ = x;
    lastMouseY_ = y;
}

void OrbitCamera::onMouseUp() {
    isDragging_ = false;
}

void OrbitCamera::onMouseMove(int x, int y) {
    if (!isDragging_)
        return;

    int deltaX = x - lastMouseX_;
    int deltaY = y - lastMouseY_;

    yaw_ += deltaX * sensitivity;
    pitch_ = Math::clamp(pitch_ + deltaY * sensitivity, -maxPitch_, maxPitch_);

    lastMouseX_ = x;
    lastMouseY_ = y;
}

void OrbitCamera::onMouseWheel(float delta) {
    distance_ = std::max(0.1f, distance_ - delta * zoomSensitivity * distance_);
}

Math::Vec3 OrbitCamera::getPosition() const {
    return { target_.x + distance_ * std::cos(pitch_) * std::sin(yaw_), target_.y + distance_ * std::sin(pitch_),
             target_.z + distance_ * std::cos(pitch_) * std::cos(yaw_) };
}

Math::Mat4 OrbitCamera::getViewMatrix() const {
    return Math::Mat4::lookAt(getPosition(), target_, { 0, 1, 0 });
}
