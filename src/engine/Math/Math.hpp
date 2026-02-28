// src/Math/Math.hpp
#pragma once

#include <cmath>
#include <array>
#include <initializer_list>

namespace Engine::Math
{
    // 定数
    constexpr float PI = 3.14159265359f;
    constexpr float PI2 = PI * 2.0f;
    constexpr float PI_HALF = PI * 0.5f;
    constexpr float DEG_TO_RAD = PI / 180.0f;
    constexpr float RAD_TO_DEG = 180.0f / PI;

    // ユーティリティ関数
    template<typename T>
    constexpr T clamp(T value, T min, T max)
    {
        return (value < min) ? min : (value > max) ? max : value;
    }

    constexpr float lerp(float a, float b, float t)
    {
        return a + t * (b - a);
    }

    constexpr float degrees(float radians)
    {
        return radians * RAD_TO_DEG;
    }

    constexpr float radians(float degrees)
    {
        return degrees * DEG_TO_RAD;
    }

    //2次元ベクトルクラス
    class Vector2
    {
    public:
        float x, y;

        // コンストラクタ
        constexpr Vector2() : x(0.0f), y(0.0f) {}
        constexpr Vector2(float x_, float y_) : x(x_), y(y_) {}
        constexpr Vector2(float value) : x(value), y(value) {}

        // 静的定数
        static constexpr Vector2 zero() { return Vector2(0.0f, 0.0f); }
        static constexpr Vector2 one() { return Vector2(1.0f, 1.0f); }
        static constexpr Vector2 up() { return Vector2(0.0f, 1.0f); }
        static constexpr Vector2 down() { return Vector2(0.0f, -1.0f); }
        static constexpr Vector2 left() { return Vector2(-1.0f, 0.0f); }
        static constexpr Vector2 right() { return Vector2(1.0f, 0.0f); }

        // 演算子オーバーロード
        constexpr Vector2 operator+(const Vector2& other) const
        {
            return Vector2(x + other.x, y + other.y);
        }

        constexpr Vector2 operator-(const Vector2& other) const
        {
            return Vector2(x - other.x, y - other.y);
        }

        constexpr Vector2 operator*(float scalar) const
        {
            return Vector2(x * scalar, y * scalar);
        }

        constexpr Vector2 operator*(const Vector2& other) const
        {
            return Vector2(x * other.x, y * other.y);
        }

        constexpr Vector2 operator/(float scalar) const
        {
            return Vector2(x / scalar, y / scalar);
        }

        Vector2& operator+=(const Vector2& other)
        {
            x += other.x; y += other.y;
            return *this;
        }

        Vector2& operator-=(const Vector2& other)
        {
            x -= other.x; y -= other.y;
            return *this;
        }

        Vector2& operator*=(float scalar)
        {
            x *= scalar; y *= scalar;
            return *this;
        }

        constexpr Vector2 operator-() const
        {
            return Vector2(-x, -y);
        }

        // 比較演算子
        constexpr bool operator==(const Vector2& other) const
        {
            return x == other.x && y == other.y;
        }

        constexpr bool operator!=(const Vector2& other) const
        {
            return !(*this == other);
        }

        // ベクトル操作
        float length() const
        {
            return std::sqrt(x * x + y * y);
        }

        float lengthSquared() const
        {
            return x * x + y * y;
        }

        Vector2 normalized() const
        {
            float len = length();
            if (len > 0.0f)
                return *this / len;
            return Vector2::zero();
        }

        void normalize()
        {
            *this = normalized();
        }

        //静的関数
        static float dot(const Vector2& a, const Vector2& b)
        {
            return a.x * b.x + a.y * b.y;
        }

        static Vector2 lerp(const Vector2& a, const Vector2& b, float t)
        {
            return a * (b - a) * t;
        }

        static float distance(const Vector2& a, const Vector2& b)
        {
            return (b - a).length();
        }

        //2Dの外積(スカラー値)
        static float cross(const Vector2& a, const Vector2& b)
        {
            return a.x * b.y - a.y * b.x;
        }
    };

    //スカラー×Vector2の演算子
    constexpr Vector2 operator*(float scalar, const Vector2& vector)
    {
        return vector * scalar;
    }

    // 3次元ベクトルクラス
    class Vector3
    {
    public:
        float x, y, z;

        // コンストラクタ
        constexpr Vector3() : x(0.0f), y(0.0f), z(0.0f) {}
        constexpr Vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
        constexpr Vector3(float value) : x(value), y(value), z(value) {}

        // 静的定数
        static constexpr Vector3 zero() { return Vector3(0.0f, 0.0f, 0.0f); }
        static constexpr Vector3 one() { return Vector3(1.0f, 1.0f, 1.0f); }
        static constexpr Vector3 up() { return Vector3(0.0f, 1.0f, 0.0f); }
        static constexpr Vector3 down() { return Vector3(0.0f, -1.0f, 0.0f); }
        static constexpr Vector3 left() { return Vector3(-1.0f, 0.0f, 0.0f); }
        static constexpr Vector3 right() { return Vector3(1.0f, 0.0f, 0.0f); }
        static constexpr Vector3 forward() { return Vector3(0.0f, 0.0f, 1.0f); }
        static constexpr Vector3 back() { return Vector3(0.0f, 0.0f, -1.0f); }

        // 演算子オーバーロード
        constexpr Vector3 operator+(const Vector3& other) const
        {
            return Vector3(x + other.x, y + other.y, z + other.z);
        }

        constexpr Vector3 operator-(const Vector3& other) const
        {
            return Vector3(x - other.x, y - other.y, z - other.z);
        }

        constexpr Vector3 operator*(float scalar) const
        {
            return Vector3(x * scalar, y * scalar, z * scalar);
        }

        constexpr Vector3 operator*(const Vector3& other) const
        {
            return Vector3(x * other.x, y * other.y, z * other.z);
        }

        constexpr Vector3 operator/(float scalar) const
        {
            return Vector3(x / scalar, y / scalar, z / scalar);
        }

        Vector3& operator+=(const Vector3& other)
        {
            x += other.x; y += other.y; z += other.z;
            return *this;
        }

        Vector3& operator-=(const Vector3& other)
        {
            x -= other.x; y -= other.y; z -= other.z;
            return *this;
        }

        Vector3& operator*=(float scalar)
        {
            x *= scalar; y *= scalar; z *= scalar;
            return *this;
        }

        constexpr Vector3 operator-() const
        {
            return Vector3(-x, -y, -z);
        }

        Vector3& operator=(const Vector3& other)
        {
            if (this != &other)
            {
                x = other.x;
                y = other.y;
                z = other.z;
            }
            return *this;
        }

        // 比較演算子 - UITextのdirtyフラグ用
        constexpr bool operator==(const Vector3& other) const
        {
            return x == other.x && y == other.y && z == other.z;
        }

        constexpr bool operator!=(const Vector3& other) const
        {
            return !(*this == other);
        }

        // ベクトル操作
        float length() const
        {
            return std::sqrt(x * x + y * y + z * z);
        }

        float lengthSquared() const
        {
            return x * x + y * y + z * z;
        }

        Vector3 normalized() const
        {
            float len = length();
            if (len > 0.0f)
                return *this / len;
            return Vector3::zero();
        }

        void normalize()
        {
            *this = normalized();
        }

        Vector3 cross(const Vector3& other) const
        {
            return Vector3(
                y * other.z - z * other.y,
                z * other.x - x * other.z,
                x * other.y - y * other.x
            );
        }

        // 静的関数
        static float dot(const Vector3& a, const Vector3& b)
        {
            return a.x * b.x + a.y * b.y + a.z * b.z;
        }

        static Vector3 cross(const Vector3& a, const Vector3& b)
        {
            return Vector3(
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x
            );
        }

        static Vector3 lerp(const Vector3& a, const Vector3& b, float t)
        {
            return a + (b - a) * t;
        }

        static float distance(const Vector3& a, const Vector3& b)
        {
            return (b - a).length();
        }

        float& operator[](int index)
        {
            switch (index)
            {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            default:
                return x;
            }
        }

        const float& operator[](int index) const
        {
            switch (index)
            {
            case 0: return x;
            case 1: return y;
            case 2: return z;
            default:
                return x;
            }
        }
    };

    // スカラー * Vector3の演算子
    constexpr Vector3 operator*(float scalar, const Vector3& vector)
    {
        return vector * scalar;
    }

    //4次元ベクトルクラス
    class Vector4
    {
    public:
        float x, y, z, w;

        //コンストラクタ
        constexpr Vector4() : x(0.0f), y(0.0f), z(0.0f), w(0.0f) {}
        constexpr Vector4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
        constexpr Vector4(const Vector3& xyz, float w_) : x(xyz.x), y(xyz.y), z(xyz.z), w(w_) {}
        constexpr Vector4(float value) : x(value), y(value), z(value), w(value) {}

        //静的関数
        static constexpr Vector4 zero() { return Vector4(0.0f, 0.0f, 0.0f, 0.0f); }
        static constexpr Vector4 one() { return Vector4(1.0f, 1.0f, 1.0f, 1.0f); }

        //Vector3への変換
        Vector3 xyz() const { return Vector3(x, y, z); }

        // 演算子オーバーロード
        constexpr Vector4 operator/(float scalar) const
        {
            return Vector4(x / scalar, y / scalar, z / scalar, w / scalar);
        }

        Vector4& operator/=(float scalar)
        {
            x /= scalar;
            y /= scalar;
            z /= scalar;
            w /= scalar;
            return *this;
        }

        constexpr Vector4 operator+(const Vector4& other) const
        {
            return Vector4(x + other.x, y + other.y, z + other.z, w + other.w);
        }

        constexpr Vector4 operator-(const Vector4& other) const
        {
            return Vector4(x - other.x, y - other.y, z - other.z, w - other.w);
        }

        constexpr Vector4 operator*(float scalar) const
        {
            return Vector4(x * scalar, y * scalar, z * scalar, w * scalar);
        }

        // 比較演算子
        constexpr bool operator==(const Vector4& other) const
        {
            return x == other.x && y == other.y && z == other.z && w == other.w;
        }

        constexpr bool operator!=(const Vector4& other) const
        {
            return !(*this == other);
        }
    };

    // 4x4行列クラス
    class Matrix4
    {
    public:
        // 行列データ(行優先)
        std::array<std::array<float, 4>, 4> m;

        // コンストラクタ
        Matrix4()
        {
            // デフォルトは単位行列
            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    m[i][j] = (i == j) ? 1.0f : 0.0f;
                }
            }
        }

        Matrix4(std::initializer_list<std::initializer_list<float>> values)
        {
            // まず単位行列で初期化
            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    m[i][j] = (i == j) ? 1.0f : 0.0f;
                }
            }

            // 指定された値で上書き
            int row = 0;
            for (const auto& rowData : values)
            {
                if (row >= 4) break;
                int col = 0;
                for (float value : rowData)
                {
                    if (col >= 4) break;
                    m[row][col] = value;
                    ++col;
                }
                ++row;
            }
        }

        // 要素アクセス
        float& operator()(int row, int col) { return m[row][col]; }
        const float& operator()(int row, int col) const { return m[row][col]; }

        // 単位行列を作成(静的メソッド)
        static Matrix4 identity()
        {
            Matrix4 result;
            // コンストラクタで既に単位行列になっている
            return result;
        }

        static Matrix4 scale(float x, float y, float z)
        {
            Matrix4 result;
            result.m[0][0] = x;
            result.m[1][1] = y;
            result.m[2][2] = z;
            return result;
        }

        static Matrix4 scale(const Vector3& scale)
        {
            return Matrix4::scale(scale.x, scale.y, scale.z);
        }

        // 行列演算
        Matrix4 operator*(const Matrix4& other) const
        {
            Matrix4 result;
            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    result.m[i][j] = 0.0f;
                    for (int k = 0; k < 4; ++k)
                    {
                        result.m[i][j] += m[i][k] * other.m[k][j];
                    }
                }
            }
            return result;
        }

        Vector3 transformPoint(const Vector3& point) const
        {
            float w = m[3][0] * point.x + m[3][1] * point.y + m[3][2] * point.z + m[3][3];
            if (w != 0.0f)
            {
                return Vector3(
                    (m[0][0] * point.x + m[0][1] * point.y + m[0][2] * point.z + m[0][3]) / w,
                    (m[1][0] * point.x + m[1][1] * point.y + m[1][2] * point.z + m[1][3]) / w,
                    (m[2][0] * point.x + m[2][1] * point.y + m[2][2] * point.z + m[2][3]) / w
                );
            }
            return Vector3::zero();
        }

        Vector3 transformDirection(const Vector3& direction) const
        {
            return Vector3(
                m[0][0] * direction.x + m[0][1] * direction.y + m[0][2] * direction.z,
                m[1][0] * direction.x + m[1][1] * direction.y + m[1][2] * direction.z,
                m[2][0] * direction.x + m[2][1] * direction.y + m[2][2] * direction.z
            );
        }

        // 変換行列の作成
        static Matrix4 translation(const Vector3& position)
        {
            Matrix4 result;
            result.m[0][3] = position.x;
            result.m[1][3] = position.y;
            result.m[2][3] = position.z;
            return result;
        }

        static Matrix4 rotationX(float angle)
        {
            Matrix4 result;
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);
            result.m[1][1] = cosA;
            result.m[1][2] = -sinA;
            result.m[2][1] = sinA;
            result.m[2][2] = cosA;
            return result;
        }

        static Matrix4 rotationY(float angle)
        {
            Matrix4 result;
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);
            result.m[0][0] = cosA;
            result.m[0][2] = sinA;
            result.m[2][0] = -sinA;
            result.m[2][2] = cosA;
            return result;
        }

        static Matrix4 rotationZ(float angle)
        {
            Matrix4 result;
            float cosA = std::cos(angle);
            float sinA = std::sin(angle);
            result.m[0][0] = cosA;
            result.m[0][1] = -sinA;
            result.m[1][0] = sinA;
            result.m[1][1] = cosA;
            return result;
        }

        static Matrix4 scaling(const Vector3& scale)
        {
            Matrix4 result;
            result.m[0][0] = scale.x;
            result.m[1][1] = scale.y;
            result.m[2][2] = scale.z;
            return result;
        }

        // ビュー行列
        static Matrix4 lookAt(const Vector3& eye, const Vector3& target, const Vector3& up)
        {
            Vector3 forward = (target - eye).normalized();
            Vector3 right = Vector3::cross(forward, up).normalized();
            Vector3 newUp = Vector3::cross(right, forward);

            Matrix4 result;
            result.m[0][0] = right.x;
            result.m[0][1] = right.y;
            result.m[0][2] = right.z;
            result.m[0][3] = -Vector3::dot(right, eye);

            result.m[1][0] = newUp.x;
            result.m[1][1] = newUp.y;
            result.m[1][2] = newUp.z;
            result.m[1][3] = -Vector3::dot(newUp, eye);

            result.m[2][0] = -forward.x;
            result.m[2][1] = -forward.y;
            result.m[2][2] = -forward.z;
            result.m[2][3] = Vector3::dot(forward, eye);

            result.m[3][0] = 0.0f;
            result.m[3][1] = 0.0f;
            result.m[3][2] = 0.0f;
            result.m[3][3] = 1.0f;

            return result;
        }

        // プロジェクション行列
        static Matrix4 perspective(float fovy, float aspect, float nearPlane, float farPlane)
        {
            float tanHalfFovy = std::tan(fovy * 0.5f);

            Matrix4 result;
            result.m[0][0] = 1.0f / (aspect * tanHalfFovy);
            result.m[1][1] = 1.0f / tanHalfFovy;
            result.m[2][2] = -(farPlane + nearPlane) / (farPlane - nearPlane);
            result.m[2][3] = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
            result.m[3][2] = -1.0f;
            result.m[3][3] = 0.0f;

            return result;
        }

        static Matrix4 orthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane)
        {
            Matrix4 result;
            result.m[0][0] = 2.0f / (right - left);
            result.m[1][1] = 2.0f / (top - bottom);
            result.m[2][2] = -2.0f / (farPlane - nearPlane);
            result.m[0][3] = -(right + left) / (right - left);
            result.m[1][3] = -(top + bottom) / (top - bottom);
            result.m[2][3] = -(farPlane + nearPlane) / (farPlane - nearPlane);

            return result;
        }

        Vector4 operator*(const Vector4& v) const
        {
            return Vector4(
                m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z + m[0][3] * v.w,
                m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z + m[1][3] * v.w,
                m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z + m[2][3] * v.w,
                m[3][0] * v.x + m[3][1] * v.y + m[3][2] * v.z + m[3][3] * v.w
            );
        }

        Matrix4 inverse() const
        {
            Matrix4 inv;
            float det{};

            // 余因子行列を計算
            inv.m[0][0] = m[1][1] * m[2][2] * m[3][3] -
                m[1][1] * m[2][3] * m[3][2] -
                m[2][1] * m[1][2] * m[3][3] +
                m[2][1] * m[1][3] * m[3][2] +
                m[3][1] * m[1][2] * m[2][3] -
                m[3][1] * m[1][3] * m[2][2];

            inv.m[1][0] = -m[1][0] * m[2][2] * m[3][3] +
                m[1][0] * m[2][3] * m[3][2] +
                m[2][0] * m[1][2] * m[3][3] -
                m[2][0] * m[1][3] * m[3][2] -
                m[3][0] * m[1][2] * m[2][3] +
                m[3][0] * m[1][3] * m[2][2];

            inv.m[2][0] = m[1][0] * m[2][1] * m[3][3] -
                m[1][0] * m[2][3] * m[3][1] -
                m[2][0] * m[1][1] * m[3][3] +
                m[2][0] * m[1][3] * m[3][1] +
                m[3][0] * m[1][1] * m[2][3] -
                m[3][0] * m[1][3] * m[2][1];

            inv.m[3][0] = -m[1][0] * m[2][1] * m[3][2] +
                m[1][0] * m[2][2] * m[3][1] +
                m[2][0] * m[1][1] * m[3][2] -
                m[2][0] * m[1][2] * m[3][1] -
                m[3][0] * m[1][1] * m[2][2] +
                m[3][0] * m[1][2] * m[2][1];

            inv.m[0][1] = -m[0][1] * m[2][2] * m[3][3] +
                m[0][1] * m[2][3] * m[3][2] +
                m[2][1] * m[0][2] * m[3][3] -
                m[2][1] * m[0][3] * m[3][2] -
                m[3][1] * m[0][2] * m[2][3] +
                m[3][1] * m[0][3] * m[2][2];

            inv.m[1][1] = m[0][0] * m[2][2] * m[3][3] -
                m[0][0] * m[2][3] * m[3][2] -
                m[2][0] * m[0][2] * m[3][3] +
                m[2][0] * m[0][3] * m[3][2] +
                m[3][0] * m[0][2] * m[2][3] -
                m[3][0] * m[0][3] * m[2][2];

            inv.m[2][1] = -m[0][0] * m[2][1] * m[3][3] +
                m[0][0] * m[2][3] * m[3][1] +
                m[2][0] * m[0][1] * m[3][3] -
                m[2][0] * m[0][3] * m[3][1] -
                m[3][0] * m[0][1] * m[2][3] +
                m[3][0] * m[0][3] * m[2][1];

            inv.m[3][1] = m[0][0] * m[2][1] * m[3][2] -
                m[0][0] * m[2][2] * m[3][1] -
                m[2][0] * m[0][1] * m[3][2] +
                m[2][0] * m[0][2] * m[3][1] +
                m[3][0] * m[0][1] * m[2][2] -
                m[3][0] * m[0][2] * m[2][1];

            inv.m[0][2] = m[0][1] * m[1][2] * m[3][3] -
                m[0][1] * m[1][3] * m[3][2] -
                m[1][1] * m[0][2] * m[3][3] +
                m[1][1] * m[0][3] * m[3][2] +
                m[3][1] * m[0][2] * m[1][3] -
                m[3][1] * m[0][3] * m[1][2];

            inv.m[1][2] = -m[0][0] * m[1][2] * m[3][3] +
                m[0][0] * m[1][3] * m[3][2] +
                m[1][0] * m[0][2] * m[3][3] -
                m[1][0] * m[0][3] * m[3][2] -
                m[3][0] * m[0][2] * m[1][3] +
                m[3][0] * m[0][3] * m[1][2];

            inv.m[2][2] = m[0][0] * m[1][1] * m[3][3] -
                m[0][0] * m[1][3] * m[3][1] -
                m[1][0] * m[0][1] * m[3][3] +
                m[1][0] * m[0][3] * m[3][1] +
                m[3][0] * m[0][1] * m[1][3] -
                m[3][0] * m[0][3] * m[1][1];

            inv.m[3][2] = -m[0][0] * m[1][1] * m[3][2] +
                m[0][0] * m[1][2] * m[3][1] +
                m[1][0] * m[0][1] * m[3][2] -
                m[1][0] * m[0][2] * m[3][1] -
                m[3][0] * m[0][1] * m[1][2] +
                m[3][0] * m[0][2] * m[1][1];

            inv.m[0][3] = -m[0][1] * m[1][2] * m[2][3] +
                m[0][1] * m[1][3] * m[2][2] +
                m[1][1] * m[0][2] * m[2][3] -
                m[1][1] * m[0][3] * m[2][2] -
                m[2][1] * m[0][2] * m[1][3] +
                m[2][1] * m[0][3] * m[1][2];

            inv.m[1][3] = m[0][0] * m[1][2] * m[2][3] -
                m[0][0] * m[1][3] * m[2][2] -
                m[1][0] * m[0][2] * m[2][3] +
                m[1][0] * m[0][3] * m[2][2] +
                m[2][0] * m[0][2] * m[1][3] -
                m[2][0] * m[0][3] * m[1][2];

            inv.m[2][3] = -m[0][0] * m[1][1] * m[2][3] +
                m[0][0] * m[1][3] * m[2][1] +
                m[1][0] * m[0][1] * m[2][3] -
                m[1][0] * m[0][3] * m[2][1] -
                m[2][0] * m[0][1] * m[1][3] +
                m[2][0] * m[0][3] * m[1][1];

            inv.m[3][3] = m[0][0] * m[1][1] * m[2][2] -
                m[0][0] * m[1][2] * m[2][1] -
                m[1][0] * m[0][1] * m[2][2] +
                m[1][0] * m[0][2] * m[2][1] +
                m[2][0] * m[0][1] * m[1][2] -
                m[2][0] * m[0][2] * m[1][1];

            // 行列式を計算
            det = m[0][0] * inv.m[0][0] + m[0][1] * inv.m[1][0] +
                m[0][2] * inv.m[2][0] + m[0][3] * inv.m[3][0];

            if (std::abs(det) < 1e-6f)
            {
                // 逆行列が存在しない場合は単位行列を返す
                return Matrix4::identity();
            }

            det = 1.0f / det;

            // 全要素をスケール
            for (int i = 0; i < 4; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    inv.m[i][j] *= det;
                }
            }

            return inv;
        }

        // データポインターを取得(DirectX用)
        const float* data() const { return &m[0][0]; }
    };
}

