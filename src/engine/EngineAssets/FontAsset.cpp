// src/engine/EngineAssets/FontAsset.cpp
#include "FontAsset.hpp"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace Engine::EngineAssets
{
    Utils::VoidResult FontAsset::loadFromBMFont(
        Graphics::TextureManager* textureManager,
        const std::string& fntPath,
        const std::string& texturePath)
    {
        Utils::log_info("=== FontAsset::loadFromBMFont ===");
        Utils::log_info(std::format("  FNT Path: {}", fntPath));
        Utils::log_info(std::format("  Texture Path: {}", texturePath));

        if (!std::filesystem::exists(fntPath))
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::FileI0,
                std::format("Font file not found: {}", fntPath)));
            return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                "FNT file does not exist"));
        }

        if (!std::filesystem::exists(texturePath))
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::FileI0,
                std::format("Font texture not found: {}", texturePath)));
            return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                "Font texture does not exist"));
        }

        std::ifstream file(fntPath);
        if (!file.is_open())
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::FileI0,
                std::format("Failed to open font file: {}", fntPath)));
            return std::unexpected(Utils::make_error(Utils::ErrorType::FileI0,
                "Failed to open FNT file"));
        }

        Utils::log_info("  Parsing FNT file...");

        std::string line;
        int lineCount = 0;
        int charCount = 0;
        int parsedChars = 0;

        while (std::getline(file, line))
        {
            lineCount++;

            if (line.empty() || line[0] == '#')
                continue;

            std::istringstream iss(line);
            std::string type;
            iss >> type;

            if (type == "common")
            {
                std::string token;
                while (iss >> token)
                {
                    if (token.substr(0, 6) == "scaleW")
                    {
                        m_textureWidth = std::stoi(token.substr(7));
                    }
                    else if (token.substr(0, 6) == "scaleH")
                    {
                        m_textureHeight = std::stoi(token.substr(7));
                    }
                }

                Utils::log_info(std::format("  Texture dimensions: {}x{}",
                    m_textureWidth, m_textureHeight));

                if (m_textureWidth == 0 || m_textureHeight == 0)
                {
                    Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                        "Invalid texture dimensions in FNT file"));
                    return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                        "Invalid texture dimensions"));
                }
            }
            else if (type == "char")
            {
                charCount++;
                Glyph glyph;
                char id = 0;

                int x = 0, y = 0, width = 0, height = 0;
                int xoffset = 0, yoffset = 0, xadvance = 0;

                std::string token;
                while (iss >> token)
                {
                    auto pos = token.find('=');
                    if (pos == std::string::npos)
                        continue;

                    std::string key = token.substr(0, pos);
                    std::string value = token.substr(pos + 1);

                    try
                    {
                        if (key == "id")
                        {
                            id = static_cast<char>(std::stoi(value));
                        }
                        else if (key == "x")
                        {
                            x = std::stoi(value);
                        }
                        else if (key == "y")
                        {
                            y = std::stoi(value);
                        }
                        else if (key == "width")
                        {
                            width = std::stoi(value);
                        }
                        else if (key == "height")
                        {
                            height = std::stoi(value);
                        }
                        else if (key == "xoffset")
                        {
                            xoffset = std::stoi(value);
                        }
                        else if (key == "yoffset")
                        {
                            yoffset = std::stoi(value);
                        }
                        else if (key == "xadvance")
                        {
                            xadvance = std::stoi(value);
                        }
                    }
                    catch (const std::exception& e)
                    {
                        Utils::log_warning(std::format("Failed to parse token '{}': {}",
                            token, e.what()));
                    }
                }

                if (id != 0 && m_textureWidth > 0 && m_textureHeight > 0)
                {
                    glyph.uvMin.x = static_cast<float>(x) / m_textureWidth;
                    glyph.uvMin.y = static_cast<float>(y) / m_textureHeight;
                    glyph.uvMax.x = static_cast<float>(x + width) / m_textureWidth;
                    glyph.uvMax.y = static_cast<float>(y + height) / m_textureHeight;

                    glyph.size.x = static_cast<float>(width) / m_textureWidth;
                    glyph.size.y = static_cast<float>(height) / m_textureHeight;

                    glyph.bearing.x = static_cast<float>(xoffset) / m_textureWidth;
                    glyph.bearing.y = static_cast<float>(yoffset) / m_textureHeight;

                    glyph.advance = static_cast<float>(xadvance) / m_textureWidth;

                    m_glyphs[id] = glyph;
                    parsedChars++;

                    if (parsedChars <= 5)
                    {
                        Utils::log_info(std::format("  Char '{}' (id={}): size=({:.4f},{:.4f}), bearing=({:.4f},{:.4f}), advance={:.4f}",
                            id, static_cast<int>(id),
                            glyph.size.x, glyph.size.y,
                            glyph.bearing.x, glyph.bearing.y,
                            glyph.advance));
                        Utils::log_info(std::format("    UV: ({:.4f},{:.4f}) to ({:.4f},{:.4f})",
                            glyph.uvMin.x, glyph.uvMin.y,
                            glyph.uvMax.x, glyph.uvMax.y));
                    }
                }
            }
        }

        file.close();

        Utils::log_info(std::format("  Parsed {} lines, {} char definitions, {} chars stored",
            lineCount, charCount, parsedChars));

        if (m_glyphs.empty())
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::Unknown,
                "No glyphs were parsed from FNT file"));
            return std::unexpected(Utils::make_error(Utils::ErrorType::Unknown,
                "No glyphs found in FNT file"));
        }

        Utils::log_info("  Loading font texture...");
        m_texture = textureManager->loadTexture(texturePath, false, true);

        if (!m_texture)
        {
            Utils::log_error(Utils::make_error(Utils::ErrorType::ResourceCreation,
                "Failed to load font texture"));
            return std::unexpected(Utils::make_error(Utils::ErrorType::ResourceCreation,
                "Failed to load font texture"));
        }

        Utils::log_info(std::format("  Font texture loaded: {}x{}, format={}, mips={}",
            m_texture->getWidth(),
            m_texture->getHeight(),
            static_cast<int>(m_texture->getFormat()),
            m_texture->getMipLevels()));

        Utils::log_info("  Testing glyph availability:");
        const char testChars[] = "ABCabc0123 .!?";
        for (char c : testChars)
        {
            if (c == '\0') break;
            auto it = m_glyphs.find(c);
            if (it != m_glyphs.end())
            {
                Utils::log_info(std::format("    '{}' - OK (advance={:.4f})",
                    c, it->second.advance));
            }
            else
            {
                Utils::log_warning(std::format("    '{}' - MISSING", c));
            }
        }

        Utils::log_info(std::format("  Font texture loaded successfully:"));
        Utils::log_info(std::format("    Width: {}", m_texture->getWidth()));
        Utils::log_info(std::format("    Height: {}", m_texture->getHeight()));
        Utils::log_info(std::format("    Format: {}", static_cast<int>(m_texture->getFormat())));
        Utils::log_info(std::format("    Mip Levels: {}", m_texture->getMipLevels()));

        auto srvHandle = m_texture->getSRVHandle();
        Utils::log_info(std::format("    SRV Handle: 0x{:016X}", srvHandle.ptr));

        if (srvHandle.ptr == 0) {
            Utils::log_error(Utils::make_error(Utils::ErrorType::ResourceCreation,
                "Font texture SRV handle is NULL!"));
            return std::unexpected(Utils::make_error(Utils::ErrorType::ResourceCreation,
                "Invalid SRV handle"));
        }

        auto resource = m_texture->getResource();
        if (!resource) {
            Utils::log_error(Utils::make_error(Utils::ErrorType::ResourceCreation,
                "Font texture resource is NULL!"));
            return std::unexpected(Utils::make_error(Utils::ErrorType::ResourceCreation,
                "Invalid texture resource"));
        }
        Utils::log_info(std::format("    Resource: 0x{:016X}", (uint64_t)resource));

        Utils::log_info("=== FontAsset loaded successfully ===");
        return {};
    }

    const Glyph& FontAsset::getGlyph(char c) const
    {
        static Glyph defaultGlyph;

        auto it = m_glyphs.find(c);
        if (it != m_glyphs.end())
        {
            return it->second;
        }

        static bool warned = false;
        if (!warned)
        {
            Utils::log_warning(std::format("Glyph not found for character '{}' (code={})",
                c, static_cast<int>(c)));
            Utils::log_warning("Using default glyph (will appear as blank)");
            warned = true;
        }

        return defaultGlyph;
    }
}