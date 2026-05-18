#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <iostream>

int main()
{
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.majorVersion = 4;
    settings.minorVersion = 1;

    sf::Window window(
        sf::VideoMode({1280, 720}),
        "Sistema Solare 3D - Tappa 01",
        sf::Style::Default,
        sf::State::Windowed,
        settings
    );

    window.setVerticalSyncEnabled(true);

    glClearColor(0.02f, 0.02f, 0.08f, 1.0f);

    while (window.isOpen())
    {
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }

            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
            {
                if (keyPressed->code == sf::Keyboard::Key::Escape)
                {
                    window.close();
                }
            }
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        window.display();
    }

    return 0;
}