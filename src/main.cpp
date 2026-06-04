#include <SFML/Window.hpp>
#include <SFML/OpenGL.hpp>
#include <optional>

void drawTriangle()
{
    glBegin(GL_TRIANGLES);

    glColor3f(1.0f, 0.8f, 0.1f);
    glVertex2f(0.0f, 0.6f);

    glColor3f(0.1f, 0.6f, 1.0f);
    glVertex2f(-0.6f, -0.5f);

    glColor3f(0.8f, 0.1f, 1.0f);
    glVertex2f(0.6f, -0.5f);

    glEnd();
}

int main()
{
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.majorVersion = 4;
    settings.minorVersion = 1;

    settings.attributeFlags = sf::ContextSettings::Attribute::Core;
    sf::Window window(
        sf::VideoMode({1280, 720}),
        "Sistema Solare 3D - Tappa 02",
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

        drawTriangle();

        window.display();
    }

    return 0;
}