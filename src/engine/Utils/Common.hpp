#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include <expected>
#include <string>
#include <format>
#include <source_location>

namespace Engine::Utils 
{
    // =============================================================================
    // エラー型定義
    // =============================================================================

    //エンジン内で発生するエラーの種類
    enum class ErrorType
    {
        WindowCreation, //ウィンドウ作成エラー
        DeviceCreation, //デバイス作成エラー
        SwapChainCreation,//スワップチェイン作成エラー
        ResourceCreation, // リソース作成エラー
        ShaderCompilation, // シェーダーコンパイルエラー
        FileI0, // ファイルIO エラー
        Unknown // 不明なエラー
    };

    struct Error
    {
        ErrorType type;
        std::string message;
        std::source_location location;
        HRESULT hr = S_OK; //WindowsAPI用のHRESULT

        //エラーメッセージを取得
        [[nodiscard]] std::string what() const noexcept
        {
            std::string result = std::format(
                "Error: {} at {}:{} in function '{}'\n",
                message,
                location.file_name(),
                location.line(),
                location.function_name()
            );

            if (FAILED(hr))
            {
                result += std::format("HRESULT: 0x{:08x}\n", static_cast<unsigned>(hr));
            }


            return result;
        }
    };

    // =============================================================================
   // Result型の定義（std::expectedの別名）
     // =============================================================================

    //成功時の値かエラーを表す型
    template <typename T>
    using Result = std::expected<T, Error>;

    //vpid型でも戻り値を返すようにする
    using VoidResult = Result<void>;

    // =============================================================================
     //@brief エラー作成用のヘルパー関数
     // =============================================================================

    // エラーを作成するヘルパー関数
  // HRESULT あり
    [[nodiscard]] constexpr Error make_error(
        ErrorType type,
        std::string_view message,
        HRESULT hr,
        std::source_location location = std::source_location::current()) noexcept
    {
        return Error{ type, std::string(message), location, hr };
    }

    // HRESULT なし（CHECK_CONDITION 用）
    [[nodiscard]] constexpr Error make_error(
        ErrorType type,
        std::string_view message,
        std::source_location location = std::source_location::current()) noexcept
    {
        return Error{ type, std::string(message), location, S_OK };
    }




    // =============================================================================
   // HRESULT チェック用マクロ
   // =============================================================================


    //RESULTをチェックして、失敗時にエラーを返すマクロ
#define CHECK_HR(hr, error_type, message) \
    do { \
        HRESULT _hr = (hr); \
        if (FAILED(_hr)) { \
            return std::unexpected(Engine::Utils::make_error(error_type, message, _hr)); \
        } \
    } while(0)


#undef CHECK_CONDITION

    /// 条件をチェックして、失敗時にエラーを返すマクロ
#define CHECK_CONDITION(condition, error_type, message) \
        do { \
            if (!(condition)) { \
                return std::unexpected(Engine::Utils::make_error(error_type, message)); \
            } \
        } while(0)

    //ログ出力用のヘルパー関数

    //エラーをデバッグ出力に表示
    inline void log_error(const Error& error) noexcept
    {
        OutputDebugStringA(error.what().c_str());
    }

    //情報をデバッグ出力に表示
    inline void log_info(std::string_view message) noexcept
    {
        std::string full_message = "[INFO] ";
        full_message += message;
        full_message += "\n";
        OutputDebugStringA(full_message.c_str());
    }

    //警告をデバッグに表示
    inline void log_warning(std::string_view message) noexcept
    {
        std::string full_message = "[WARNING] ";
        full_message += message;
        full_message += "\n";
        OutputDebugStringA(full_message.c_str());
    }

    /**
     * @brief 実行ファイル(EXE)のディレクトリを基準とした絶対パスを取得する
     * @param relativePath engine-assets/... などの相対パス
     * @return 変換された絶対パス
     */
    inline std::wstring get_absolute_path(const std::wstring& relativePath)
    {
        wchar_t exePath[MAX_PATH];
        if (GetModuleFileNameW(NULL, exePath, MAX_PATH) == 0)
        {
            return relativePath; // 取得失敗時は入力値をそのまま返す
        }

        std::wstring path(exePath);
        size_t lastSlash = path.find_last_of(L"\\/");
        if (lastSlash == std::wstring::npos)
        {
            return relativePath;
        }

        return path.substr(0, lastSlash) + L"\\" + relativePath;
    }
}

