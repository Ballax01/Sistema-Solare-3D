#!/usr/bin/env python3
"""
Sostituisce sf::Image con stb_image nelle tappe 15-23.
Legge main.cpp di un ramo, rimpiazza loadTextureFromFile,
rimuove #include <SFML/Graphics.hpp>, aggiunge #include stb.

Uso: python fix_sfml_graphics.py <main.cpp>
"""

import sys
import re

STB_INCLUDES = """\
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
"""

NEW_LOAD_TEXTURE = '''\
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
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        static_cast<GLsizei>(width),
        static_cast<GLsizei>(height),
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        pixels
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


def remove_function(src: str, func_name: str) -> str:
    pattern = re.compile(
        r'(?m)^GLuint\s+' + re.escape(func_name) + r'\s*\('
    )
    match = pattern.search(src)
    if not match:
        return src
    line_start = src.rfind('\n', 0, match.start())
    line_start = 0 if line_start == -1 else line_start + 1
    brace_open = src.find('{', match.end())
    if brace_open == -1:
        return src
    depth = 0
    pos = brace_open
    while pos < len(src):
        if src[pos] == '{':
            depth += 1
        elif src[pos] == '}':
            depth -= 1
            if depth == 0:
                end = pos + 1
                while end < len(src) and src[end] in '\n\r':
                    end += 1
                return src[:line_start] + src[end:]
        pos += 1
    return src


def transform(src: str) -> str:
    # 1. Rimuovi #include <SFML/Graphics.hpp>
    src = src.replace('#include <SFML/Graphics.hpp>\n', '')

    # 2. Aggiungi <fstream> se mancante
    if '#include <fstream>' not in src:
        src = src.replace('#include <iostream>', '#include <fstream>\n#include <iostream>')

    # 3. Aggiungi stb_image dopo #include <SFML/Window.hpp>
    sfml_window = '#include <SFML/Window.hpp>'
    if sfml_window in src and 'stb_image' not in src:
        idx = src.find(sfml_window)
        end = src.find('\n', idx) + 1
        src = src[:end] + STB_INCLUDES + src[end:]

    # 4. Sostituisci loadTextureFromFile
    src = remove_function(src, 'loadTextureFromFile')
    # Reinserisci dopo createProceduralTexture
    marker = 'void drawSphere('
    idx = src.find(marker)
    if idx != -1:
        src = src[:idx] + NEW_LOAD_TEXTURE + '\n\n' + src[idx:]

    return src


def main():
    if len(sys.argv) < 2:
        print(f"Uso: {sys.argv[0]} <main.cpp>")
        sys.exit(1)

    path = sys.argv[1]
    with open(path, 'r', encoding='utf-8') as f:
        src = f.read()

    result = transform(src)

    with open(path, 'w', encoding='utf-8') as f:
        f.write(result)

    print(f"Trasformato: {path}")


if __name__ == '__main__':
    main()
