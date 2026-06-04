#!/usr/bin/env python3
"""
Trasforma main.cpp di tappa-24 (master):
- sf::RenderWindow -> sf::Window + Core profile
- Rimuove il rendering 2D SFML (pushGLStates, setView, vecchie drawInfoPanel/drawControlsLegend/loadUIFont)
- Mantiene wrapText (serve alla nuova drawInfoPanel)
- Inserisce codice FreeType per rendering testo OpenGL Core
- Riscrive drawInfoPanel e drawControlsLegend con OpenGL

Uso: python fix_tappa24.py <input_file> <output_file>
"""

import sys
import re


# ── Blocchi di codice da iniettare ────────────────────────────────────────────

FREETYPE_INCLUDES = """\
#include <ft2build.h>
#include FT_FREETYPE_H
"""

UI_GLOBALS = """\
// ── Rendering 2D (FreeType + OpenGL Core) ────────────────────────────────────

struct GlyphInfo
{
    int atlasX, atlasY;
    int width, height;
    int bearingX, bearingY;
    int advance;
};

static const int UI_ATLAS_W    = 512;
static const int UI_ATLAS_H    = 256;
static const int UI_FONT_PX    = 18;
static const int UI_FIRST_CHAR = 32;
static const int UI_LAST_CHAR  = 126;

static GlyphInfo s_glyphs[UI_LAST_CHAR - UI_FIRST_CHAR + 1] = {};
static GLuint    s_atlasTexture = 0;
static GLuint    s_uiShader     = 0;
static GLuint    s_uiVAO        = 0;
static GLuint    s_uiVBO        = 0;
static glm::mat4 s_uiProj;
static bool      s_fontOk = false;

"""

UI_SHADER_AND_FUNCTIONS = """\
GLuint createUiShader()
{
    const char* vs = R"(
        #version 410 core
        layout (location = 0) in vec4 vertex;
        uniform mat4 projection;
        out vec2 TexCoords;
        void main() {
            gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
            TexCoords = vertex.zw;
        }
    )";

    const char* fs = R"(
        #version 410 core
        in vec2 TexCoords;
        out vec4 FragColor;
        uniform sampler2D atlas;
        uniform vec4      color;
        uniform bool      useTexture;
        void main() {
            if (useTexture) {
                float a = texture(atlas, TexCoords).r;
                FragColor = vec4(color.rgb, color.a * a);
            } else {
                FragColor = color;
            }
        }
    )";

    GLuint program = glCreateProgram();
    GLuint vert = compileShader(GL_VERTEX_SHADER,   vs);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fs);
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);
    glDeleteShader(vert);
    glDeleteShader(frag);
    return program;
}

bool initFontAtlas()
{
    const std::vector<std::string> fontPaths = {
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/calibri.ttf",
        "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
        "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
        "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
        "/usr/share/fonts/opentype/urw-base35/NimbusSans-Regular.otf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/Library/Fonts/Arial.ttf"
    };

    FT_Library ft;
    if (FT_Init_FreeType(&ft))
    {
        std::cerr << "FreeType: init fallita\n";
        return false;
    }

    FT_Face face = nullptr;
    for (const auto& path : fontPaths)
    {
        if (FT_New_Face(ft, path.c_str(), 0, &face) == 0)
        {
            break;
        }
        face = nullptr;
    }

    if (!face)
    {
        std::cerr << "FreeType: nessun font trovato\n";
        FT_Done_FreeType(ft);
        return false;
    }

    FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(UI_FONT_PX));

    // Crea texture atlas monocromatica
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glGenTextures(1, &s_atlasTexture);
    glBindTexture(GL_TEXTURE_2D, s_atlasTexture);

    std::vector<unsigned char> blank(UI_ATLAS_W * UI_ATLAS_H, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, UI_ATLAS_W, UI_ATLAS_H, 0,
                 GL_RED, GL_UNSIGNED_BYTE, blank.data());

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int penX = 0, penY = 0, rowH = 0;

    for (int c = UI_FIRST_CHAR; c <= UI_LAST_CHAR; ++c)
    {
        if (FT_Load_Char(face, static_cast<FT_ULong>(c), FT_LOAD_RENDER))
            continue;

        FT_GlyphSlot g = face->glyph;
        int bw = static_cast<int>(g->bitmap.width);
        int bh = static_cast<int>(g->bitmap.rows);

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
        gi.bearingX = g->bitmap_left;
        gi.bearingY = g->bitmap_top;
        gi.advance  = static_cast<int>(g->advance.x >> 6);

        if (bw > 0 && bh > 0)
        {
            glTexSubImage2D(GL_TEXTURE_2D, 0, penX, penY, bw, bh,
                            GL_RED, GL_UNSIGNED_BYTE, g->bitmap.buffer);
        }

        penX += bw + 2;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    FT_Done_Face(face);
    FT_Done_FreeType(ft);
    return true;
}

bool initUI(unsigned int winW, unsigned int winH)
{
    s_uiShader = createUiShader();

    if (!initFontAtlas())
    {
        return false;
    }

    // VAO / VBO per quad dinamici 2D
    glGenVertexArrays(1, &s_uiVAO);
    glGenBuffers(1, &s_uiVBO);
    glBindVertexArray(s_uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_uiVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(0));
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    s_uiProj = glm::ortho(0.0f, static_cast<float>(winW),
                          static_cast<float>(winH), 0.0f);

    glUseProgram(s_uiShader);
    glUniformMatrix4fv(
        glGetUniformLocation(s_uiShader, "projection"), 1, GL_FALSE,
        glm::value_ptr(s_uiProj));
    glUniform1i(glGetUniformLocation(s_uiShader, "atlas"), 0);

    s_fontOk = true;
    return true;
}

void updateUiProjection(unsigned int winW, unsigned int winH)
{
    s_uiProj = glm::ortho(0.0f, static_cast<float>(winW),
                          static_cast<float>(winH), 0.0f);
    glUseProgram(s_uiShader);
    glUniformMatrix4fv(
        glGetUniformLocation(s_uiShader, "projection"), 1, GL_FALSE,
        glm::value_ptr(s_uiProj));
}

void renderRect(float x, float y, float w, float h,
                float r, float g, float b, float a)
{
    glUseProgram(s_uiShader);
    glUniform4f(glGetUniformLocation(s_uiShader, "color"), r, g, b, a);
    glUniform1i(glGetUniformLocation(s_uiShader, "useTexture"), 0);
    glUniformMatrix4fv(
        glGetUniformLocation(s_uiShader, "projection"), 1, GL_FALSE,
        glm::value_ptr(s_uiProj));

    float verts[4][4] = {
        { x,     y,     0.0f, 0.0f },
        { x + w, y,     1.0f, 0.0f },
        { x,     y + h, 0.0f, 1.0f },
        { x + w, y + h, 1.0f, 1.0f }
    };

    glBindVertexArray(s_uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_uiVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void renderText(const std::string& text, float x, float y,
                float r, float g, float b, float a)
{
    if (!s_fontOk) return;

    glUseProgram(s_uiShader);
    glUniform4f(glGetUniformLocation(s_uiShader, "color"), r, g, b, a);
    glUniform1i(glGetUniformLocation(s_uiShader, "useTexture"), 1);
    glUniformMatrix4fv(
        glGetUniformLocation(s_uiShader, "projection"), 1, GL_FALSE,
        glm::value_ptr(s_uiProj));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, s_atlasTexture);

    glBindVertexArray(s_uiVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_uiVBO);

    float penX = x;
    float penY = y;

    for (unsigned char c : text)
    {
        if (c == '\\n')
        {
            penX  = x;
            penY += static_cast<float>(UI_FONT_PX) + 4.0f;
            continue;
        }

        if (c < UI_FIRST_CHAR || c > UI_LAST_CHAR) continue;

        const GlyphInfo& gi = s_glyphs[c - UI_FIRST_CHAR];

        float xpos = penX + static_cast<float>(gi.bearingX);
        float ypos = penY + static_cast<float>(UI_FONT_PX - gi.bearingY);
        float fw   = static_cast<float>(gi.width);
        float fh   = static_cast<float>(gi.height);

        float u0 = static_cast<float>(gi.atlasX)             / static_cast<float>(UI_ATLAS_W);
        float v0 = static_cast<float>(gi.atlasY)             / static_cast<float>(UI_ATLAS_H);
        float u1 = static_cast<float>(gi.atlasX + gi.width)  / static_cast<float>(UI_ATLAS_W);
        float v1 = static_cast<float>(gi.atlasY + gi.height) / static_cast<float>(UI_ATLAS_H);

        float verts[4][4] = {
            { xpos,      ypos,      u0, v0 },
            { xpos + fw, ypos,      u1, v0 },
            { xpos,      ypos + fh, u0, v1 },
            { xpos + fw, ypos + fh, u1, v1 }
        };

        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(verts), verts);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        penX += static_cast<float>(gi.advance);
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

"""

NEW_DRAW_INFO_PANEL = """\
void drawInfoPanel(
    const std::vector<Planet>& planets,
    int selectedPlanetIndex,
    const SunInfo& sunInfo,
    bool isSunSelected,
    const MoonInfo& moonInfo,
    bool isMoonSelected,
    unsigned int windowWidth,
    unsigned int windowHeight
)
{
    if (!s_fontOk) return;

    float panelWidth = 460.0f;

    if (windowWidth < 620)
    {
        panelWidth = static_cast<float>(windowWidth) - 32.0f;
    }

    const float panelHeight = 244.0f;
    const float margin      = 18.0f;
    const float x           = margin;
    const float y           = static_cast<float>(windowHeight) - panelHeight - margin;

    // Sfondo pannello
    renderRect(x, y, panelWidth, panelHeight,
               6.0f/255.0f, 10.0f/255.0f, 20.0f/255.0f, 232.0f/255.0f);
    // Bordo
    renderRect(x,       y,                1.0f, panelHeight, 120.0f/255.0f, 148.0f/255.0f, 205.0f/255.0f, 235.0f/255.0f);
    renderRect(x + panelWidth - 1.0f, y, 1.0f, panelHeight, 120.0f/255.0f, 148.0f/255.0f, 205.0f/255.0f, 235.0f/255.0f);
    renderRect(x, y,                       panelWidth, 1.0f, 120.0f/255.0f, 148.0f/255.0f, 205.0f/255.0f, 235.0f/255.0f);
    renderRect(x, y + panelHeight - 1.0f,  panelWidth, 1.0f, 120.0f/255.0f, 148.0f/255.0f, 205.0f/255.0f, 235.0f/255.0f);
    // Accento verticale
    renderRect(x + 10.0f, y + 12.0f, 4.0f, panelHeight - 24.0f,
               255.0f/255.0f, 208.0f/255.0f, 96.0f/255.0f, 210.0f/255.0f);

    std::size_t maxChars = static_cast<std::size_t>((panelWidth - 36.0f) / 8.5f);

    std::string title = "Nessun corpo selezionato";
    std::string body  = wrapText(
        "Clicca sul Sole o su un pianeta per vedere nome, tipo e descrizione.",
        maxChars
    );

    if (isSunSelected)
    {
        title = sunInfo.name;
        body  =
            "Tipo: " + sunInfo.type + "\\n" +
            wrapText(sunInfo.description, maxChars) + "\\n" +
            "Scala dimensione: " + formatSimulationValue(sunInfo.size, "fattore scala");
    }
    else if (isMoonSelected)
    {
        title = moonInfo.name;
        body  =
            "Tipo: " + moonInfo.type + "\\n" +
            wrapText(moonInfo.description, maxChars) + "\\n" +
            "Distanza dalla Terra: " + formatSimulationValue(moonInfo.orbitRadius, "unita scena") + "\\n" +
            "Scala dimensione: " + formatSimulationValue(moonInfo.size, "fattore scala");
    }
    else if (selectedPlanetIndex >= 0 && selectedPlanetIndex < static_cast<int>(planets.size()))
    {
        const Planet& planet = planets[selectedPlanetIndex];
        title = planet.name;
        body  =
            std::string("Caratteristiche principali:\\n") +
            "- Tipo: " + planet.type + "\\n" +
            "- Distanza reale: " + formatRealValue(planet.realDistanceAU, "UA", 3) + "\\n" +
            "- Diametro reale: " + formatRealValue(planet.realDiameterKm, "km", 0) + "\\n" +
            "- Scala scena: " + formatSimulationValue(planet.orbitRadius, "unita") + " / " +
                formatSimulationValue(planet.size, "raggio") + "\\n" +
            "- Eccentricita: " + formatSimulationValue(planet.orbitEccentricity, "simulata") + "\\n" +
            "Descrizione: " + wrapText(planet.description, maxChars);
    }

    renderText(title, x + 24.0f, y + 18.0f,
               255.0f/255.0f, 232.0f/255.0f, 140.0f/255.0f, 1.0f);
    renderText(body,  x + 24.0f, y + 52.0f,
               225.0f/255.0f, 232.0f/255.0f, 245.0f/255.0f, 1.0f);
}

void drawControlsLegend()
{
    if (!s_fontOk) return;

    const float x      = 18.0f;
    const float y      = 18.0f;
    const float width  = 640.0f;
    const float height = 126.0f;

    // Sfondo
    renderRect(x, y, width, height,
               8.0f/255.0f, 12.0f/255.0f, 24.0f/255.0f, 190.0f/255.0f);
    // Bordo
    renderRect(x,           y,               1.0f,  height, 95.0f/255.0f, 120.0f/255.0f, 170.0f/255.0f, 210.0f/255.0f);
    renderRect(x+width-1.f, y,               1.0f,  height, 95.0f/255.0f, 120.0f/255.0f, 170.0f/255.0f, 210.0f/255.0f);
    renderRect(x,           y,               width, 1.0f,   95.0f/255.0f, 120.0f/255.0f, 170.0f/255.0f, 210.0f/255.0f);
    renderRect(x,           y+height-1.0f,   width, 1.0f,   95.0f/255.0f, 120.0f/255.0f, 170.0f/255.0f, 210.0f/255.0f);

    renderText("Comandi", x + 14.0f, y + 14.0f,
               255.0f/255.0f, 232.0f/255.0f, 140.0f/255.0f, 1.0f);

    const std::string controls =
        "Mouse: trascina camera   rotella: zoom   SPACE: pausa   +/-: velocita\\n"
        "C: camera libera/orbitale   WASD: muovi   Q/E: alto/basso\\n"
        "R: reset camera   T: reset tempo   O: orbite   I: interfaccia\\n"
        "Orbitale A/D W/S   F: esci follow   1: Sole   2-9: pianeti   ESC: esci";

    renderText(controls, x + 14.0f, y + 42.0f,
               225.0f/255.0f, 232.0f/255.0f, 245.0f/255.0f, 1.0f);
}

"""

RENDER_PANEL_CALL = """\
        if (showInfoPanel && s_fontOk)
        {
            glDisable(GL_DEPTH_TEST);
            drawControlsLegend();
            drawInfoPanel(
                planets,
                selectedPlanetIndex,
                sunInfo,
                isSunSelected,
                moonInfo,
                isMoonSelected,
                windowWidth,
                windowHeight
            );
            glEnable(GL_DEPTH_TEST);
        }

"""


# ── Rimozione funzioni ────────────────────────────────────────────────────────

def remove_function(src: str, func_name: str) -> str:
    pattern = re.compile(
        r'(?m)^(?:bool|void|std::string|int|float)\s+' + re.escape(func_name) + r'\s*\(',
    )
    match = pattern.search(src)
    if not match:
        return src

    start = match.start()
    line_start = src.rfind('\n', 0, start)
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


def remove_call(src: str, func_name: str) -> str:
    while True:
        pattern = re.compile(r'(?m)^[ \t]*' + re.escape(func_name) + r'\s*\(')
        match = pattern.search(src)
        if not match:
            break

        line_start = src.rfind('\n', 0, match.start())
        line_start = 0 if line_start == -1 else line_start + 1

        depth = 0
        pos = match.start() + len(func_name)
        found_semi = -1
        while pos < len(src):
            if src[pos] == '(':
                depth += 1
            elif src[pos] == ')':
                depth -= 1
                if depth == 0:
                    semi = src.find(';', pos)
                    if semi != -1:
                        found_semi = semi
                    break
            pos += 1

        if found_semi == -1:
            break

        end = found_semi + 1
        while end < len(src) and src[end] in '\n\r':
            end += 1
        src = src[:line_start] + src[end:]

    return src


def remove_if_block(src: str, condition_substr: str) -> str:
    while True:
        pattern = re.compile(
            r'(?m)^[ \t]*if\s*\([^)]*' + re.escape(condition_substr) + r'[^)]*\)\s*\n?\s*\{'
        )
        match = pattern.search(src)
        if not match:
            break

        line_start = src.rfind('\n', 0, match.start())
        line_start = 0 if line_start == -1 else line_start + 1

        brace_open = src.rfind('{', match.start(), match.end())
        if brace_open == -1:
            break

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
                    src = src[:line_start] + src[end:]
                    break
            pos += 1
        else:
            break

    return src


def remove_setview_blocks(src: str) -> str:
    while True:
        match = re.search(r'(?m)^[ \t]*window\.setView\(', src)
        if not match:
            break

        line_start = src.rfind('\n', 0, match.start())
        line_start = 0 if line_start == -1 else line_start + 1

        depth = 0
        pos = match.start() + len('window.setView(') - 1
        found_semi = -1
        while pos < len(src):
            if src[pos] == '(':
                depth += 1
            elif src[pos] == ')':
                depth -= 1
                if depth == 0:
                    semi = src.find(';', pos)
                    if semi != -1:
                        found_semi = semi
                    break
            pos += 1

        if found_semi == -1:
            break

        end = found_semi + 1
        while end < len(src) and src[end] in '\n\r':
            end += 1
        src = src[:line_start] + src[end:]

    return src


def remove_lines_containing(src: str, patterns: list) -> str:
    lines = src.split('\n')
    result = []
    for line in lines:
        if not any(p in line for p in patterns):
            result.append(line)
    return '\n'.join(result)


# ── Trasformazione principale ─────────────────────────────────────────────────

def transform(src: str) -> str:
    # 1. Tipi SFML
    src = src.replace('sf::RenderWindow', 'sf::Window')
    src = src.replace('Attribute::Default', 'Attribute::Core')

    # 2. Rimuovi funzioni SFML 2D (NON wrapText: serve alla nuova drawInfoPanel)
    src = remove_function(src, 'loadUIFont')
    src = remove_function(src, 'drawInfoPanel')
    src = remove_function(src, 'drawControlsLegend')

    # 3. Rimuovi righe singole
    src = remove_lines_containing(src, [
        'bool uiFontLoaded = loadUIFont',
        'sf::Font uiFont;',
        'window.pushGLStates();',
        'window.popGLStates();',
    ])

    # 4. Rimuovi window.setView(...)
    src = remove_setview_blocks(src)

    # 5. Rimuovi chiamata vecchia drawInfoPanel / drawControlsLegend
    #    (dovrebbero essere già sparite, ma per sicurezza)
    src = remove_call(src, 'drawInfoPanel')
    src = remove_call(src, 'drawControlsLegend')

    # 6. Rimuovi blocco if (showInfoPanel) { ... }
    src = remove_if_block(src, 'showInfoPanel')

    # 7. Inserisci include FreeType dopo #include <glad/glad.h>
    glad_include = '#include <glad/glad.h>'
    idx = src.find(glad_include)
    if idx != -1:
        end = src.find('\n', idx) + 1
        src = src[:end] + FREETYPE_INCLUDES + src[end:]

    # 8. Inserisci globali UI dopo l'ultima struct (PlanetScreenInfo)
    planet_screen_struct_end = src.find('};', src.find('struct PlanetScreenInfo'))
    if planet_screen_struct_end != -1:
        end = src.find('\n', planet_screen_struct_end) + 1
        src = src[:end] + '\n' + UI_GLOBALS + src[end:]

    # 9. Inserisci createUiShader + initFontAtlas + initUI + updateUiProjection
    #    + renderRect + renderText DOPO createShaderProgram
    #    Cerca la fine di createShaderProgram
    create_sp_match = re.search(r'(?m)^GLuint createShaderProgram\(\)', src)
    if create_sp_match:
        # Trova la chiusura della funzione
        brace_pos = src.find('{', create_sp_match.start())
        depth = 0
        pos = brace_pos
        while pos < len(src):
            if src[pos] == '{':
                depth += 1
            elif src[pos] == '}':
                depth -= 1
                if depth == 0:
                    end = pos + 1
                    while end < len(src) and src[end] in '\n\r':
                        end += 1
                    src = src[:end] + '\n' + UI_SHADER_AND_FUNCTIONS + src[end:]
                    break
            pos += 1

    # 10. Inserisci nuove drawInfoPanel e drawControlsLegend prima di main()
    main_match = re.search(r'(?m)^int main\(\)', src)
    if main_match:
        src = src[:main_match.start()] + NEW_DRAW_INFO_PANEL + src[main_match.start():]

    # 11. Aggiungi initUI() in main(), dopo gladLoadGL
    glad_check = 'if (!gladLoadGL())'
    idx = src.find(glad_check)
    if idx != -1:
        # Trova la fine del blocco if
        brace_pos = src.find('{', idx)
        depth = 0
        pos = brace_pos
        while pos < len(src):
            if src[pos] == '{':
                depth += 1
            elif src[pos] == '}':
                depth -= 1
                if depth == 0:
                    end = pos + 1
                    # Salta spazi/newline
                    while end < len(src) and src[end] in ' \t\n\r':
                        end += 1
                    # Stiamo subito dopo il blocco if, inseriamo initUI prima di glViewport
                    glviewport = 'glViewport'
                    vp_idx = src.find(glviewport, end)
                    if vp_idx != -1:
                        vp_line_start = src.rfind('\n', 0, vp_idx) + 1
                        src = src[:vp_line_start] + '    initUI(windowWidth, windowHeight);\n\n' + src[vp_line_start:]
                    break
            pos += 1

    # 12. Aggiungi updateUiProjection nel resize handler
    # Dopo setView rimosso, il handler termina con glUniformMatrix4fv(projectionLocation, ...)
    # Marker univoco nel resize handler: 'glm::value_ptr(projection)' DOPO getIf<sf::Event::Resized>
    resize_event = 'getIf<sf::Event::Resized>'
    idx = src.find(resize_event)
    if idx != -1:
        proj_marker = 'glm::value_ptr(projection)'
        proj_idx = src.find(proj_marker, idx)
        if proj_idx != -1:
            semi = src.find(';', proj_idx)
            if semi != -1:
                insert_text = '\n\n                updateUiProjection(windowWidth, windowHeight);'
                src = src[:semi + 1] + insert_text + src[semi + 1:]

    # 13. Inserisci chiamata al pannello prima di window.display()
    display_call = '        window.display();'
    idx = src.rfind(display_call)  # ultima occorrenza (nel loop principale)
    if idx != -1:
        src = src[:idx] + RENDER_PANEL_CALL + src[idx:]

    # 14. Pulisci righe vuote multiple
    src = re.sub(r'\n{4,}', '\n\n\n', src)

    return src


def main():
    if len(sys.argv) < 3:
        print(f"Uso: {sys.argv[0]} <input> <output>")
        sys.exit(1)

    with open(sys.argv[1], 'r', encoding='utf-8') as f:
        src = f.read()

    result = transform(src)

    with open(sys.argv[2], 'w', encoding='utf-8') as f:
        f.write(result)

    print(f"Trasformato tappa-24: {sys.argv[1]} -> {sys.argv[2]}")


if __name__ == '__main__':
    main()
