#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

struct Vertex
{
    glm::vec3 position;
};

struct Planet
{
    std::string name;
    std::string type;
    std::string description;

    float orbitRadius;
    float size;
    float orbitSpeed;
    float rotationSpeed;

    glm::vec3 color;
};

struct PlanetScreenInfo
{
    int index;
    float x;
    float y;
    float radius;
};

GLuint compileShader(GLenum type, const char* source)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Errore compilazione shader:\n" << infoLog << std::endl;
    }

    return shader;
}

GLuint createShaderProgram()
{
    const char* vertexShaderSource = R"(
        #version 410 core

        layout (location = 0) in vec3 aPos;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        void main()
        {
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 410 core

        out vec4 FragColor;
        uniform vec3 objectColor;

        void main()
        {
            FragColor = vec4(objectColor, 1.0);
        }
    )";

    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    GLint success = 0;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);

    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        std::cerr << "Errore linking shader program:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

void createSphere(
    std::vector<Vertex>& vertices,
    std::vector<unsigned int>& indices,
    float radius,
    int sectorCount,
    int stackCount
)
{
    const float PI = 3.14159265359f;

    for (int i = 0; i <= stackCount; ++i)
    {
        float stackAngle = PI / 2.0f - i * PI / stackCount;
        float xy = radius * std::cos(stackAngle);
        float z = radius * std::sin(stackAngle);

        for (int j = 0; j <= sectorCount; ++j)
        {
            float sectorAngle = j * 2.0f * PI / sectorCount;

            float x = xy * std::cos(sectorAngle);
            float y = xy * std::sin(sectorAngle);

            vertices.push_back({ glm::vec3(x, y, z) });
        }
    }

    for (int i = 0; i < stackCount; ++i)
    {
        int k1 = i * (sectorCount + 1);
        int k2 = k1 + sectorCount + 1;

        for (int j = 0; j < sectorCount; ++j, ++k1, ++k2)
        {
            if (i != 0)
            {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }

            if (i != stackCount - 1)
            {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }
}

void createOrbitCircle(std::vector<glm::vec3>& vertices, int segments)
{
    const float PI = 3.14159265359f;

    for (int i = 0; i < segments; ++i)
    {
        float angle = 2.0f * PI * static_cast<float>(i) / static_cast<float>(segments);
        float x = std::cos(angle);
        float y = std::sin(angle);

        vertices.push_back(glm::vec3(x, y, 0.0f));
    }
}

void drawSphere(
    GLuint shaderProgram,
    GLuint VAO,
    unsigned int indexCount,
    const glm::mat4& model,
    const glm::vec3& color
)
{
    glUseProgram(shaderProgram);

    GLint modelLocation = glGetUniformLocation(shaderProgram, "model");
    GLint colorLocation = glGetUniformLocation(shaderProgram, "objectColor");

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(colorLocation, 1, glm::value_ptr(color));

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void drawOrbit(
    GLuint shaderProgram,
    GLuint orbitVAO,
    int vertexCount,
    float radius,
    const glm::vec3& color
)
{
    glUseProgram(shaderProgram);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(radius, radius, 1.0f));

    GLint modelLocation = glGetUniformLocation(shaderProgram, "model");
    GLint colorLocation = glGetUniformLocation(shaderProgram, "objectColor");

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(colorLocation, 1, glm::value_ptr(color));

    glBindVertexArray(orbitVAO);
    glDrawArrays(GL_LINE_LOOP, 0, vertexCount);
    glBindVertexArray(0);
}

glm::mat4 createCameraView(float yaw, float pitch, float distance)
{
    float yawRad = glm::radians(yaw);
    float pitchRad = glm::radians(pitch);

    glm::vec3 cameraPosition;
    cameraPosition.x = distance * std::cos(pitchRad) * std::sin(yawRad);
    cameraPosition.y = -distance * std::cos(pitchRad) * std::cos(yawRad);
    cameraPosition.z = distance * std::sin(pitchRad);

    return glm::lookAt(
        cameraPosition,
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 0.0f, 1.0f)
    );
}

glm::mat4 createPlanetModel(const Planet& planet, float simulationTime)
{
    glm::mat4 planetModel = glm::mat4(1.0f);

    planetModel = glm::rotate(
        planetModel,
        simulationTime * planet.orbitSpeed,
        glm::vec3(0.0f, 0.0f, 1.0f)
    );

    planetModel = glm::translate(
        planetModel,
        glm::vec3(planet.orbitRadius, 0.0f, 0.0f)
    );

    planetModel = glm::rotate(
        planetModel,
        simulationTime * planet.rotationSpeed,
        glm::vec3(0.0f, 0.0f, 1.0f)
    );

    planetModel = glm::scale(
        planetModel,
        glm::vec3(planet.size)
    );

    return planetModel;
}

void printPlanetInfo(const Planet& planet)
{
    std::cout << "\n==============================\n";
    std::cout << "Pianeta selezionato: " << planet.name << "\n";
    std::cout << "Tipo: " << planet.type << "\n";
    std::cout << "Descrizione: " << planet.description << "\n";
    std::cout << "Distanza orbitale simulata: " << planet.orbitRadius << "\n";
    std::cout << "Dimensione simulata: " << planet.size << "\n";
    std::cout << "==============================\n";
}

int findClickedPlanet(
    int mouseX,
    int mouseY,
    const std::vector<PlanetScreenInfo>& planetScreenInfos
)
{
    int selectedIndex = -1;
    float bestDistance = 1000000.0f;

    for (const PlanetScreenInfo& info : planetScreenInfos)
    {
        float dx = static_cast<float>(mouseX) - info.x;
        float dy = static_cast<float>(mouseY) - info.y;
        float distance = std::sqrt(dx * dx + dy * dy);

        float clickRadius = info.radius;

        if (clickRadius < 18.0f)
        {
            clickRadius = 18.0f;
        }

        if (distance <= clickRadius && distance < bestDistance)
        {
            bestDistance = distance;
            selectedIndex = info.index;
        }
    }

    return selectedIndex;
}

int main()
{
    unsigned int windowWidth = 1280;
    unsigned int windowHeight = 720;

    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.majorVersion = 4;
    settings.minorVersion = 1;
    settings.attributeFlags = sf::ContextSettings::Attribute::Core;

    sf::Window window(
        sf::VideoMode({windowWidth, windowHeight}),
        "Sistema Solare 3D - Tappa 10",
        sf::Style::Default,
        sf::State::Windowed,
        settings
    );

    window.setVerticalSyncEnabled(true);

    if (!gladLoadGL())
    {
        std::cerr << "Errore: impossibile inizializzare glad." << std::endl;
        return -1;
    }

    glViewport(0, 0, static_cast<GLsizei>(windowWidth), static_cast<GLsizei>(windowHeight));
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.01f, 0.01f, 0.04f, 1.0f);

    GLuint shaderProgram = createShaderProgram();

    std::vector<Vertex> sphereVertices;
    std::vector<unsigned int> sphereIndices;

    createSphere(sphereVertices, sphereIndices, 1.0f, 48, 24);

    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        sphereVertices.size() * sizeof(Vertex),
        sphereVertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sphereIndices.size() * sizeof(unsigned int),
        sphereIndices.data(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(0)
    );
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    std::vector<glm::vec3> orbitVertices;
    createOrbitCircle(orbitVertices, 180);

    GLuint orbitVAO = 0;
    GLuint orbitVBO = 0;

    glGenVertexArrays(1, &orbitVAO);
    glGenBuffers(1, &orbitVBO);

    glBindVertexArray(orbitVAO);

    glBindBuffer(GL_ARRAY_BUFFER, orbitVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        orbitVertices.size() * sizeof(glm::vec3),
        orbitVertices.data(),
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(glm::vec3),
        reinterpret_cast<void*>(0)
    );
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        static_cast<float>(windowWidth) / static_cast<float>(windowHeight),
        0.1f,
        100.0f
    );

    glUseProgram(shaderProgram);

    GLint viewLocation = glGetUniformLocation(shaderProgram, "view");
    GLint projectionLocation = glGetUniformLocation(shaderProgram, "projection");

    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(projection));

    std::vector<Planet> planets = {
        {
            "Mercurio",
            "Pianeta roccioso",
            "Il pianeta più vicino al Sole e il più piccolo del Sistema Solare.",
            2.4f,
            0.22f,
            2.40f,
            2.00f,
            glm::vec3(0.55f, 0.55f, 0.55f)
        },
        {
            "Venere",
            "Pianeta roccioso",
            "Pianeta con atmosfera molto densa e temperature superficiali molto elevate.",
            3.4f,
            0.38f,
            1.85f,
            1.20f,
            glm::vec3(0.95f, 0.72f, 0.35f)
        },
        {
            "Terra",
            "Pianeta roccioso",
            "Unico pianeta noto a ospitare vita, con acqua liquida in superficie.",
            4.5f,
            0.42f,
            1.50f,
            3.00f,
            glm::vec3(0.1f, 0.35f, 1.0f)
        },
        {
            "Marte",
            "Pianeta roccioso",
            "Conosciuto come pianeta rosso per la presenza di ossidi di ferro sulla superficie.",
            5.6f,
            0.32f,
            1.20f,
            2.60f,
            glm::vec3(0.9f, 0.25f, 0.1f)
        },
        {
            "Giove",
            "Gigante gassoso",
            "Il pianeta più grande del Sistema Solare, caratterizzato da intense tempeste atmosferiche.",
            7.4f,
            0.90f,
            0.85f,
            3.50f,
            glm::vec3(0.85f, 0.62f, 0.38f)
        },
        {
            "Saturno",
            "Gigante gassoso",
            "Famoso per il suo grande sistema di anelli, che verrà aggiunto in una tappa successiva.",
            9.4f,
            0.78f,
            0.65f,
            3.20f,
            glm::vec3(0.95f, 0.82f, 0.50f)
        },
        {
            "Urano",
            "Gigante ghiacciato",
            "Pianeta dal colore azzurro-verde dovuto alla presenza di metano nell'atmosfera.",
            11.2f,
            0.62f,
            0.48f,
            2.20f,
            glm::vec3(0.45f, 0.85f, 0.85f)
        },
        {
            "Nettuno",
            "Gigante ghiacciato",
            "Il pianeta più lontano dal Sole, caratterizzato da venti molto intensi.",
            13.0f,
            0.60f,
            0.38f,
            2.00f,
            glm::vec3(0.15f, 0.25f, 0.90f)
        }
    };

    float simulationTime = 0.0f;

    float cameraYaw = 0.0f;
    float cameraPitch = 32.0f;
    float cameraDistance = 22.0f;

    int selectedPlanetIndex = -1;

    std::vector<PlanetScreenInfo> planetScreenInfos;

    sf::Clock clock;

    while (window.isOpen())
    {
        float deltaTime = clock.restart().asSeconds();

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

            if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (mousePressed->button == sf::Mouse::Button::Left)
                {
                    int clickedIndex = findClickedPlanet(
                        mousePressed->position.x,
                        mousePressed->position.y,
                        planetScreenInfos
                    );

                    if (clickedIndex != -1)
                    {
                        selectedPlanetIndex = clickedIndex;
                        printPlanetInfo(planets[selectedPlanetIndex]);

                        window.setTitle(
                            "Sistema Solare 3D - Tappa 10 | Selezionato: " +
                            planets[selectedPlanetIndex].name
                        );
                    }
                    else
                    {
                        selectedPlanetIndex = -1;
                        std::cout << "\nNessun pianeta selezionato.\n";
                        window.setTitle("Sistema Solare 3D - Tappa 10");
                    }
                }
            }

            if (const auto* resized = event->getIf<sf::Event::Resized>())
            {
                windowWidth = resized->size.x;
                windowHeight = resized->size.y;

                glViewport(
                    0,
                    0,
                    static_cast<GLsizei>(windowWidth),
                    static_cast<GLsizei>(windowHeight)
                );

                projection = glm::perspective(
                    glm::radians(45.0f),
                    static_cast<float>(windowWidth) / static_cast<float>(windowHeight),
                    0.1f,
                    100.0f
                );

                glUseProgram(shaderProgram);
                glUniformMatrix4fv(
                    projectionLocation,
                    1,
                    GL_FALSE,
                    glm::value_ptr(projection)
                );

            }
        }

        float cameraRotationSpeed = 70.0f;
        float cameraZoomSpeed = 10.0f;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
        {
            cameraYaw -= cameraRotationSpeed * deltaTime;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
        {
            cameraYaw += cameraRotationSpeed * deltaTime;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
        {
            cameraPitch += cameraRotationSpeed * deltaTime;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
        {
            cameraPitch -= cameraRotationSpeed * deltaTime;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
        {
            cameraDistance -= cameraZoomSpeed * deltaTime;
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
        {
            cameraDistance += cameraZoomSpeed * deltaTime;
        }

        if (cameraPitch > 80.0f)
        {
            cameraPitch = 80.0f;
        }

        if (cameraPitch < 5.0f)
        {
            cameraPitch = 5.0f;
        }

        if (cameraDistance < 8.0f)
        {
            cameraDistance = 8.0f;
        }

        if (cameraDistance > 45.0f)
        {
            cameraDistance = 45.0f;
        }

        simulationTime += deltaTime;

        glm::mat4 view = createCameraView(cameraYaw, cameraPitch, cameraDistance);

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glLineWidth(1.5f);

        for (const Planet& planet : planets)
        {
            drawOrbit(
                shaderProgram,
                orbitVAO,
                static_cast<int>(orbitVertices.size()),
                planet.orbitRadius,
                glm::vec3(0.35f, 0.35f, 0.45f)
            );
        }

        glm::mat4 sunModel = glm::mat4(1.0f);
        sunModel = glm::scale(sunModel, glm::vec3(1.35f));

        drawSphere(
            shaderProgram,
            VAO,
            static_cast<unsigned int>(sphereIndices.size()),
            sunModel,
            glm::vec3(1.0f, 0.75f, 0.05f)
        );

        planetScreenInfos.clear();

        glm::vec4 viewport(
            0.0f,
            0.0f,
            static_cast<float>(windowWidth),
            static_cast<float>(windowHeight)
        );

        for (int i = 0; i < static_cast<int>(planets.size()); ++i)
        {
            const Planet& planet = planets[i];

            glm::mat4 planetModel = createPlanetModel(planet, simulationTime);

            glm::vec3 color = planet.color;

            if (i == selectedPlanetIndex)
            {
                color = glm::vec3(1.0f, 1.0f, 0.35f);
            }

            drawSphere(
                shaderProgram,
                VAO,
                static_cast<unsigned int>(sphereIndices.size()),
                planetModel,
                color
            );

            glm::vec4 planetCenterWorld = planetModel * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

            glm::vec3 projectedCenter = glm::project(
                glm::vec3(planetCenterWorld),
                view,
                projection,
                viewport
            );

            float screenX = projectedCenter.x;
            float screenY = static_cast<float>(windowHeight) - projectedCenter.y;

            float approximateRadius = 20.0f + planet.size * 18.0f;

            planetScreenInfos.push_back(
                {
                    i,
                    screenX,
                    screenY,
                    approximateRadius
                }
            );
        }

        window.display();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    glDeleteVertexArrays(1, &orbitVAO);
    glDeleteBuffers(1, &orbitVBO);

    glDeleteProgram(shaderProgram);

    return 0;
}
