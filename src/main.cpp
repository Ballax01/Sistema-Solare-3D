#include <SFML/Window.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <SFML/System.hpp>
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

struct Planet
{
    std::string name;
    std::string type;
    std::string description;

    float realDistanceAU;
    float realDiameterKm;
    float orbitRadius;
    float orbitEccentricity;
    float size;
    float orbitSpeed;
    float rotationSpeed;

    glm::vec3 color;
    int textureIndex;
};

struct SunInfo
{
    std::string name;
    std::string type;
    std::string description;
    float size;
};

struct MoonInfo
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
        layout (location = 1) in vec3 aNormal;
        layout (location = 2) in vec2 aTexCoord;

        uniform mat4 model;
        uniform mat4 view;
        uniform mat4 projection;

        out vec3 FragPos;
        out vec3 Normal;
        out vec2 TexCoord;

        void main()
        {
            FragPos = vec3(model * vec4(aPos, 1.0));
            Normal = mat3(transpose(inverse(model))) * aNormal;
            TexCoord = aTexCoord;
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 410 core

        out vec4 FragColor;

        uniform vec3 objectColor;
        uniform vec3 lightPosition;
        uniform vec3 viewPosition;
        uniform int renderMode;
        uniform int useTexture;
        uniform sampler2D diffuseTexture;

        in vec3 FragPos;
        in vec3 Normal;
        in vec2 TexCoord;

        vec3 getBaseColor()
        {
            if (useTexture == 1)
            {
                return texture(diffuseTexture, TexCoord).rgb * objectColor;
            }

            return objectColor;
        }

        void main()
        {
            vec3 normal = normalize(Normal);
            vec3 baseColor = getBaseColor();

            // renderMode 0: oggetti piatti, usato per orbite e linee
            if (renderMode == 0)
            {
                FragColor = vec4(objectColor, 1.0);
                return;
            }

            // renderMode 3: texture piatta per superfici sottili come l'anello di Saturno.
            if (renderMode == 3)
            {
                vec4 textureColor = texture(diffuseTexture, TexCoord);
                FragColor = vec4(textureColor.rgb * objectColor, textureColor.a);
                return;
            }

            // renderMode 6: sfondo stellato non illuminato.
            if (renderMode == 6)
            {
                vec3 starColor = texture(diffuseTexture, TexCoord).rgb;
                FragColor = vec4(starColor * objectColor, 1.0);
                return;
            }

            // renderMode 4: overlay semitrasparente illuminato dalla vista, usato per nuvole/atmosfere.
            if (renderMode == 4)
            {
                vec4 textureColor = texture(diffuseTexture, TexCoord);
                vec3 viewDirection = normalize(viewPosition - FragPos);
                float facing = max(dot(normal, viewDirection), 0.0);
                float brightness = max(max(textureColor.r, textureColor.g), textureColor.b);
                float textureMask = smoothstep(0.08, 0.55, brightness);
                float edgeFade = smoothstep(0.08, 0.55, facing);
                float alpha = 0.28 * textureMask * edgeFade;
                FragColor = vec4(textureColor.rgb * objectColor, alpha);
                return;
            }

            // renderMode 5: texture emissiva tenue, usata per la night map terrestre.
            if (renderMode == 5)
            {
                vec3 lightDirection = normalize(lightPosition - FragPos);
                float nightFactor = pow(1.0 - max(dot(normal, lightDirection), 0.0), 1.45);
                vec4 textureColor = texture(diffuseTexture, TexCoord);
                FragColor = vec4(textureColor.rgb * objectColor * nightFactor * 1.35, nightFactor * 0.92);
                return;
            }

            // renderMode 2: Sole con ombreggiatura fittizia per renderlo piu 3D,
            // mantenendo un aspetto luminoso e leggibile.
            if (renderMode == 2)
            {
                vec3 viewDirection = normalize(viewPosition - FragPos);
                float frontLight = max(dot(normal, viewDirection), 0.0);
                float rim = pow(1.0 - frontLight, 2.0);

                vec3 core = baseColor * (0.62 + 0.45 * frontLight);
                vec3 warmGlow = vec3(1.0, 0.38, 0.04) * 0.20;
                vec3 rimGlow = vec3(1.0, 0.55, 0.08) * rim * 0.35;

                FragColor = vec4(core + warmGlow + rimGlow, 1.0);
                return;
            }

            // renderMode 1: illuminazione Phong completa per i pianeti
            vec3 lightDirection = normalize(lightPosition - FragPos);
            vec3 viewDirection = normalize(viewPosition - FragPos);
            vec3 reflectDirection = reflect(-lightDirection, normal);

            float ambientStrength = 0.16;
            float diffuseStrength = max(dot(normal, lightDirection), 0.0);
            float specularStrength = 0.30;
            float shininess = 32.0;
            float specularFactor = pow(max(dot(viewDirection, reflectDirection), 0.0), shininess);

            vec3 ambient = ambientStrength * baseColor;
            vec3 diffuse = diffuseStrength * baseColor;
            vec3 specular = specularStrength * specularFactor * vec3(1.0);

            FragColor = vec4(ambient + diffuse + specular, 1.0);
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
            glm::vec3 position(x, y, z);
            float u = static_cast<float>(j) / static_cast<float>(sectorCount);
            float v = static_cast<float>(i) / static_cast<float>(stackCount);

            vertices.push_back({ position, glm::normalize(position), glm::vec2(u, v) });
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

void createRing(
    std::vector<Vertex>& vertices,
    std::vector<unsigned int>& indices,
    float innerRadius,
    float outerRadius,
    int segments
)
{
    const float PI = 3.14159265359f;

    for (int i = 0; i <= segments; ++i)
    {
        float angle = 2.0f * PI * static_cast<float>(i) / static_cast<float>(segments);
        float cosAngle = std::cos(angle);
        float sinAngle = std::sin(angle);

        vertices.push_back({
            glm::vec3(innerRadius * cosAngle, innerRadius * sinAngle, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f),
            glm::vec2(0.0f, 0.5f)
        });

        vertices.push_back({
            glm::vec3(outerRadius * cosAngle, outerRadius * sinAngle, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f),
            glm::vec2(1.0f, 0.5f)
        });
    }

    for (int i = 0; i < segments; ++i)
    {
        unsigned int innerCurrent = static_cast<unsigned int>(i * 2);
        unsigned int outerCurrent = innerCurrent + 1;
        unsigned int innerNext = innerCurrent + 2;
        unsigned int outerNext = innerCurrent + 3;

        indices.push_back(innerCurrent);
        indices.push_back(outerCurrent);
        indices.push_back(innerNext);

        indices.push_back(innerNext);
        indices.push_back(outerCurrent);
        indices.push_back(outerNext);
    }
}

float pseudoNoise(float x, float y, float seed)
{
    float value = std::sin(x * 12.9898f + y * 78.233f + seed * 37.719f) * 43758.5453f;
    return value - std::floor(value);
}

unsigned char toByte(float value)
{
    if (value < 0.0f)
    {
        value = 0.0f;
    }

    if (value > 1.0f)
    {
        value = 1.0f;
    }

    return static_cast<unsigned char>(value * 255.0f);
}

GLuint createProceduralTexture(int textureType)
{
    const int width = 256;
    const int height = 128;
    const float PI = 3.14159265359f;

    std::vector<unsigned char> pixels(width * height * 3);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            float u = static_cast<float>(x) / static_cast<float>(width - 1);
            float v = static_cast<float>(y) / static_cast<float>(height - 1);
            float n = pseudoNoise(u * 16.0f, v * 16.0f, static_cast<float>(textureType) + 1.0f);

            glm::vec3 color(1.0f);

            if (textureType == 0) // Sole
            {
                float bands = 0.5f + 0.5f * std::sin((u * 18.0f + v * 8.0f) * PI);
                color = glm::vec3(
                    1.0f,
                    0.45f + 0.35f * bands,
                    0.04f + 0.08f * n
                );
            }
            else if (textureType == 1) // Mercurio
            {
                float shade = 0.35f + 0.35f * n;
                color = glm::vec3(shade, shade, shade);
            }
            else if (textureType == 2) // Venere
            {
                float clouds = 0.5f + 0.5f * std::sin((u * 12.0f + v * 18.0f) * PI);
                color = glm::vec3(
                    0.85f + 0.12f * clouds,
                    0.62f + 0.12f * n,
                    0.30f + 0.10f * clouds
                );
            }
            else if (textureType == 3) // Terra
            {
                float continents =
                    std::sin(u * 24.0f * PI + std::sin(v * 9.0f * PI)) +
                    std::cos(v * 15.0f * PI + u * 5.0f);

                if (continents > 0.55f)
                {
                    color = glm::vec3(0.12f + 0.15f * n, 0.45f + 0.25f * n, 0.12f);
                }
                else if (continents > 0.25f)
                {
                    color = glm::vec3(0.65f, 0.58f, 0.30f);
                }
                else
                {
                    color = glm::vec3(0.05f, 0.20f + 0.10f * n, 0.75f + 0.15f * n);
                }

                if (pseudoNoise(u * 32.0f, v * 32.0f, 9.0f) > 0.84f)
                {
                    color = glm::vec3(0.90f, 0.94f, 1.0f);
                }
            }
            else if (textureType == 4) // Marte
            {
                float shade = 0.45f + 0.25f * n;
                color = glm::vec3(0.75f * shade + 0.20f, 0.22f * shade + 0.08f, 0.08f);
            }
            else if (textureType == 5) // Giove
            {
                float bands = 0.5f + 0.5f * std::sin(v * 34.0f * PI + n * 2.0f);
                color = glm::mix(
                    glm::vec3(0.72f, 0.48f, 0.30f),
                    glm::vec3(0.95f, 0.82f, 0.60f),
                    bands
                );
            }
            else if (textureType == 6) // Saturno
            {
                float bands = 0.5f + 0.5f * std::sin(v * 26.0f * PI);
                color = glm::mix(
                    glm::vec3(0.75f, 0.58f, 0.34f),
                    glm::vec3(0.98f, 0.86f, 0.55f),
                    bands
                );
            }
            else if (textureType == 7) // Urano
            {
                float bands = 0.5f + 0.5f * std::sin(v * 12.0f * PI);
                color = glm::vec3(0.35f + 0.08f * bands, 0.78f + 0.12f * bands, 0.82f + 0.10f * n);
            }
            else if (textureType == 8) // Nettuno
            {
                float bands = 0.5f + 0.5f * std::sin((v * 16.0f + u * 2.0f) * PI);
                color = glm::vec3(0.06f + 0.05f * n, 0.16f + 0.08f * bands, 0.70f + 0.20f * bands);
            }
            else if (textureType == 9) // Luna
            {
                float craters = pseudoNoise(u * 42.0f, v * 42.0f, 13.0f);
                float shade = 0.48f + 0.28f * n;

                if (craters > 0.82f)
                {
                    shade *= 0.62f;
                }

                color = glm::vec3(shade, shade, shade * 0.96f);
            }

            int offset = (y * width + x) * 3;
            pixels[offset] = toByte(color.r);
            pixels[offset + 1] = toByte(color.g);
            pixels[offset + 2] = toByte(color.b);
        }
    }

    GLuint textureID = 0;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        width,
        height,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        pixels.data()
    );

    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    return textureID;
}

GLuint loadTextureFromFile(const std::string& path, int fallbackTextureType)
{
    std::vector<std::string> candidatePaths = {
        path, "../" + path, "../../" + path, "../../../" + path
    };

    int width = 0, height = 0, channels = 0;
    unsigned char* pixels = nullptr;
    std::string loadedPath;

    stbi_set_flip_vertically_on_load(0);

    for (const std::string& candidatePath : candidatePaths)
    {
        pixels = stbi_load(candidatePath.c_str(), &width, &height, &channels, 4);
        if (pixels) { loadedPath = candidatePath; break; }
    }

    if (!pixels)
    {
        std::cerr << "Impossibile caricare la texture: " << path
                  << ". Uso texture procedurale di fallback.\n";
        return createProceduralTexture(fallbackTextureType);
    }

    GLuint textureID = 0;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
        static_cast<GLsizei>(width), static_cast<GLsizei>(height),
        0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    stbi_image_free(pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    std::cout << "Texture caricata: " << loadedPath << "\n";
    return textureID;
}

void drawSphere(
    GLuint shaderProgram,
    GLuint VAO,
    unsigned int indexCount,
    const glm::mat4& model,
    const glm::vec3& color,
    const glm::vec3& lightPosition,
    const glm::vec3& viewPosition,
    int renderMode,
    GLuint textureID,
    bool useTexture
)
{
    glUseProgram(shaderProgram);

    GLint modelLocation = glGetUniformLocation(shaderProgram, "model");
    GLint colorLocation = glGetUniformLocation(shaderProgram, "objectColor");
    GLint lightPositionLocation = glGetUniformLocation(shaderProgram, "lightPosition");
    GLint viewPositionLocation = glGetUniformLocation(shaderProgram, "viewPosition");
    GLint renderModeLocation = glGetUniformLocation(shaderProgram, "renderMode");
    GLint useTextureLocation = glGetUniformLocation(shaderProgram, "useTexture");
    GLint diffuseTextureLocation = glGetUniformLocation(shaderProgram, "diffuseTexture");

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(colorLocation, 1, glm::value_ptr(color));
    glUniform3fv(lightPositionLocation, 1, glm::value_ptr(lightPosition));
    glUniform3fv(viewPositionLocation, 1, glm::value_ptr(viewPosition));
    glUniform1i(renderModeLocation, renderMode);
    glUniform1i(useTextureLocation, useTexture ? 1 : 0);

    if (useTexture)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glUniform1i(diffuseTextureLocation, 0);
    }

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    if (useTexture)
    {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void drawOrbit(
    GLuint shaderProgram,
    GLuint orbitVAO,
    int vertexCount,
    float radius,
    float eccentricity,
    const glm::vec3& color
)
{
    glUseProgram(shaderProgram);

    float semiMinorAxis = radius * std::sqrt(1.0f - eccentricity * eccentricity);

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(-radius * eccentricity, 0.0f, 0.0f));
    model = glm::scale(model, glm::vec3(radius, semiMinorAxis, 1.0f));

    GLint modelLocation = glGetUniformLocation(shaderProgram, "model");
    GLint colorLocation = glGetUniformLocation(shaderProgram, "objectColor");
    GLint renderModeLocation = glGetUniformLocation(shaderProgram, "renderMode");
    GLint useTextureLocation = glGetUniformLocation(shaderProgram, "useTexture");

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(colorLocation, 1, glm::value_ptr(color));
    glUniform1i(renderModeLocation, 0);
    glUniform1i(useTextureLocation, 0);

    glBindVertexArray(orbitVAO);
    glDrawArrays(GL_LINE_LOOP, 0, vertexCount);
    glBindVertexArray(0);
}

void drawOrbitWithModel(
    GLuint shaderProgram,
    GLuint orbitVAO,
    int vertexCount,
    const glm::mat4& model,
    const glm::vec3& color
)
{
    glUseProgram(shaderProgram);

    GLint modelLocation = glGetUniformLocation(shaderProgram, "model");
    GLint colorLocation = glGetUniformLocation(shaderProgram, "objectColor");
    GLint renderModeLocation = glGetUniformLocation(shaderProgram, "renderMode");
    GLint useTextureLocation = glGetUniformLocation(shaderProgram, "useTexture");

    glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(colorLocation, 1, glm::value_ptr(color));
    glUniform1i(renderModeLocation, 0);
    glUniform1i(useTextureLocation, 0);

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

glm::vec3 createCameraPosition(float yaw, float pitch, float distance)
{
    float yawRad = glm::radians(yaw);
    float pitchRad = glm::radians(pitch);

    glm::vec3 cameraPosition;
    cameraPosition.x = distance * std::cos(pitchRad) * std::sin(yawRad);
    cameraPosition.y = -distance * std::cos(pitchRad) * std::cos(yawRad);
    cameraPosition.z = distance * std::sin(pitchRad);

    return cameraPosition;
}

glm::mat4 createCameraViewAroundTarget(
    float yaw,
    float pitch,
    float distance,
    const glm::vec3& target
)
{
    return glm::lookAt(
        target + createCameraPosition(yaw, pitch, distance),
        target,
        glm::vec3(0.0f, 0.0f, 1.0f)
    );
}

glm::vec3 createFreeCameraForward(float yaw, float pitch)
{
    float yawRad = glm::radians(yaw);
    float pitchRad = glm::radians(pitch);

    glm::vec3 forward;
    forward.x = std::cos(pitchRad) * std::sin(yawRad);
    forward.y = -std::cos(pitchRad) * std::cos(yawRad);
    forward.z = std::sin(pitchRad);

    return glm::normalize(forward);
}

glm::vec3 createFreeCameraRight(float yaw)
{
    glm::vec3 forward = createFreeCameraForward(yaw, 0.0f);
    return glm::normalize(glm::cross(forward, glm::vec3(0.0f, 0.0f, 1.0f)));
}

glm::mat4 createFreeCameraView(
    const glm::vec3& cameraPosition,
    float yaw,
    float pitch
)
{
    glm::vec3 forward = createFreeCameraForward(yaw, pitch);

    return glm::lookAt(
        cameraPosition,
        cameraPosition + forward,
        glm::vec3(0.0f, 0.0f, 1.0f)
    );
}

glm::mat4 createPlanetOrbitModel(const Planet& planet, float simulationTime)
{
    float angle = simulationTime * planet.orbitSpeed;
    float semiMinorAxis = planet.orbitRadius * std::sqrt(
        1.0f - planet.orbitEccentricity * planet.orbitEccentricity
    );

    glm::vec3 orbitPosition(
        planet.orbitRadius * (std::cos(angle) - planet.orbitEccentricity),
        semiMinorAxis * std::sin(angle),
        0.0f
    );

    glm::mat4 orbitModel = glm::mat4(1.0f);
    orbitModel = glm::translate(orbitModel, orbitPosition);

    return orbitModel;
}

glm::mat4 createPlanetModel(const Planet& planet, float simulationTime)
{
    glm::mat4 planetModel = createPlanetOrbitModel(planet, simulationTime);

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

glm::mat4 createMoonModel(
    const Planet& earth,
    const MoonInfo& moon,
    float simulationTime
)
{
    glm::mat4 moonModel = createPlanetOrbitModel(earth, simulationTime);

    moonModel = glm::rotate(
        moonModel,
        simulationTime * moon.orbitSpeed,
        glm::vec3(0.0f, 0.0f, 1.0f)
    );

    moonModel = glm::translate(
        moonModel,
        glm::vec3(moon.orbitRadius, 0.0f, 0.0f)
    );

    moonModel = glm::rotate(
        moonModel,
        simulationTime * moon.rotationSpeed,
        glm::vec3(0.0f, 0.0f, 1.0f)
    );

    moonModel = glm::scale(
        moonModel,
        glm::vec3(moon.size)
    );

    return moonModel;
}

std::string formatSimulationValue(float value, const std::string& unit)
{
    std::ostringstream stream;
    stream.setf(std::ios::fixed);
    stream.precision(2);
    stream << value << " " << unit;
    return stream.str();
}

std::string formatRealValue(float value, const std::string& unit, int precision)
{
    std::ostringstream stream;
    stream.setf(std::ios::fixed);
    stream.precision(precision);
    stream << value << " " << unit;
    return stream.str();
}

void printPlanetInfo(const Planet& planet)
{
    std::cout << "\n==============================\n";
    std::cout << "Pianeta selezionato: " << planet.name << "\n";
    std::cout << "Tipo: " << planet.type << "\n";
    std::cout << "Descrizione: " << planet.description << "\n";
    std::cout << "Distanza media reale dal Sole: "
              << formatRealValue(planet.realDistanceAU, "UA", 3) << "\n";
    std::cout << "Diametro reale: "
              << formatRealValue(planet.realDiameterKm, "km", 0) << "\n";
    std::cout << "Distanza orbitale simulata: "
              << formatSimulationValue(planet.orbitRadius, "unita scena") << "\n";
    std::cout << "Scala dimensione: "
              << formatSimulationValue(planet.size, "fattore scala") << "\n";
    std::cout << "==============================\n";
}

void printSunInfo(const SunInfo& sun)
{
    std::cout << "\n==============================\n";
    std::cout << "Corpo selezionato: " << sun.name << "\n";
    std::cout << "Tipo: " << sun.type << "\n";
    std::cout << "Descrizione: " << sun.description << "\n";
    std::cout << "Scala dimensione: "
              << formatSimulationValue(sun.size, "fattore scala") << "\n";
    std::cout << "==============================\n";
}

void printMoonInfo(const MoonInfo& moon)
{
    std::cout << "\n==============================\n";
    std::cout << "Corpo selezionato: " << moon.name << "\n";
    std::cout << "Tipo: " << moon.type << "\n";
    std::cout << "Descrizione: " << moon.description << "\n";
    std::cout << "Distanza orbitale dalla Terra simulata: "
              << formatSimulationValue(moon.orbitRadius, "unita scena") << "\n";
    std::cout << "Scala dimensione: "
              << formatSimulationValue(moon.size, "fattore scala") << "\n";
    std::cout << "==============================\n";
}

void selectPlanet(
    int planetIndex,
    int& selectedPlanetIndex,
    const std::vector<Planet>& planets,
    sf::Window& window
)
{
    if (planetIndex >= 0 && planetIndex < static_cast<int>(planets.size()))
    {
        selectedPlanetIndex = planetIndex;
        printPlanetInfo(planets[selectedPlanetIndex]);

        window.setTitle(
            "Sistema Solare 3D - Tappa 21 | Selezionato: " +
            planets[selectedPlanetIndex].name
        );
    }
}

void startFollowingSelectedPlanet(
    int selectedPlanetIndex,
    const std::vector<Planet>& planets,
    bool& isFollowingPlanet,
    float& cameraYaw,
    float& cameraPitch,
    float& cameraDistance,
    float& previousCameraYaw,
    float& previousCameraPitch,
    float& previousCameraDistance
)
{
    if (selectedPlanetIndex < 0 || selectedPlanetIndex >= static_cast<int>(planets.size()))
    {
        return;
    }

    if (!isFollowingPlanet)
    {
        previousCameraYaw = cameraYaw;
        previousCameraPitch = cameraPitch;
        previousCameraDistance = cameraDistance;
    }

    isFollowingPlanet = true;
    cameraDistance = planets[selectedPlanetIndex].size * 5.0f + 2.6f;

    if (cameraDistance < 4.0f)
    {
        cameraDistance = 4.0f;
    }

    if (cameraDistance > 8.0f)
    {
        cameraDistance = 8.0f;
    }

    std::cout << "Camera in inseguimento su " << planets[selectedPlanetIndex].name << ". Premi F per tornare alla vista precedente.\n";
}

void stopFollowingPlanet(
    bool& isFollowingPlanet,
    float& cameraYaw,
    float& cameraPitch,
    float& cameraDistance,
    float previousCameraYaw,
    float previousCameraPitch,
    float previousCameraDistance
)
{
    if (!isFollowingPlanet)
    {
        return;
    }

    isFollowingPlanet = false;
    cameraYaw = previousCameraYaw;
    cameraPitch = previousCameraPitch;
    cameraDistance = previousCameraDistance;

    std::cout << "Ritorno alla vista precedente.\n";
}

void selectSun(
    int& selectedPlanetIndex,
    bool& isSunSelected,
    bool& isMoonSelected,
    bool& isFollowingPlanet,
    float& cameraYaw,
    float& cameraPitch,
    float& cameraDistance,
    float& previousCameraYaw,
    float& previousCameraPitch,
    float& previousCameraDistance,
    const SunInfo& sunInfo,
    sf::Window& window
)
{
    if (!isFollowingPlanet)
    {
        previousCameraYaw = cameraYaw;
        previousCameraPitch = cameraPitch;
        previousCameraDistance = cameraDistance;
    }

    selectedPlanetIndex = -1;
    isSunSelected = true;
    isMoonSelected = false;
    isFollowingPlanet = true;
    cameraDistance = 5.5f;

    printSunInfo(sunInfo);
    window.setTitle("Sistema Solare 3D - Tappa 21 | Selezionato: Sole");
}

int numberFromKey(sf::Keyboard::Key key)
{
    if (key >= sf::Keyboard::Key::Num1 && key <= sf::Keyboard::Key::Num9)
    {
        return static_cast<int>(key) - static_cast<int>(sf::Keyboard::Key::Num1) + 1;
    }

    if (key >= sf::Keyboard::Key::Numpad1 && key <= sf::Keyboard::Key::Numpad9)
    {
        return static_cast<int>(key) - static_cast<int>(sf::Keyboard::Key::Numpad1) + 1;
    }

    return -1;
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

bool isClickOnScreenInfo(
    int mouseX,
    int mouseY,
    const PlanetScreenInfo& screenInfo
)
{
    float dx = static_cast<float>(mouseX) - screenInfo.x;
    float dy = static_cast<float>(mouseY) - screenInfo.y;
    float distance = std::sqrt(dx * dx + dy * dy);

    return distance <= screenInfo.radius;
}

void handleCelestialBodyClick(
    int mouseX,
    int mouseY,
    PlanetScreenInfo& sunScreenInfo,
    PlanetScreenInfo& moonScreenInfo,
    const std::vector<PlanetScreenInfo>& planetScreenInfos,
    const SunInfo& sunInfo,
    const MoonInfo& moonInfo,
    const std::vector<Planet>& planets,
    int& selectedPlanetIndex,
    bool& isSunSelected,
    bool& isMoonSelected,
    bool& isFollowingPlanet,
    float& cameraYaw,
    float& cameraPitch,
    float& cameraDistance,
    float& previousCameraYaw,
    float& previousCameraPitch,
    float& previousCameraDistance,
    sf::Window& window
)
{
    if (isClickOnScreenInfo(mouseX, mouseY, sunScreenInfo))
    {
        selectSun(
            selectedPlanetIndex,
            isSunSelected,
            isMoonSelected,
            isFollowingPlanet,
            cameraYaw,
            cameraPitch,
            cameraDistance,
            previousCameraYaw,
            previousCameraPitch,
            previousCameraDistance,
            sunInfo,
            window
        );
        return;
    }

    if (isClickOnScreenInfo(mouseX, mouseY, moonScreenInfo))
    {
        selectedPlanetIndex = -1;
        isSunSelected = false;
        isMoonSelected = true;

        stopFollowingPlanet(
            isFollowingPlanet,
            cameraYaw,
            cameraPitch,
            cameraDistance,
            previousCameraYaw,
            previousCameraPitch,
            previousCameraDistance
        );

        printMoonInfo(moonInfo);
        window.setTitle("Sistema Solare 3D - Tappa 21 | Selezionato: Luna");
        return;
    }

    int clickedIndex = findClickedPlanet(mouseX, mouseY, planetScreenInfos);

    if (clickedIndex != -1)
    {
        isSunSelected = false;
        isMoonSelected = false;
        selectPlanet(clickedIndex, selectedPlanetIndex, planets, window);
        startFollowingSelectedPlanet(
            selectedPlanetIndex,
            planets,
            isFollowingPlanet,
            cameraYaw,
            cameraPitch,
            cameraDistance,
            previousCameraYaw,
            previousCameraPitch,
            previousCameraDistance
        );
        return;
    }

    selectedPlanetIndex = -1;
    isSunSelected = false;
    isMoonSelected = false;
    stopFollowingPlanet(
        isFollowingPlanet,
        cameraYaw,
        cameraPitch,
        cameraDistance,
        previousCameraYaw,
        previousCameraPitch,
        previousCameraDistance
    );
    std::cout << "\nNessun corpo celeste selezionato.\n";
    window.setTitle("Sistema Solare 3D - Tappa 21");
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
        "Sistema Solare 3D - Tappa 21",
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
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.01f, 0.01f, 0.04f, 1.0f);

    GLuint shaderProgram = createShaderProgram();

    GLuint sunTexture = loadTextureFromFile("assets/textures/sun.jpg", 0);
    GLuint moonTexture = loadTextureFromFile("assets/textures/moon.jpg", 9);
    GLuint saturnRingTexture = loadTextureFromFile("assets/textures/saturn_ring.png", 6);
    GLuint earthCloudsTexture = loadTextureFromFile("assets/textures/earth_clouds.jpg", 3);
    GLuint earthNightTexture = loadTextureFromFile("assets/textures/earth_nightmap.jpg", 3);
    GLuint venusAtmosphereTexture = loadTextureFromFile("assets/textures/venus_atmosphere.jpg", 2);
    GLuint starsTexture = loadTextureFromFile("assets/textures/stars_milky_way.jpg", 8);
    std::vector<GLuint> planetTextures = {
        loadTextureFromFile("assets/textures/mercury.jpg", 1), // Mercurio
        loadTextureFromFile("assets/textures/venus.jpg", 2),   // Venere
        loadTextureFromFile("assets/textures/earth.jpg", 3),   // Terra
        loadTextureFromFile("assets/textures/mars.jpg", 4),    // Marte
        loadTextureFromFile("assets/textures/jupiter.jpg", 5), // Giove
        loadTextureFromFile("assets/textures/saturn.jpg", 6),  // Saturno
        loadTextureFromFile("assets/textures/uranus.jpg", 7),  // Urano
        loadTextureFromFile("assets/textures/neptune.jpg", 8)  // Nettuno
    };

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

    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(sizeof(glm::vec3))
    );
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(sizeof(glm::vec3) * 2)
    );
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    std::vector<Vertex> ringVertices;
    std::vector<unsigned int> ringIndices;
    createRing(ringVertices, ringIndices, 0.95f, 1.55f, 128);

    GLuint ringVAO = 0;
    GLuint ringVBO = 0;
    GLuint ringEBO = 0;

    glGenVertexArrays(1, &ringVAO);
    glGenBuffers(1, &ringVBO);
    glGenBuffers(1, &ringEBO);

    glBindVertexArray(ringVAO);

    glBindBuffer(GL_ARRAY_BUFFER, ringVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        ringVertices.size() * sizeof(Vertex),
        ringVertices.data(),
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ringEBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        ringIndices.size() * sizeof(unsigned int),
        ringIndices.data(),
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

    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(sizeof(glm::vec3))
    );
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(
        2,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(Vertex),
        reinterpret_cast<void*>(sizeof(glm::vec3) * 2)
    );
    glEnableVertexAttribArray(2);

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

    SunInfo sunInfo = {
        "Sole",
        "Stella",
        "Il Sole e la stella al centro del Sistema Solare. La sua energia deriva da reazioni di fusione nucleare e illumina e riscalda i pianeti che orbitano attorno ad esso.",
        1.35f
    };

    MoonInfo moonInfo = {
        "Luna",
        "Satellite naturale",
        "La Luna e il satellite naturale della Terra. Orbita attorno al nostro pianeta e influenza fenomeni come le maree.",
        0.85f,
        0.12f,
        5.20f,
        1.80f,
        glm::vec3(0.80f, 0.80f, 0.78f)
    };

    std::vector<Planet> planets = {
        {
            "Mercurio",
            "Pianeta roccioso",
            "Piccolo, roccioso e quasi privo di atmosfera, passa da temperature estreme tra giorno e notte.",
            0.387f,
            4879.0f,
            3.05f,
            0.16f,
            0.22f,
            2.40f,
            2.00f,
            glm::vec3(0.55f, 0.55f, 0.55f),
            0
        },
        {
            "Venere",
            "Pianeta roccioso",
            "Ha un'atmosfera densissima ricca di anidride carbonica e nubi che intrappolano calore.",
            0.723f,
            12104.0f,
            4.05f,
            0.02f,
            0.38f,
            1.85f,
            1.20f,
            glm::vec3(0.95f, 0.72f, 0.35f),
            1
        },
        {
            "Terra",
            "Pianeta roccioso",
            "Ospita oceani, atmosfera respirabile e una magnetosfera che contribuisce a proteggerla.",
            1.000f,
            12742.0f,
            5.10f,
            0.03f,
            0.42f,
            1.50f,
            3.00f,
            glm::vec3(0.1f, 0.35f, 1.0f),
            2
        },
        {
            "Marte",
            "Pianeta roccioso",
            "Il colore rosso deriva dagli ossidi di ferro; conserva tracce di antichi fiumi e vulcani enormi.",
            1.524f,
            6779.0f,
            6.55f,
            0.09f,
            0.32f,
            1.20f,
            2.60f,
            glm::vec3(0.9f, 0.25f, 0.1f),
            3
        },
        {
            "Giove",
            "Gigante gassoso",
            "Gigante dominato da idrogeno ed elio, con bande atmosferiche e tempeste persistenti.",
            5.203f,
            139820.0f,
            9.10f,
            0.05f,
            0.90f,
            0.85f,
            3.50f,
            glm::vec3(0.85f, 0.62f, 0.38f),
            4
        },
        {
            "Saturno",
            "Gigante gassoso",
            "Celebre per il sistema di anelli ghiacciati e per la bassa densita media.",
            9.537f,
            116460.0f,
            13.00f,
            0.06f,
            0.78f,
            0.65f,
            3.20f,
            glm::vec3(0.95f, 0.82f, 0.50f),
            5
        },
        {
            "Urano",
            "Gigante ghiacciato",
            "Il metano gli dona il colore azzurro-verde; il suo asse e inclinato in modo estremo.",
            19.191f,
            50724.0f,
            20.80f,
            0.08f,
            0.62f,
            0.48f,
            2.20f,
            glm::vec3(0.45f, 0.85f, 0.85f),
            6
        },
        {
            "Nettuno",
            "Gigante ghiacciato",
            "Mondo blu e freddo, con venti tra i piu rapidi osservati nel Sistema Solare.",
            30.070f,
            49244.0f,
            30.50f,
            0.02f,
            0.60f,
            0.38f,
            2.00f,
            glm::vec3(0.15f, 0.25f, 0.90f),
            7
        }
    };

    float simulationTime = 0.0f;

    float cameraYaw = 0.0f;
    float cameraPitch = 32.0f;
    float cameraDistance = 22.0f;
    bool isFreeCamera = false;
    glm::vec3 freeCameraPosition(0.0f, -18.0f, 8.0f);
    float freeCameraYaw = 0.0f;
    float freeCameraPitch = 18.0f;

    int selectedPlanetIndex = -1;
    bool isSunSelected = false;
    bool isMoonSelected = false;
    bool isPaused = false;
    const float defaultTimeScale = 0.45f;
    float timeScale = defaultTimeScale;
    bool showOrbits = true;
    bool isFollowingPlanet = false;
    bool isMouseDragging = false;
    bool hasMouseDragged = false;
    sf::Vector2i mousePressPosition;
    sf::Vector2i previousMousePosition;
    float previousCameraYaw = cameraYaw;
    float previousCameraPitch = cameraPitch;
    float previousCameraDistance = cameraDistance;

    std::vector<PlanetScreenInfo> planetScreenInfos;

    PlanetScreenInfo sunScreenInfo = {
        -100,
        static_cast<float>(windowWidth) * 0.5f,
        static_cast<float>(windowHeight) * 0.5f,
        50.0f
    };

    PlanetScreenInfo moonScreenInfo = {
        -200,
        static_cast<float>(windowWidth) * 0.5f,
        static_cast<float>(windowHeight) * 0.5f,
        18.0f
    };

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

                if (keyPressed->code == sf::Keyboard::Key::Space)
                {
                    isPaused = !isPaused;
                    std::cout << (isPaused ? "Simulazione in pausa.\n" : "Simulazione ripresa.\n");
                }

                if (
                    keyPressed->code == sf::Keyboard::Key::Add ||
                    keyPressed->code == sf::Keyboard::Key::Equal
                )
                {
                    timeScale += 0.25f;

                    if (timeScale > 5.0f)
                    {
                        timeScale = 5.0f;
                    }

                    std::cout << "Velocita simulazione: " << timeScale << "x\n";
                }

                if (
                    keyPressed->code == sf::Keyboard::Key::Subtract ||
                    keyPressed->code == sf::Keyboard::Key::Hyphen
                )
                {
                    timeScale -= 0.25f;

                    if (timeScale < 0.25f)
                    {
                        timeScale = 0.25f;
                    }

                    std::cout << "Velocita simulazione: " << timeScale << "x\n";
                }

                if (keyPressed->code == sf::Keyboard::Key::R)
                {
                    isFollowingPlanet = false;
                    cameraYaw = 0.0f;
                    cameraPitch = 32.0f;
                    cameraDistance = 22.0f;
                    freeCameraPosition = glm::vec3(0.0f, -18.0f, 8.0f);
                    freeCameraYaw = 0.0f;
                    freeCameraPitch = 18.0f;
                    std::cout << "Camera ripristinata.\n";
                }

                if (keyPressed->code == sf::Keyboard::Key::C)
                {
                    isFreeCamera = !isFreeCamera;
                    isFollowingPlanet = false;
                    std::cout << (isFreeCamera ? "Camera libera attiva.\n" : "Camera orbitale attiva.\n");
                }

                if (keyPressed->code == sf::Keyboard::Key::T)
                {
                    simulationTime = 0.0f;
                    timeScale = defaultTimeScale;
                    isPaused = false;
                    std::cout << "Simulazione temporale ripristinata.\n";
                }

                if (keyPressed->code == sf::Keyboard::Key::F)
                {
                    stopFollowingPlanet(
                        isFollowingPlanet,
                        cameraYaw,
                        cameraPitch,
                        cameraDistance,
                        previousCameraYaw,
                        previousCameraPitch,
                        previousCameraDistance
                    );
                }

                if (keyPressed->code == sf::Keyboard::Key::O)
                {
                    showOrbits = !showOrbits;
                    std::cout << (showOrbits ? "Orbite visibili.\n" : "Orbite nascoste.\n");
                }
int keyboardSelectionNumber = numberFromKey(keyPressed->code);

                if (keyboardSelectionNumber == 1)
                {
                    selectSun(
                        selectedPlanetIndex,
                        isSunSelected,
                        isMoonSelected,
                        isFollowingPlanet,
                        cameraYaw,
                        cameraPitch,
                        cameraDistance,
                        previousCameraYaw,
                        previousCameraPitch,
                        previousCameraDistance,
                        sunInfo,
                        window
                    );
                }
                else if (keyboardSelectionNumber >= 2 && keyboardSelectionNumber <= 9)
                {
                    int keyboardPlanetIndex = keyboardSelectionNumber - 2;
                    isSunSelected = false;
                    isMoonSelected = false;
                    selectPlanet(keyboardPlanetIndex, selectedPlanetIndex, planets, window);
                    startFollowingSelectedPlanet(
                        selectedPlanetIndex,
                        planets,
                        isFollowingPlanet,
                        cameraYaw,
                        cameraPitch,
                        cameraDistance,
                        previousCameraYaw,
                        previousCameraPitch,
                        previousCameraDistance
                    );
                }
            }

            if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (mousePressed->button == sf::Mouse::Button::Left)
                {
                    isMouseDragging = true;
                    hasMouseDragged = false;
                    mousePressPosition = mousePressed->position;
                    previousMousePosition = mousePressed->position;
                }
            }

            if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>())
            {
                if (isMouseDragging)
                {
                    sf::Vector2i currentPosition = mouseMoved->position;
                    sf::Vector2i delta = currentPosition - previousMousePosition;
                    sf::Vector2i totalDelta = currentPosition - mousePressPosition;

                    if (
                        std::abs(totalDelta.x) > 4 ||
                        std::abs(totalDelta.y) > 4
                    )
                    {
                        hasMouseDragged = true;
                    }

                    if (isFreeCamera)
                    {
                        freeCameraYaw += static_cast<float>(delta.x) * 0.18f;
                        freeCameraPitch -= static_cast<float>(delta.y) * 0.16f;
                    }
                    else
                    {
                        cameraYaw += static_cast<float>(delta.x) * 0.28f;
                        cameraPitch += static_cast<float>(delta.y) * 0.22f;
                    }

                    previousMousePosition = currentPosition;
                }
            }

            if (const auto* mouseReleased = event->getIf<sf::Event::MouseButtonReleased>())
            {
                if (mouseReleased->button == sf::Mouse::Button::Left)
                {
                    isMouseDragging = false;

                    if (!hasMouseDragged)
                    {
                        handleCelestialBodyClick(
                            mouseReleased->position.x,
                            mouseReleased->position.y,
                            sunScreenInfo,
                            moonScreenInfo,
                            planetScreenInfos,
                            sunInfo,
                            moonInfo,
                            planets,
                            selectedPlanetIndex,
                            isSunSelected,
                            isMoonSelected,
                            isFollowingPlanet,
                            cameraYaw,
                            cameraPitch,
                            cameraDistance,
                            previousCameraYaw,
                            previousCameraPitch,
                            previousCameraDistance,
                            window
                        );
                    }
                }
            }

            if (const auto* mouseWheelScrolled = event->getIf<sf::Event::MouseWheelScrolled>())
            {
                if (mouseWheelScrolled->wheel == sf::Mouse::Wheel::Vertical)
                {
                    if (isFreeCamera)
                    {
                        freeCameraPosition += createFreeCameraForward(
                            freeCameraYaw,
                            freeCameraPitch
                        ) * (mouseWheelScrolled->delta * 1.8f);
                    }
                    else
                    {
                        cameraDistance -= mouseWheelScrolled->delta * 1.4f;
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
        float freeCameraSpeed = 9.0f;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift))
        {
            freeCameraSpeed *= 2.0f;
        }

        if (!isFreeCamera && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
        {
            cameraYaw -= cameraRotationSpeed * deltaTime;
        }

        if (!isFreeCamera && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
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

        if (!isFreeCamera && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
        {
            cameraDistance -= cameraZoomSpeed * deltaTime;
        }

        if (!isFreeCamera && sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
        {
            cameraDistance += cameraZoomSpeed * deltaTime;
        }

        if (isFreeCamera)
        {
            glm::vec3 forward = createFreeCameraForward(freeCameraYaw, freeCameraPitch);
            glm::vec3 right = createFreeCameraRight(freeCameraYaw);
            glm::vec3 up(0.0f, 0.0f, 1.0f);
            glm::vec3 movement(0.0f);

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W))
            {
                movement += forward;
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
            {
                movement -= forward;
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D))
            {
                movement += right;
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A))
            {
                movement -= right;
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::E))
            {
                movement += up;
            }

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Q))
            {
                movement -= up;
            }

            if (glm::length(movement) > 0.0f)
            {
                freeCameraPosition += glm::normalize(movement) * freeCameraSpeed * deltaTime;
            }
        }

        if (cameraPitch > 80.0f)
        {
            cameraPitch = 80.0f;
        }

        if (cameraPitch < 5.0f)
        {
            cameraPitch = 5.0f;
        }

        if (freeCameraPitch > 85.0f)
        {
            freeCameraPitch = 85.0f;
        }

        if (freeCameraPitch < -85.0f)
        {
            freeCameraPitch = -85.0f;
        }

        float minimumCameraDistance = isFollowingPlanet ? 3.5f : 8.0f;
        float maximumCameraDistance = isFollowingPlanet ? 18.0f : 45.0f;

        if (isSunSelected)
        {
            minimumCameraDistance = 4.0f;
            maximumCameraDistance = 16.0f;
        }

        if (cameraDistance < minimumCameraDistance)
        {
            cameraDistance = minimumCameraDistance;
        }

        if (cameraDistance > maximumCameraDistance)
        {
            cameraDistance = maximumCameraDistance;
        }

        if (!isPaused)
        {
            simulationTime += deltaTime * timeScale;
        }

        glm::vec3 cameraTarget(0.0f, 0.0f, 0.0f);

        if (
            isFollowingPlanet &&
            selectedPlanetIndex >= 0 &&
            selectedPlanetIndex < static_cast<int>(planets.size())
        )
        {
            glm::mat4 selectedPlanetModel = createPlanetModel(
                planets[selectedPlanetIndex],
                simulationTime
            );
            cameraTarget = glm::vec3(selectedPlanetModel * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        }

        glm::vec3 cameraWorldPosition = cameraTarget + createCameraPosition(
            cameraYaw,
            cameraPitch,
            cameraDistance
        );

        glm::mat4 view = createCameraViewAroundTarget(
            cameraYaw,
            cameraPitch,
            cameraDistance,
            cameraTarget
        );

        if (isFreeCamera)
        {
            cameraWorldPosition = freeCameraPosition;
            view = createFreeCameraView(
                freeCameraPosition,
                freeCameraYaw,
                freeCameraPitch
            );
        }

        glUseProgram(shaderProgram);
        glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(view));

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec3 lightPosition(0.0f, 0.0f, 0.0f);

        glm::mat4 starsModel = glm::mat4(1.0f);
        starsModel = glm::translate(starsModel, cameraWorldPosition);
        starsModel = glm::rotate(
            starsModel,
            simulationTime * 0.015f,
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        starsModel = glm::scale(starsModel, glm::vec3(35.0f));

        glDepthMask(GL_FALSE);
        drawSphere(
            shaderProgram,
            VAO,
            static_cast<unsigned int>(sphereIndices.size()),
            starsModel,
            glm::vec3(0.55f, 0.60f, 0.72f),
            lightPosition,
            cameraWorldPosition,
            6,
            starsTexture,
            true
        );
        glDepthMask(GL_TRUE);

        glLineWidth(1.5f);
        const int venusIndex = 1;
        const int earthIndex = 2;
        const int saturnIndex = 5;

        if (showOrbits)
        {
            for (const Planet& planet : planets)
            {
                drawOrbit(
                    shaderProgram,
                    orbitVAO,
                    static_cast<int>(orbitVertices.size()),
                    planet.orbitRadius,
                    planet.orbitEccentricity,
                    glm::vec3(0.35f, 0.35f, 0.45f)
                );
            }

            glm::mat4 moonOrbitModel = createPlanetOrbitModel(planets[earthIndex], simulationTime);
            moonOrbitModel = glm::scale(
                moonOrbitModel,
                glm::vec3(moonInfo.orbitRadius, moonInfo.orbitRadius, 1.0f)
            );

            drawOrbitWithModel(
                shaderProgram,
                orbitVAO,
                static_cast<int>(orbitVertices.size()),
                moonOrbitModel,
                glm::vec3(0.42f, 0.42f, 0.50f)
            );
        }

        glm::mat4 sunModel = glm::mat4(1.0f);
        sunModel = glm::rotate(
            sunModel,
            simulationTime * 0.18f,
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        float sunScale = sunInfo.size;

        if (isSunSelected)
        {
            sunScale *= 1.32f;
        }

        sunModel = glm::scale(sunModel, glm::vec3(sunScale));

        glm::vec3 sunColor = glm::vec3(1.0f, 1.0f, 1.0f);

        if (isSunSelected)
        {
            sunColor = glm::vec3(1.15f, 1.15f, 0.85f);
        }

        drawSphere(
            shaderProgram,
            VAO,
            static_cast<unsigned int>(sphereIndices.size()),
            sunModel,
            sunColor,
            lightPosition,
            cameraWorldPosition,
            2,
            sunTexture,
            true
        );

        planetScreenInfos.clear();

        glm::vec4 viewport(
            0.0f,
            0.0f,
            static_cast<float>(windowWidth),
            static_cast<float>(windowHeight)
        );

        glm::vec4 sunCenterWorld = sunModel * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

        glm::vec3 projectedSun = glm::project(
            glm::vec3(sunCenterWorld),
            view,
            projection,
            viewport
        );

        sunScreenInfo.x = projectedSun.x;
        sunScreenInfo.y = static_cast<float>(windowHeight) - projectedSun.y;
        sunScreenInfo.radius = 36.0f + sunInfo.size * 20.0f;

        for (int i = 0; i < static_cast<int>(planets.size()); ++i)
        {
            const Planet& planet = planets[i];

            glm::mat4 planetModel = createPlanetModel(planet, simulationTime);

            glm::vec3 color = glm::vec3(1.0f);

            drawSphere(
                shaderProgram,
                VAO,
                static_cast<unsigned int>(sphereIndices.size()),
                planetModel,
                color,
                lightPosition,
                cameraWorldPosition,
                1,
                planetTextures[planet.textureIndex],
                true
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

        glm::mat4 earthOverlayBase = createPlanetOrbitModel(planets[earthIndex], simulationTime);

        glm::mat4 earthNightModel = glm::rotate(
            earthOverlayBase,
            simulationTime * planets[earthIndex].rotationSpeed,
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        earthNightModel = glm::scale(
            earthNightModel,
            glm::vec3(planets[earthIndex].size * 1.006f)
        );

        drawSphere(
            shaderProgram,
            VAO,
            static_cast<unsigned int>(sphereIndices.size()),
            earthNightModel,
            glm::vec3(1.0f),
            lightPosition,
            cameraWorldPosition,
            5,
            earthNightTexture,
            true
        );

        glm::mat4 earthCloudModel = glm::rotate(
            earthOverlayBase,
            simulationTime * (planets[earthIndex].rotationSpeed * 1.18f + 0.12f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        earthCloudModel = glm::scale(
            earthCloudModel,
            glm::vec3(planets[earthIndex].size * 1.018f)
        );

        drawSphere(
            shaderProgram,
            VAO,
            static_cast<unsigned int>(sphereIndices.size()),
            earthCloudModel,
            glm::vec3(0.95f, 0.98f, 1.0f),
            lightPosition,
            cameraWorldPosition,
            4,
            earthCloudsTexture,
            true
        );

        glm::mat4 venusAtmosphereModel = createPlanetOrbitModel(planets[venusIndex], simulationTime);
        venusAtmosphereModel = glm::rotate(
            venusAtmosphereModel,
            simulationTime * (planets[venusIndex].rotationSpeed * 0.72f + 0.18f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        venusAtmosphereModel = glm::scale(
            venusAtmosphereModel,
            glm::vec3(planets[venusIndex].size * 1.022f)
        );

        drawSphere(
            shaderProgram,
            VAO,
            static_cast<unsigned int>(sphereIndices.size()),
            venusAtmosphereModel,
            glm::vec3(1.0f, 0.82f, 0.55f),
            lightPosition,
            cameraWorldPosition,
            4,
            venusAtmosphereTexture,
            true
        );

        glm::mat4 saturnRingModel = createPlanetOrbitModel(planets[saturnIndex], simulationTime);
        saturnRingModel = glm::rotate(
            saturnRingModel,
            glm::radians(26.0f),
            glm::vec3(1.0f, 0.0f, 0.0f)
        );
        saturnRingModel = glm::rotate(
            saturnRingModel,
            simulationTime * 0.55f,
            glm::vec3(0.0f, 0.0f, 1.0f)
        );

        drawSphere(
            shaderProgram,
            ringVAO,
            static_cast<unsigned int>(ringIndices.size()),
            saturnRingModel,
            glm::vec3(1.0f),
            lightPosition,
            cameraWorldPosition,
            3,
            saturnRingTexture,
            true
        );

        glm::mat4 moonModel = createMoonModel(planets[earthIndex], moonInfo, simulationTime);
        glm::vec3 moonColor = glm::vec3(1.0f);

        drawSphere(
            shaderProgram,
            VAO,
            static_cast<unsigned int>(sphereIndices.size()),
            moonModel,
            moonColor,
            lightPosition,
            cameraWorldPosition,
            1,
            moonTexture,
            true
        );

        glm::vec4 moonCenterWorld = moonModel * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

        glm::vec3 projectedMoon = glm::project(
            glm::vec3(moonCenterWorld),
            view,
            projection,
            viewport
        );

        moonScreenInfo.x = projectedMoon.x;
        moonScreenInfo.y = static_cast<float>(windowHeight) - projectedMoon.y;
        moonScreenInfo.radius = 18.0f;

        window.display();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    glDeleteVertexArrays(1, &ringVAO);
    glDeleteBuffers(1, &ringVBO);
    glDeleteBuffers(1, &ringEBO);

    glDeleteVertexArrays(1, &orbitVAO);
    glDeleteBuffers(1, &orbitVBO);

    glDeleteTextures(1, &sunTexture);
    glDeleteTextures(1, &moonTexture);
    glDeleteTextures(1, &saturnRingTexture);
    glDeleteTextures(1, &earthCloudsTexture);
    glDeleteTextures(1, &earthNightTexture);
    glDeleteTextures(1, &venusAtmosphereTexture);
    glDeleteTextures(1, &starsTexture);
    for (GLuint textureID : planetTextures)
    {
        glDeleteTextures(1, &textureID);
    }

    glDeleteProgram(shaderProgram);

    return 0;
}
