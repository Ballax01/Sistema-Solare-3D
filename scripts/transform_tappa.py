#!/usr/bin/env python3
"""
Trasforma main.cpp delle tappe 10-23:
- sf::RenderWindow -> sf::Window
- Attribute::Default -> Attribute::Core
- Rimuove le funzioni del pannello UI (loadUIFont, wrapText, drawInfoPanel, drawControlsLegend)
- Rimuove le variabili e chiamate correlate
- Rimuove window.pushGLStates/popGLStates e window.setView

Uso: python transform_tappa.py <input_file> <output_file> [--keep-graphics] [--has-legend]
"""

import sys
import re


def remove_function(src: str, func_name: str) -> str:
    """Rimuove una definizione di funzione cercando il blocco {} corrispondente."""
    # Cerca la firma della funzione (non una chiamata)
    pattern = re.compile(
        r'(?m)^(?:bool|void|std::string|int|float)\s+' + re.escape(func_name) + r'\s*\(',
    )
    match = pattern.search(src)
    if not match:
        return src

    start = match.start()
    # Trova la riga precedente (per rimuovere anche eventuali righe vuote sopra)
    line_start = src.rfind('\n', 0, start)
    if line_start == -1:
        line_start = 0
    else:
        line_start += 1  # non includere il \n

    # Trova la prima { della funzione
    brace_open = src.find('{', match.end())
    if brace_open == -1:
        return src

    # Conta le parentesi graffe per trovare la chiusura
    depth = 0
    pos = brace_open
    while pos < len(src):
        if src[pos] == '{':
            depth += 1
        elif src[pos] == '}':
            depth -= 1
            if depth == 0:
                end = pos + 1
                # Consuma le righe vuote successive
                while end < len(src) and src[end] in '\n\r':
                    end += 1
                return src[:line_start] + src[end:]
        pos += 1

    return src


def remove_call(src: str, func_name: str) -> str:
    """Rimuove tutte le chiamate a func_name(...); comprese quelle multiriga."""
    while True:
        # Cerca la chiamata (potrebbe avere indentazione)
        pattern = re.compile(r'(?m)^[ \t]*' + re.escape(func_name) + r'\s*\(')
        match = pattern.search(src)
        if not match:
            break

        line_start = src.rfind('\n', 0, match.start())
        line_start = 0 if line_start == -1 else line_start + 1

        # Conta le parentesi
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
    """Rimuove il blocco if che contiene condition_substr nella condizione."""
    while True:
        pattern = re.compile(r'(?m)^[ \t]*if\s*\([^)]*' + re.escape(condition_substr) + r'[^)]*\)\s*\{?')
        match = pattern.search(src)
        if not match:
            break

        line_start = src.rfind('\n', 0, match.start())
        line_start = 0 if line_start == -1 else line_start + 1

        brace_open = src.find('{', match.end() - 1)
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
    """Rimuove tutti i blocchi window.setView(sf::View(...));"""
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
    """Rimuove le righe che contengono uno dei pattern dati."""
    lines = src.split('\n')
    result = []
    for line in lines:
        if not any(p in line for p in patterns):
            result.append(line)
    return '\n'.join(result)


def transform(src: str, keep_graphics: bool, has_legend: bool) -> str:
    # 1. Rimuove SFML/Graphics.hpp se non serve per sf::Image
    if not keep_graphics:
        src = src.replace('#include <SFML/Graphics.hpp>\n', '')

    # 2. sf::RenderWindow -> sf::Window (tipo e riferimenti)
    src = src.replace('sf::RenderWindow', 'sf::Window')

    # 3. Attribute::Default -> Attribute::Core
    src = src.replace('Attribute::Default', 'Attribute::Core')

    # 4. Rimuovi funzioni del pannello UI
    src = remove_function(src, 'wrapText')
    src = remove_function(src, 'loadUIFont')
    src = remove_function(src, 'drawInfoPanel')
    if has_legend:
        src = remove_function(src, 'drawControlsLegend')

    # 5. Rimuovi righe singole problematiche
    single_line_patterns = [
        'bool uiFontLoaded = loadUIFont',
        'sf::Font uiFont;',
        'window.pushGLStates();',
        'window.popGLStates();',
        'bool showInfoPanel = true;',
        'showInfoPanel = !showInfoPanel;',
        'showInfoPanel ? "Pannello informativo visibile',
        'Pannello informativo visibile',
        'Pannello informativo nascosto',
    ]
    src = remove_lines_containing(src, single_line_patterns)

    # 6. Rimuovi tutti i blocchi window.setView(...)
    src = remove_setview_blocks(src)

    # 7. Rimuovi chiamate al pannello
    src = remove_call(src, 'drawInfoPanel')
    if has_legend:
        src = remove_call(src, 'drawControlsLegend')

    # 8. Rimuovi il blocco if (showInfoPanel) { ... }
    if has_legend:
        src = remove_if_block(src, 'showInfoPanel')

    # 9. Rimuovi la riga 'if (keyPressed->code == sf::Keyboard::Key::I)'
    #    e il suo blocco (che ora è vuoto o contiene solo la riga rimossa)
    #    Cerca il blocco che contiene solo il toggle di showInfoPanel
    #    Dopo la pulizia precedente, potrebbe restare un if vuoto
    src = re.sub(
        r'(?m)^\s*if \(keyPressed->code == sf::Keyboard::Key::I\)\s*\{?\s*\}?\s*\n?',
        '',
        src
    )

    # 10. Pulisci righe vuote multiple consecutive (max 2)
    src = re.sub(r'\n{3,}', '\n\n', src)

    return src


def main():
    if len(sys.argv) < 3:
        print(f"Uso: {sys.argv[0]} <input> <output> [--keep-graphics] [--has-legend]")
        sys.exit(1)

    input_path = sys.argv[1]
    output_path = sys.argv[2]
    keep_graphics = '--keep-graphics' in sys.argv
    has_legend = '--has-legend' in sys.argv

    with open(input_path, 'r', encoding='utf-8') as f:
        src = f.read()

    result = transform(src, keep_graphics, has_legend)

    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(result)

    print(f"Trasformato: {input_path} -> {output_path}")


if __name__ == '__main__':
    main()
