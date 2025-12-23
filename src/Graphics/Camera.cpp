// src/Graphics/Camera.cpp
#include "Camera.hpp"
#include <algorithm>

namespace Engine::Graphics
{
    Camera::Camera()
        : m_position(0.0f, 0.0f, 5.0f)
        , m_rotation(0.0f, 0.0f, 0.0f)
        , m_projectionType(ProjectionType::Perspective)
        , m_fov(45.0f)
        , m_aspect(16.0f / 9.0f)
        , m_nearPlane(0.1f)
        , m_farPlane(1000.0f)
        , m_left(-1.0f), m_right(1.0f), m_bottom(-1.0f), m_top(1.0f)
        , m_viewMatrixDirty(true)
        , m_projectionMatrixDirty(true)
    {
    }

    void Camera::setPosition(const Math::Vector3& position)
    {
        m_position = position;
        m_viewMatrixDirty = true;
    }

    void Camera::setRotation(const Math::Vector3& eulerAngles)
    {
        m_rotation = eulerAngles;
        normalizeRotation();
        m_viewMatrixDirty = true;
    }

    void Camera::lookAt(const Math::Vector3& target, const Math::Vector3& up)
    {
        Math::Vector3 forward = (target - m_position).normalized();
        Math::Vector3 right = Math::Vector3::cross(forward, up).normalized();
        Math::Vector3 newUp = Math::Vector3::cross(right, forward);

        m_rotation.y = Math::degrees(std::atan2(forward.x, forward.z));
        m_rotation.x = Math::degrees(std::asin(-forward.y));
        m_rotation.z = 0.0f;

        normalizeRotation();
        m_viewMatrixDirty = true;
    }

    void Camera::setPerspective(float fov, float aspect, float nearPlane, float farPlane)
    {
        m_projectionType = ProjectionType::Perspective;
        m_fov = fov;
        m_aspect = aspect;
        m_nearPlane = nearPlane;
        m_farPlane = farPlane;
        m_projectionMatrixDirty = true;
    }

    void Camera::setOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane)
    {
        m_projectionType = ProjectionType::Orthographic;
        m_left = left;
        m_right = right;
        m_bottom = bottom;
        m_top = top;
        m_nearPlane = nearPlane;
        m_farPlane = farPlane;
        m_projectionMatrixDirty = true;
    }

    Math::Vector3 Camera::getForward() const
    {
        float pitchRad = Math::radians(m_rotation.x);
        float yawRad = Math::radians(m_rotation.y);

        return Math::Vector3(
            std::sin(yawRad) * std::cos(pitchRad),
            -std::sin(pitchRad),
            std::cos(yawRad) * std::cos(pitchRad)
        ).normalized();
    }

    Math::Vector3 Camera::getRight() const
    {
        Math::Vector3 forward = getForward();
        return Math::Vector3::cross(forward, Math::Vector3::up()).normalized();
    }

    Math::Vector3 Camera::getUp() const
    {
        Math::Vector3 forward = getForward();
        Math::Vector3 right = getRight();
        return Math::Vector3::cross(right, forward);
    }

    Math::Matrix4 Camera::getViewMatrix() const
    {
        if (m_viewMatrixDirty)
        {
            updateViewMatrix();
        }
        return m_viewMatrix;
    }

    Math::Matrix4 Camera::getProjectionMatrix() const
    {
        if (m_projectionMatrixDirty)
        {
            updateProjectionMatrix();
        }
        return m_projectionMatrix;
    }

    Math::Matrix4 Camera::getViewProjectionMatrix() const
    {
        return getProjectionMatrix() * getViewMatrix();
    }

    void Camera::moveForward(float distance)
    {
        m_position += getForward() * distance;
        m_viewMatrixDirty = true;
    }

    void Camera::moveRight(float distance)
    {
        m_position += getRight() * distance;
        m_viewMatrixDirty = true;
    }

    void Camera::moveUp(float distance)
    {
        m_position += Math::Vector3::up() * distance;
        m_viewMatrixDirty = true;
    }

    void Camera::rotate(float pitch, float yaw, float roll)
    {
        m_rotation.x += pitch;
        m_rotation.y += yaw;
        m_rotation.z += roll;
        normalizeRotation();
        m_viewMatrixDirty = true;
    }

    void Camera::rotatePitch(float pitch)
    {
        m_rotation.x += pitch;
        m_rotation.x = std::clamp(m_rotation.x, -89.0f, 89.0f);
        m_viewMatrixDirty = true;
    }

    void Camera::rotateYaw(float yaw)
    {
        m_rotation.y += yaw;
        normalizeRotation();
        m_viewMatrixDirty = true;
    }

    void Camera::updateAspect(float newAspect)
    {
        m_aspect = newAspect;
        m_projectionMatrixDirty = true;
    }

    void Camera::processMouseMovement(float deltaX, float deltaY, float sensitivity)
    {
        rotatePitch(-deltaY * sensitivity);
        rotateYaw(deltaX * sensitivity);
    }

    Math::Vector3 Camera::screenToWorldPoint(const Math::Vector3& screenPoint, float viewportWidth, float viewportHeight) const
    {
        // NDC計算
        float x = (2.0f * screenPoint.x) / viewportWidth - 1.0f;
        float y = 1.0f - (2.0f * screenPoint.y) / viewportHeight;
        float z = screenPoint.z;

    
        Math::Vector3 clipCoords(x, y, z);

        return clipCoords;  
    }

    void Camera::updateViewMatrix() const
    {
        Math::Vector3 target = m_position + getForward();
        m_viewMatrix = Math::Matrix4::lookAt(m_position, target, Math::Vector3::up());
        m_viewMatrixDirty = false;
    }

    void Camera::updateProjectionMatrix() const
    {
        if (m_projectionType == ProjectionType::Perspective)
        {
            m_projectionMatrix = Math::Matrix4::perspective(
                Math::radians(m_fov), m_aspect, m_nearPlane, m_farPlane
            );
        }
        else
        {
            m_projectionMatrix = Math::Matrix4::orthographic(
                m_left, m_right, m_bottom, m_top, m_nearPlane, m_farPlane
            );
        }
        m_projectionMatrixDirty = false;
    }

    void Camera::normalizeRotation()
    {
        while (m_rotation.y > 180.0f) m_rotation.y -= 360.0f;
        while (m_rotation.y < -180.0f) m_rotation.y += 360.0f;

        m_rotation.x = std::clamp(m_rotation.x, -89.0f, 89.0f);
    }

    // FPSCameraController実装
    FPSCameraController::FPSCameraController(Camera* camera)
        : m_camera(camera)
        , m_movementSpeed(5.0f)
        , m_mouseSensitivity(0.1f)
    {
    }

    void FPSCameraController::update(float deltaTime)
    {
    }

    void FPSCameraController::processKeyboard(bool forward, bool backward, bool left, bool right, bool up, bool down, float deltaTime)
    {
        float velocity = m_movementSpeed * deltaTime;

        if (forward)
            m_camera->moveForward(velocity);    // 前方向
        if (backward)
            m_camera->moveForward(-velocity);   // 後ろ方向
        if (right)
            m_camera->moveRight(velocity);      //右方向
        if (left)
            m_camera->moveRight(-velocity);     // 左方向
        if (up)
            m_camera->moveUp(velocity);        // 上方向
        if (down)
            m_camera->moveUp(-velocity);       // 下方向
    }

    void FPSCameraController::processMouseMovement(float deltaX, float deltaY)
    {
        m_camera->processMouseMovement(deltaX, deltaY, m_mouseSensitivity);
    }
}
