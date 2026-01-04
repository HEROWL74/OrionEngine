// src/Graphics/Camera.hpp
#pragma once

#include "../Math/Math.hpp"

namespace Engine::Graphics
{
    // カメラの投影タイプ
    enum class ProjectionType
    {
        Perspective,    // 透視投影（3Dゲーム用）
        Orthographic   // 平行投影（2Dゲームや技術図面用）
    };

    // カメラクラス
    class Camera
    {
    public:
        Camera();
        ~Camera() = default;

        // コピー・ムーブ許可
        Camera(const Camera&) = default;
        Camera& operator=(const Camera&) = default;
        Camera(Camera&&) = default;
        Camera& operator=(Camera&&) = default;

        // 位置・回転の設定
        void setPosition(const Math::Vector3& position);
        void setRotation(const Math::Vector3& eulerAngles);
        void lookAt(const Math::Vector3& target, const Math::Vector3& up = Math::Vector3::up());

        // 投影設定
        void setPerspective(float fov, float aspect, float nearPlane, float farPlane);
        void setOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);

        // ゲッター
        const Math::Vector3& getPosition() const { return m_position; }
        const Math::Vector3& getRotation() const { return m_rotation; }
        Math::Vector3 getForward() const;
        Math::Vector3 getRight() const;
        Math::Vector3 getUp() const;

        float getFov() const { return m_fov; }
        float getAspect() const { return m_aspect; }
        float getNearPlane() const { return m_nearPlane; }
        float getFarPlane() const { return m_farPlane; }

        ProjectionType getProjectionType() const { return m_projectionType; }

        // 行列の取得
        Math::Matrix4 getViewMatrix() const;
        Math::Matrix4 getProjectionMatrix() const;
        Math::Matrix4 getViewProjectionMatrix() const;

        // カメラ移動（FPSスタイル）
        void moveForward(float distance);
        void moveRight(float distance);
        void moveUp(float distance);

        // カメラ回転
        void rotate(float pitch, float yaw, float roll = 0.0f);
        void rotatePitch(float pitch);
        void rotateYaw(float yaw);

        // アスペクト比を更新（ウィンドウリサイズ時に使用）
        void updateAspect(float newAspect);

        // マウス操作（FPSカメラ）
        void processMouseMovement(float deltaX, float deltaY, float sensitivity = 0.1f);

        // ビューポート座標から3D空間への変換
        Math::Vector3 screenToWorldPoint(const Math::Vector3& screenPoint, float viewportWidth, float viewportHeight) const;

    private:
        // カメラの位置・回転
        Math::Vector3 m_position;
        Math::Vector3 m_rotation;  // x=pitch, y=yaw, z=roll (度)

        // 投影設定
        ProjectionType m_projectionType;
        float m_fov;           // 視野角（度）
        float m_aspect;        // アスペクト比
        float m_nearPlane;     // 近クリップ面
        float m_farPlane;      // 遠クリップ面

        // 平行投影用
        float m_left, m_right, m_bottom, m_top;

        // 内部計算用
        mutable bool m_viewMatrixDirty;
        mutable bool m_projectionMatrixDirty;
        mutable Math::Matrix4 m_viewMatrix;
        mutable Math::Matrix4 m_projectionMatrix;

        // 行列の更新
        void updateViewMatrix() const;
        void updateProjectionMatrix() const;

        // 回転を正規化（-180~180度に収める）
        void normalizeRotation();
    };

    // カメラコントローラー（簡単なFPSカメラ操作）
    class FPSCameraController
    {
    public:
        explicit FPSCameraController(Camera* camera);

        // 入力処理
        void update(float deltaTime);
        void processKeyboard(bool forward, bool backward, bool left, bool right, bool up, bool down, float deltaTime);
        void processMouseMovement(float deltaX, float deltaY);

        // 設定
        void setMovementSpeed(float speed) { m_movementSpeed = speed; }
        void setMouseSensitivity(float sensitivity) { m_mouseSensitivity = sensitivity; }

        float getMovementSpeed() const { return m_movementSpeed; }
        float getMouseSensitivity() const { return m_mouseSensitivity; }

    private:
        Camera* m_camera;
        float m_movementSpeed;      // 移動速度
        float m_mouseSensitivity;   // マウス感度
    };
}
