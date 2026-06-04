#!/usr/bin/env python3
"""
Converte main.cpp da FreeType+sf::Image a stb_truetype+stb_image.
Applicabile sia a tappa-24 (master) che alle tappe 15-23.
"""
import sys, re

# ── Sostituzione initFontAtlas (FreeType -> stb_truetype) ─────────────────────

NEW_INIT_FONT = '''\
bool initFontAtlas()
{
    const std::vector<std::string> fontPaths = {
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/calibri.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/Library/Fonts/Arial.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
        "/usr/share/fonts/opentype/urw-base35/NimbusSans-Regular.otf"
    };

    std::vector<unsigned char> fontBuffer;
    for (const auto& path : fontPaths)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file) continue;
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        fontBuffer.resize(static_cast<std::size_t>(size));
        if (file.read(reinterpret_cast<char*>(fontBuffer.data()), size))
            break;
        fontBuffer.clear();
    }

    if (fontBuffer.empty())
    {
        std::cerr << "Pannello: nessun font di sistema trovato.\\n";
        return false;
    }

    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, fontBuffer.data(), 0))
    {
        std::cerr << "Pannello: font non valido.\\n";
        return false;
    }

    float scale = stbtt_ScaleForPixelHeight(&font, static_cast<float>(UI_FONT_PX));

    std::vector<unsigned char> atlas(UI_ATLAS_W * UI_ATLAS_H, 0);

    int penX = 0, penY = 0, rowH = 0;

    for (int c = UI_FIRST_CHAR; c <= UI_LAST_CHAR; ++c)
    {
        int advance, lsb, x0, y0, x1, y1;
        stbtt_GetCodepointHMetrics(&font, c, &advance, &lsb);
        stbtt_GetCodepointBitmapBox(&font, c, scale, scale, &x0, &y0, &x1, &y1);

        int bw = x1 - x0;
        int bh = y1 - y0;

        if (penX + bw > UI_ATLAS_W)
        {
            penX = 0;
            penY += rowH + 2;
            rowH = 0;
        }

        if (bh > rowH) rowH = bh;

        GlyphInfo& gi = s_glyphs[c - UI_FIRST_CHAR];
        gi.atlasX   = penX;
        gi.atlasY   = penY;
        gi.width    = bw;
        gi.height   = bh;
        gi.bearingX = x0;
        gi.bearingY = -y0;
        gi.advance  = static_cast<int>(std::round(advance * scale));

        if (bw > 0 && bh > 0)
        {
            stbtt_MakeCodepointBitmap(
                &font,
                atlas.data() + penY * UI_ATLAS_W + penX,
                bw, bh, UI_ATLAS_W,
                scale, scale, c
            );
        }

        penX += bw + 2;
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &s_atlasTexture);
    glBindTexture(GL_TEXTURE_2D, s_atlasTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, UI_ATLAS_W, UI_ATLAS_H, 0,
                 GL_RED, GL_UNSIGNED_BYTE, atlas.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    return true;
}'''

# ── Sostituzione loadTextureFromFile (sf::Image -> stb_image) ─────────────────

NEW_LOAD_TEX = '''\
GLuint loadTextureFromFile(const std::string& path, int fallbackTextureType)
{
    std::vector<std::string> candidatePaths = {
        path,
        "../" + path,
        "../../" + path,
        "../../../" + path
    };

    int width = 0, height = 0, channels = 0;
    unsigned char* pixels = nullptr;
    std::string loadedPath;

    stbi_set_flip_vertically_on_load(0);

    for (const std::string& candidatePath : candidatePaths)
    {
        pixels = stbi_load(candidatePath.c_str(), &width, &height, &channels, 4);
        if (pixels)
        {
            loadedPath = candidatePath;
            break;
        }
    }

    if (!pixels)
    {
        std::cerr << "Impossibile caricare la texture: " << path
                  << ". Uso texture procedurale di fallback.\\n";
        return createProceduralTexture(fallbackTextureType);
    }

    GLuint textureID = 0;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(
        GL_TEXTURE_2D, 0, GL_RGBA,
        static_cast<GLsizei>(width), static_cast<GLsizei>(height),
        0, GL_RGBA, GL_UNSIGNED_BYTE, pixels
    );

    stbi_image_free(pixels);

    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    std::cout << "Texture caricata: " << loadedPath << "\\n";
    return textureID;
}'''


def remove_function(src, func_name, return_type='GLuint'):
    pattern = re.compile(r'(?m)^' + re.escape(return_type) + r'\s+' + re.escape(func_name) + r'\s*\(')
    match = pattern.search(src)
    if not match:
        return src
    line_start = src.rfind('\n', 0, match.start())
    line_start = 0 if line_start == -1 else line_start + 1
    brace_open = src.find('{', match.end())
    if brace_open == -1:
        return src
    depth, pos = 0, brace_open
    while pos < len(src):
        if src[pos] == '{': depth += 1
        elif src[pos] == '}':
            depth -= 1
            if depth == 0:
                end = pos + 1
                while end < len(src) and src[end] in '\n\r': end += 1
                return src[:line_start] + src[end:]
        pos += 1
    return src


def transform(src, is_tappa24):
    # 1. Include: rimuovi SFML/Graphics.hpp e FreeType, aggiungi stb
    src = src.replace('#include <SFML/Graphics.hpp>\n', '')
    src = src.replace('#include <ft2build.h>\n', '')
    src = src.replace('#include FT_FREETYPE_H\n', '')

    # Aggiungi stb dopo <SFML/Window.hpp>
    marker = '#include <SFML/Window.hpp>'
    if marker in src:
        stb_block = '#define STB_IMAGE_IMPLEMENTATION\n#include <stb_image.h>\n'
        if is_tappa24:
            stb_block += '#define STB_TRUETYPE_IMPLEMENTATION\n#include <stb_truetype.h>\n'
        if 'stb_image' not in src:
            idx = src.find(marker)
            end = src.find('\n', idx) + 1
            src = src[:end] + stb_block + src[end:]

    # 2. Aggiungi <fstream> se mancante
    if '#include <fstream>' not in src:
        src = src.replace('#include <iostream>', '#include <fstream>\n#include <iostream>')

    # 3. Sostituisci loadTextureFromFile
    src = remove_function(src, 'loadTextureFromFile', 'GLuint')
    # Inserisci prima di drawSphere
    marker2 = 'void drawSphere('
    idx = src.find(marker2)
    if idx != -1:
        src = src[:idx] + NEW_LOAD_TEX + '\n\n' + src[idx:]

    # 4. (solo tappa-24) Sostituisci initFontAtlas
    if is_tappa24:
        src = remove_function(src, 'initFontAtlas', 'bool')
        # Inserisci dopo createUiShader
        marker3 = 'bool initUI('
        idx = src.find(marker3)
        if idx != -1:
            src = src[:idx] + NEW_INIT_FONT + '\n\n' + src[idx:]

    src = re.sub(r'\n{4,}', '\n\n\n', src)
    return src


def main():
    if len(sys.argv) < 2:
        print(f"Uso: {sys.argv[0]} <main.cpp> [--tappa24]")
        sys.exit(1)
    path = sys.argv[1]
    is_tappa24 = '--tappa24' in sys.argv
    with open(path, 'r', encoding='utf-8') as f:
        src = f.read()
    result = transform(src, is_tappa24)
    with open(path, 'w', encoding='utf-8') as f:
        f.write(result)
    print(f"OK: {path}")

if __name__ == '__main__':
    main()
