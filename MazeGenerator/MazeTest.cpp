#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <queue>
#include <string>
#include <algorithm>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <windows.h>
#include <iostream>
#include <direct.h>

sf::String Rus(const char* text) {
    wchar_t wtext[1024];
    MultiByteToWideChar(1251, 0, text, -1, wtext, 1024);
    return sf::String(wtext);
}

std::string GetCurrentDirectory() {
    char buffer[256];
    _getcwd(buffer, 256);
    return std::string(buffer);
}

class BeautifulPathRibbon {
private:
    sf::VertexArray ribbon;
    sf::VertexArray glow;
    sf::VertexArray pattern;
    float thickness;
    float progress;
    std::vector<std::pair<int, int>> pathCells;
    int cellSize;

    const sf::Color startColor = sf::Color(255, 50, 50);
    const sf::Color middleColor = sf::Color(255, 150, 50);
    const sf::Color endColor = sf::Color(255, 50, 150);

public:
    BeautifulPathRibbon(int cellSize) : cellSize(cellSize), thickness(12.0f), progress(0.0f) {}

    void setPath(const std::vector<std::pair<int, int>>& path, float prog) {
        pathCells = path;
        progress = prog;
        updateRibbon();
    }

    void updateRibbon() {
        if (pathCells.empty() || progress <= 0.001f) return;

        float totalSegments = static_cast<float>(pathCells.size() - 1);
        float visibleSegments = progress * totalSegments;
        int numPoints = static_cast<int>(visibleSegments) + 2;

        if (numPoints < 2) return;

        std::vector<sf::Vector2f> points;

        for (int i = 0; i <= static_cast<int>(visibleSegments) && i < static_cast<int>(pathCells.size()); i++) {
            int x = pathCells[i].first;
            int y = pathCells[i].second;
            points.push_back(sf::Vector2f(
                x * cellSize + cellSize / 2.0f,
                y * cellSize + cellSize / 2.0f
            ));
        }

        if (visibleSegments < totalSegments && static_cast<int>(visibleSegments) < static_cast<int>(pathCells.size()) - 1) {
            int index1 = static_cast<int>(visibleSegments);
            int index2 = index1 + 1;
            float t = visibleSegments - index1;

            int x1 = pathCells[index1].first;
            int y1 = pathCells[index1].second;
            int x2 = pathCells[index2].first;
            int y2 = pathCells[index2].second;

            points.push_back(sf::Vector2f(
                (x1 + (x2 - x1) * t) * cellSize + cellSize / 2.0f,
                (y1 + (y2 - y1) * t) * cellSize + cellSize / 2.0f
            ));
        }

        ribbon.clear();
        glow.clear();
        pattern.clear();

        ribbon.setPrimitiveType(sf::TrianglesStrip);
        glow.setPrimitiveType(sf::TrianglesStrip);
        pattern.setPrimitiveType(sf::TrianglesStrip);

        float time = static_cast<float>(std::clock()) / CLOCKS_PER_SEC;

        for (size_t i = 0; i < points.size(); i++) {
            sf::Vector2f pos = points[i];
            sf::Vector2f dir;

            if (i == 0 && points.size() > 1) {
                dir = points[i + 1] - pos;
            }
            else if (i == points.size() - 1 && points.size() > 1) {
                dir = pos - points[i - 1];
            }
            else if (points.size() > 2) {
                dir = points[i + 1] - points[i - 1];
            }
            else {
                dir = sf::Vector2f(1, 0);
            }

            float len = std::sqrt(dir.x * dir.x + dir.y * dir.y);
            if (len > 0.001f) {
                dir /= len;
            }

            sf::Vector2f perp(-dir.y, dir.x);
            float t = static_cast<float>(i) / std::max(1.0f, static_cast<float>(points.size() - 1));

            sf::Color color;
            if (t < 0.3f) {
                float tt = t / 0.3f;
                color = sf::Color(
                    static_cast<sf::Uint8>(startColor.r * (1 - tt) + middleColor.r * tt),
                    static_cast<sf::Uint8>(startColor.g * (1 - tt) + middleColor.g * tt),
                    static_cast<sf::Uint8>(startColor.b * (1 - tt) + middleColor.b * tt),
                    230
                );
            }
            else if (t < 0.7f) {
                float tt = (t - 0.3f) / 0.4f;
                color = sf::Color(
                    static_cast<sf::Uint8>(middleColor.r * (1 - tt) + endColor.r * tt),
                    static_cast<sf::Uint8>(middleColor.g * (1 - tt) + endColor.g * tt),
                    static_cast<sf::Uint8>(middleColor.b * (1 - tt) + endColor.b * tt),
                    230
                );
            }
            else {
                color = endColor;
                color.a = 230;
            }

            float pulse = 0.8f + 0.2f * sin(time * 3 + i * 0.5f);
            sf::Color glowColor = color;
            glowColor.a = 80;

            ribbon.append(sf::Vertex(pos + perp * thickness * 0.5f, color));
            ribbon.append(sf::Vertex(pos - perp * thickness * 0.5f, color));

            glow.append(sf::Vertex(pos + perp * thickness * 1.5f, glowColor));
            glow.append(sf::Vertex(pos - perp * thickness * 1.5f, glowColor));

            if (i % 2 == 0) {
                sf::Color patternColor = sf::Color::White;
                patternColor.a = static_cast<sf::Uint8>(100 * pulse);
                pattern.append(sf::Vertex(pos + perp * thickness * 0.3f, patternColor));
                pattern.append(sf::Vertex(pos - perp * thickness * 0.3f, patternColor));
            }
            else {
                pattern.append(sf::Vertex(pos + perp * thickness * 0.2f, sf::Color::Transparent));
                pattern.append(sf::Vertex(pos - perp * thickness * 0.2f, sf::Color::Transparent));
            }
        }
    }

    void setProgress(float prog) {
        progress = prog;
        updateRibbon();
    }

    void draw(sf::RenderWindow& window) {
        if (pathCells.empty() || progress <= 0.001f) return;

        sf::RenderStates states;
        states.blendMode = sf::BlendAdd;
        window.draw(glow, states);

        states.blendMode = sf::BlendAlpha;
        window.draw(ribbon, states);
        window.draw(pattern, states);

        drawSparkles(window);
    }

private:
    void drawSparkles(sf::RenderWindow& window) {
        if (pathCells.empty()) return;

        float time = static_cast<float>(std::clock()) / CLOCKS_PER_SEC;
        int numSparkles = 15;

        for (int i = 0; i < numSparkles; i++) {
            float t = fmod(time * 0.5f + static_cast<float>(i) / numSparkles, 1.0f);
            if (t > progress) continue;

            sf::Vector2f pos = getPointAtProgress(t);

            sf::CircleShape sparkle(3.0f);
            sparkle.setOrigin(3.0f, 3.0f);
            sparkle.setPosition(pos);

            float brightness = 0.5f + 0.5f * sin(time * 10 + i);
            sparkle.setFillColor(sf::Color(255, 255, 200, static_cast<sf::Uint8>(150 * brightness)));

            window.draw(sparkle);

            for (int j = 0; j < 4; j++) {
                float angle = j * 3.14159f / 2 + time * 5;
                sf::VertexArray ray(sf::Lines, 2);
                ray[0].position = pos;
                ray[1].position = pos + sf::Vector2f(cos(angle), sin(angle)) * 8.0f;
                ray[0].color = sf::Color(255, 255, 200, 100);
                ray[1].color = sf::Color::Transparent;
                window.draw(ray);
            }
        }
    }

    sf::Vector2f getPointAtProgress(float prog) {
        if (pathCells.empty()) return sf::Vector2f(0, 0);

        float totalSegments = static_cast<float>(pathCells.size() - 1);
        float visibleSegments = prog * totalSegments;

        if (visibleSegments >= totalSegments) {
            int x = pathCells.back().first;
            int y = pathCells.back().second;
            return sf::Vector2f(x * cellSize + cellSize / 2.0f, y * cellSize + cellSize / 2.0f);
        }
        else {
            int index1 = static_cast<int>(visibleSegments);
            int index2 = index1 + 1;
            float t = visibleSegments - index1;

            int x1 = pathCells[index1].first;
            int y1 = pathCells[index1].second;
            int x2 = pathCells[index2].first;
            int y2 = pathCells[index2].second;

            return sf::Vector2f(
                (x1 + (x2 - x1) * t) * cellSize + cellSize / 2.0f,
                (y1 + (y2 - y1) * t) * cellSize + cellSize / 2.0f
            );
        }
    }
};

struct Cell {
    bool wallTop = true;
    bool wallRight = true;
    bool wallBottom = true;
    bool wallLeft = true;
    bool visited = false;
    bool path = false;
    int distance = 0;
    int parentX = -1;
    int parentY = -1;

    float animProgress = 0.0f;
    float targetProgress = 0.0f;
};

class Maze {
private:
    int width, height;
    int cellSize;
    std::vector<std::vector<Cell>> grid;
    std::string currentGenAlgorithm = "Рекурсивный";
    int pathLength = 0;
    float solveTime = 0.0f;

    bool isSolving = false;
    bool pathCompleted = false;
    const float ANIMATION_SPEED = 0.001f;
    std::vector<std::pair<int, int>> pathCells;
    float globalLineProgress = 0.0f;

    BeautifulPathRibbon pathRibbon;

    sf::Texture snakeTexture;
    sf::Sprite snakeSprite;
    sf::Texture finishTexture;
    sf::Sprite finishSprite;
    bool textureLoaded = false;
    bool finishLoaded = false;

    sf::Font font;
    sf::Text startText;
    sf::Text finishText;

    int startX, startY;
    int finishX, finishY;

    bool isTooClose(int x1, int y1, int x2, int y2) {
        int minDistance = 5;
        int dx = abs(x1 - x2);
        int dy = abs(y1 - y2);
        return (dx + dy) < minDistance;
    }

    void carvePath(int x, int y) {
        grid[y][x].visited = true;

        int directions[4][3] = {
            {0, -1, 0},
            {1, 0, 1},
            {0, 1, 2},
            {-1, 0, 3}
        };

        for (int i = 0; i < 4; i++) {
            int r = rand() % 4;
            std::swap(directions[i][0], directions[r][0]);
            std::swap(directions[i][1], directions[r][1]);
            std::swap(directions[i][2], directions[r][2]);
        }

        for (int i = 0; i < 4; i++) {
            int newX = x + directions[i][0];
            int newY = y + directions[i][1];

            if (newX >= 0 && newX < width && newY >= 0 && newY < height && !grid[newY][newX].visited) {
                if (directions[i][2] == 0) {
                    grid[y][x].wallTop = false;
                    grid[newY][newX].wallBottom = false;
                }
                else if (directions[i][2] == 1) {
                    grid[y][x].wallRight = false;
                    grid[newY][newX].wallLeft = false;
                }
                else if (directions[i][2] == 2) {
                    grid[y][x].wallBottom = false;
                    grid[newY][newX].wallTop = false;
                }
                else if (directions[i][2] == 3) {
                    grid[y][x].wallLeft = false;
                    grid[newY][newX].wallRight = false;
                }
                carvePath(newX, newY);
            }
        }
    }

    void findPathBFS() {
        std::queue<std::pair<int, int>> q;
        std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
        pathCells.clear();

        q.push({ startX, startY });
        visited[startY][startX] = true;
        grid[startY][startX].distance = 0;
        grid[startY][startX].targetProgress = 1.0f;

        bool found = false;

        while (!q.empty() && !found) {
            int x = q.front().first;
            int y = q.front().second;
            q.pop();

            if (!grid[y][x].wallTop && y > 0 && !visited[y - 1][x]) {
                visited[y - 1][x] = true;
                grid[y - 1][x].distance = grid[y][x].distance + 1;
                grid[y - 1][x].parentX = x;
                grid[y - 1][x].parentY = y;
                grid[y - 1][x].targetProgress = 1.0f;

                if (x == finishX && y - 1 == finishY) {
                    found = true;
                    break;
                }
                q.push({ x, y - 1 });
            }
            if (!grid[y][x].wallRight && x < width - 1 && !visited[y][x + 1]) {
                visited[y][x + 1] = true;
                grid[y][x + 1].distance = grid[y][x].distance + 1;
                grid[y][x + 1].parentX = x;
                grid[y][x + 1].parentY = y;
                grid[y][x + 1].targetProgress = 1.0f;

                if (x + 1 == finishX && y == finishY) {
                    found = true;
                    break;
                }
                q.push({ x + 1, y });
            }
            if (!grid[y][x].wallBottom && y < height - 1 && !visited[y + 1][x]) {
                visited[y + 1][x] = true;
                grid[y + 1][x].distance = grid[y][x].distance + 1;
                grid[y + 1][x].parentX = x;
                grid[y + 1][x].parentY = y;
                grid[y + 1][x].targetProgress = 1.0f;

                if (x == finishX && y + 1 == finishY) {
                    found = true;
                    break;
                }
                q.push({ x, y + 1 });
            }
            if (!grid[y][x].wallLeft && x > 0 && !visited[y][x - 1]) {
                visited[y][x - 1] = true;
                grid[y][x - 1].distance = grid[y][x].distance + 1;
                grid[y][x - 1].parentX = x;
                grid[y][x - 1].parentY = y;
                grid[y][x - 1].targetProgress = 1.0f;

                if (x - 1 == finishX && y == finishY) {
                    found = true;
                    break;
                }
                q.push({ x - 1, y });
            }
        }

        if (found || visited[finishY][finishX]) {
            int x = finishX, y = finishY;
            while (!(x == startX && y == startY)) {
                grid[y][x].path = true;
                pathCells.push_back({ x, y });

                int newX = grid[y][x].parentX;
                int newY = grid[y][x].parentY;
                if (newX == -1 || newY == -1) break;
                x = newX; y = newY;
            }
            pathCells.push_back({ startX, startY });
            std::reverse(pathCells.begin(), pathCells.end());
            globalLineProgress = 0.0f;
            pathCompleted = false;
        }
    }

    void updateAnimation() {
        bool anyAnimating = false;

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float& current = grid[y][x].animProgress;
                float target = grid[y][x].targetProgress;

                if (std::abs(current - target) > 0.001f) {
                    current += (target > current) ? ANIMATION_SPEED : -ANIMATION_SPEED;
                    if (std::abs(current - target) < 0.01f) current = target;
                    anyAnimating = true;
                }
            }
        }

        if (!pathCells.empty() && globalLineProgress < 1.0f) {
            globalLineProgress += ANIMATION_SPEED * 0.2f;
            if (globalLineProgress > 1.0f) {
                globalLineProgress = 1.0f;
                pathCompleted = true;
            }
            anyAnimating = true;
            pathRibbon.setPath(pathCells, globalLineProgress);
        }

        isSolving = anyAnimating;
    }

    std::pair<float, float> getSnakeHeadPosition() {
        if (pathCells.empty() || globalLineProgress <= 0.001f) {
            return { 0, 0 };
        }

        float totalSegments = static_cast<float>(pathCells.size() - 1);
        float visibleSegments = globalLineProgress * totalSegments;

        if (visibleSegments >= totalSegments) {
            int x = pathCells.back().first;
            int y = pathCells.back().second;
            return { x * cellSize + cellSize / 2.0f, y * cellSize + cellSize / 2.0f };
        }
        else {
            int index1 = static_cast<int>(visibleSegments);
            int index2 = index1 + 1;
            float t = visibleSegments - index1;

            int x1 = pathCells[index1].first;
            int y1 = pathCells[index1].second;
            int x2 = pathCells[index2].first;
            int y2 = pathCells[index2].second;

            float worldX1 = x1 * cellSize + cellSize / 2.0f;
            float worldY1 = y1 * cellSize + cellSize / 2.0f;
            float worldX2 = x2 * cellSize + cellSize / 2.0f;
            float worldY2 = y2 * cellSize + cellSize / 2.0f;

            float worldX = worldX1 + (worldX2 - worldX1) * t;
            float worldY = worldY1 + (worldY2 - worldY1) * t;

            return { worldX, worldY };
        }
    }

public:
    Maze(int w, int h, int cell = 40) : width(w), height(h), cellSize(cell), pathRibbon(cell) {
        grid.resize(height, std::vector<Cell>(width));
        pathCompleted = false;

        font.loadFromFile("C:/Windows/Fonts/arial.ttf");

        startText.setFont(font);
        startText.setString(Rus("СТАРТ"));
        startText.setCharacterSize(12);
        startText.setFillColor(sf::Color(200, 200, 200));
        startText.setStyle(sf::Text::Bold);

        finishText.setFont(font);
        finishText.setString(Rus("ФИНИШ"));
        finishText.setCharacterSize(12);
        finishText.setFillColor(sf::Color(200, 200, 200));
        finishText.setStyle(sf::Text::Bold);

        startX = 0;
        startY = 0;
        finishX = width - 1;
        finishY = height - 1;

        std::string possiblePaths[] = {
            "C:/Project/MazeGenerator/x64/Debug/meepo1.png",
            "meepo1.png"
        };

        textureLoaded = false;
        for (const auto& path : possiblePaths) {
            if (snakeTexture.loadFromFile(path)) {
                snakeSprite.setTexture(snakeTexture);

                sf::Vector2u textureSize = snakeTexture.getSize();

                float baseScale = 4.0f * (cellSize - 10) / static_cast<float>(textureSize.x);
                float scaleX = baseScale * 1.3f;
                float scaleY = baseScale * 1.0f;

                snakeSprite.setScale(scaleX, scaleY);
                snakeSprite.setOrigin(textureSize.x / 2.0f, textureSize.y / 2.0f);

                textureLoaded = true;
                break;
            }
        }

        std::string finishPaths[] = {
            "C:/Project/MazeGenerator/x64/Debug/meepo.png",
            "meepo.png"
        };

        finishLoaded = false;
        for (const auto& path : finishPaths) {
            if (finishTexture.loadFromFile(path)) {
                finishSprite.setTexture(finishTexture);

                sf::Vector2u textureSize = finishTexture.getSize();

                float scaleX = 4.0f * (cellSize - 10) / static_cast<float>(textureSize.x);
                float scaleY = 4.0f * (cellSize - 10) / static_cast<float>(textureSize.y);

                finishSprite.setScale(scaleX, scaleY);
                finishSprite.setOrigin(textureSize.x / 2.0f, textureSize.y / 2.0f);

                finishLoaded = true;
                break;
            }
        }
    }

    void generateRecursive() {
        reset();
        std::srand(static_cast<unsigned int>(std::time(NULL)));

        startX = std::rand() % width;
        startY = std::rand() % height;

        do {
            finishX = std::rand() % width;
            finishY = std::rand() % height;
        } while (isTooClose(startX, startY, finishX, finishY));

        currentGenAlgorithm = "Рекурсивный";
        carvePath(startX, startY);
    }

    void generateEmpty() {
        reset();
        std::srand(static_cast<unsigned int>(std::time(NULL)));

        startX = std::rand() % width;
        startY = std::rand() % height;

        do {
            finishX = std::rand() % width;
            finishY = std::rand() % height;
        } while (isTooClose(startX, startY, finishX, finishY));

        currentGenAlgorithm = "Пустой";
    }

    void generateFull() {
        reset();
        std::srand(static_cast<unsigned int>(std::time(NULL)));

        startX = std::rand() % width;
        startY = std::rand() % height;

        do {
            finishX = std::rand() % width;
            finishY = std::rand() % height;
        } while (isTooClose(startX, startY, finishX, finishY));

        currentGenAlgorithm = "Открытый";
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                grid[y][x].wallTop = false;
                grid[y][x].wallRight = false;
                grid[y][x].wallBottom = false;
                grid[y][x].wallLeft = false;
            }
        }
        for (int x = 0; x < width; x++) {
            grid[0][x].wallTop = true;
            grid[height - 1][x].wallBottom = true;
        }
        for (int y = 0; y < height; y++) {
            grid[y][0].wallLeft = true;
            grid[y][width - 1].wallRight = true;
        }
    }

    void reset() {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                grid[y][x].visited = false;
                grid[y][x].path = false;
                grid[y][x].wallTop = true;
                grid[y][x].wallRight = true;
                grid[y][x].wallBottom = true;
                grid[y][x].wallLeft = true;
                grid[y][x].distance = 0;
                grid[y][x].parentX = -1;
                grid[y][x].parentY = -1;
                grid[y][x].animProgress = 0.0f;
                grid[y][x].targetProgress = 0.0f;
            }
        }
        pathCells.clear();
        globalLineProgress = 0.0f;
        pathLength = 0;
        solveTime = 0.0f;
        isSolving = false;
        pathCompleted = false;
    }

    void solveBFS() {
        if (isSolving) return;

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                grid[y][x].path = false;
                grid[y][x].animProgress = 0.0f;
                grid[y][x].targetProgress = 0.0f;
            }
        }

        pathCells.clear();
        globalLineProgress = 0.0f;
        pathCompleted = false;

        std::clock_t start = std::clock();
        findPathBFS();
        std::clock_t end = std::clock();
        solveTime = static_cast<float>(end - start) / CLOCKS_PER_SEC * 1000.0f;

        countPathLength();
        isSolving = true;
    }

    void countPathLength() {
        pathLength = 0;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (grid[y][x].path) pathLength++;
            }
        }
    }

    void handleMouseClick(int mouseX, int mouseY, bool leftButton, bool rightButton) {
        if (isSolving) return;

        int cellX = mouseX / cellSize;
        int cellY = mouseY / cellSize;

        if (cellX >= 0 && cellX < width && cellY >= 0 && cellY < height) {
            int relX = mouseX % cellSize;
            int relY = mouseY % cellSize;

            bool nearTop = relY < 5;
            bool nearBottom = relY > cellSize - 5;
            bool nearLeft = relX < 5;
            bool nearRight = relX > cellSize - 5;

            if (leftButton) {
                if (nearTop) {
                    grid[cellY][cellX].wallTop = !grid[cellY][cellX].wallTop;
                    if (cellY > 0) grid[cellY - 1][cellX].wallBottom = grid[cellY][cellX].wallTop;
                }
                else if (nearBottom) {
                    grid[cellY][cellX].wallBottom = !grid[cellY][cellX].wallBottom;
                    if (cellY < height - 1) grid[cellY + 1][cellX].wallTop = grid[cellY][cellX].wallBottom;
                }
                else if (nearLeft) {
                    grid[cellY][cellX].wallLeft = !grid[cellY][cellX].wallLeft;
                    if (cellX > 0) grid[cellY][cellX - 1].wallRight = grid[cellY][cellX].wallLeft;
                }
                else if (nearRight) {
                    grid[cellY][cellX].wallRight = !grid[cellY][cellX].wallRight;
                    if (cellX < width - 1) grid[cellY][cellX + 1].wallLeft = grid[cellY][cellX].wallRight;
                }
                else {
                    return;
                }
            }
            else if (rightButton) {
                if (nearTop) {
                    grid[cellY][cellX].wallTop = false;
                    if (cellY > 0) grid[cellY - 1][cellX].wallBottom = false;
                }
                else if (nearBottom) {
                    grid[cellY][cellX].wallBottom = false;
                    if (cellY < height - 1) grid[cellY + 1][cellX].wallTop = false;
                }
                else if (nearLeft) {
                    grid[cellY][cellX].wallLeft = false;
                    if (cellX > 0) grid[cellY][cellX - 1].wallRight = false;
                }
                else if (nearRight) {
                    grid[cellY][cellX].wallRight = false;
                    if (cellX < width - 1) grid[cellY][cellX + 1].wallLeft = false;
                }
                else {
                    return;
                }
            }

            clearPath();
        }
    }

    void clearPath() {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                grid[y][x].path = false;
                grid[y][x].animProgress = 0.0f;
                grid[y][x].targetProgress = 0.0f;
            }
        }
        pathCells.clear();
        globalLineProgress = 0.0f;
        pathLength = 0;
        solveTime = 0.0f;
        isSolving = false;
        pathCompleted = false;
    }

    int getPathLength() { return pathLength; }
    float getSolveTime() { return solveTime; }
    std::string getGenAlgorithm() { return currentGenAlgorithm; }
    bool isAnimating() { return isSolving; }

    void draw(sf::RenderWindow& window) {
        updateAnimation();

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                Cell& cell = grid[y][x];
                float x1 = static_cast<float>(x * cellSize);
                float y1 = static_cast<float>(y * cellSize);
                float x2 = static_cast<float>((x + 1) * cellSize);
                float y2 = static_cast<float>((y + 1) * cellSize);

                if (cell.path && !(x == startX && y == startY) && !(x == finishX && y == finishY)) {
                    sf::RectangleShape pathRect(sf::Vector2f(static_cast<float>(cellSize - 4),
                        static_cast<float>(cellSize - 4)));

                    sf::Uint8 alpha = static_cast<sf::Uint8>(180.0f * cell.animProgress);
                    pathRect.setFillColor(sf::Color(100, 200, 255, alpha));
                    pathRect.setPosition(x1 + 2.0f, y1 + 2.0f);
                    window.draw(pathRect);
                }

                sf::RectangleShape wall;
                wall.setFillColor(sf::Color(60, 70, 90));

                if (cell.wallTop) {
                    wall.setSize(sf::Vector2f(static_cast<float>(cellSize), 5.0f));
                    wall.setPosition(x1, y1);
                    window.draw(wall);
                }
                if (cell.wallBottom) {
                    wall.setSize(sf::Vector2f(static_cast<float>(cellSize), 5.0f));
                    wall.setPosition(x1, y2 - 3.0f);
                    window.draw(wall);
                }
                if (cell.wallLeft) {
                    wall.setSize(sf::Vector2f(5.0f, static_cast<float>(cellSize)));
                    wall.setPosition(x1, y1);
                    window.draw(wall);
                }
                if (cell.wallRight) {
                    wall.setSize(sf::Vector2f(5.0f, static_cast<float>(cellSize)));
                    wall.setPosition(x2 - 3.0f, y1);
                    window.draw(wall);
                }
            }
        }

        if (!pathCells.empty() && globalLineProgress > 0.001f) {
            pathRibbon.draw(window);
        }

        sf::RectangleShape startRect(sf::Vector2f(static_cast<float>(cellSize - 6),
            static_cast<float>(cellSize - 6)));
        startRect.setFillColor(sf::Color(70, 130, 200, 255));
        startRect.setPosition(static_cast<float>(startX * cellSize + 3),
            static_cast<float>(startY * cellSize + 3));
        window.draw(startRect);

        sf::FloatRect startTextBounds = startText.getLocalBounds();
        startText.setOrigin(startTextBounds.width / 2, startTextBounds.height / 2);
        startText.setPosition(
            startX * cellSize + cellSize / 2.0f,
            startY * cellSize + cellSize / 2.0f - cellSize / 3
        );
        window.draw(startText);

        if (textureLoaded && !isSolving && !pathCompleted) {
            float startSpriteX = startX * cellSize + cellSize / 2.0f;
            float startSpriteY = startY * cellSize + cellSize / 2.0f;

            snakeSprite.setPosition(startSpriteX, startSpriteY);
            snakeSprite.setRotation(0.0f);
            window.draw(snakeSprite);
        }

        if (textureLoaded && !pathCells.empty() && globalLineProgress > 0.001f) {
            std::pair<float, float> headPos = getSnakeHeadPosition();
            snakeSprite.setPosition(headPos.first, headPos.second);
            snakeSprite.setRotation(0.0f);
            window.draw(snakeSprite);
        }

        if (finishLoaded) {
            float finishSpriteX = finishX * cellSize + cellSize / 2.0f;
            float finishSpriteY = finishY * cellSize + cellSize / 2.0f;
            finishSprite.setPosition(finishSpriteX, finishSpriteY);
            finishSprite.setRotation(0.0f);
            window.draw(finishSprite);
        }

        sf::FloatRect finishTextBounds = finishText.getLocalBounds();
        finishText.setOrigin(finishTextBounds.width / 2, finishTextBounds.height / 2);
        finishText.setPosition(
            finishX * cellSize + cellSize / 2.0f,
            finishY * cellSize + cellSize / 2.0f - cellSize / 3
        );
        window.draw(finishText);
    }
};

class BackgroundMaze {
private:
    int width, height;
    int cellSize;
    std::vector<std::vector<Cell>> grid;
    sf::Clock regenerationClock;
    const float REGENERATION_TIME = 7.0f;

    void generateRandomMaze() {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                grid[y][x].wallTop = true;
                grid[y][x].wallRight = true;
                grid[y][x].wallBottom = true;
                grid[y][x].wallLeft = true;
                grid[y][x].visited = false;
            }
        }

        std::srand(static_cast<unsigned int>(std::time(NULL)) + rand());
        int startX = std::rand() % width;
        int startY = std::rand() % height;

        carvePath(startX, startY);
    }

    void carvePath(int x, int y) {
        grid[y][x].visited = true;

        int directions[4][3] = {
            {0, -1, 0},
            {1, 0, 1},
            {0, 1, 2},
            {-1, 0, 3}
        };

        for (int i = 0; i < 4; i++) {
            int r = rand() % 4;
            std::swap(directions[i][0], directions[r][0]);
            std::swap(directions[i][1], directions[r][1]);
            std::swap(directions[i][2], directions[r][2]);
        }

        for (int i = 0; i < 4; i++) {
            int newX = x + directions[i][0];
            int newY = y + directions[i][1];

            if (newX >= 0 && newX < width && newY >= 0 && newY < height && !grid[newY][newX].visited) {
                if (directions[i][2] == 0) {
                    grid[y][x].wallTop = false;
                    grid[newY][newX].wallBottom = false;
                }
                else if (directions[i][2] == 1) {
                    grid[y][x].wallRight = false;
                    grid[newY][newX].wallLeft = false;
                }
                else if (directions[i][2] == 2) {
                    grid[y][x].wallBottom = false;
                    grid[newY][newX].wallTop = false;
                }
                else if (directions[i][2] == 3) {
                    grid[y][x].wallLeft = false;
                    grid[newY][newX].wallRight = false;
                }
                carvePath(newX, newY);
            }
        }
    }

public:
    BackgroundMaze(int w, int h, int cell = 40) : width(w), height(h), cellSize(cell) {
        grid.resize(height, std::vector<Cell>(width));
        generateRandomMaze();
        regenerationClock.restart();
    }

    void update() {
        if (regenerationClock.getElapsedTime().asSeconds() >= REGENERATION_TIME) {
            generateRandomMaze();
            regenerationClock.restart();
        }
    }

    void draw(sf::RenderWindow& window) {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                Cell& cell = grid[y][x];

                float offsetX = (1024 - width * cellSize) / 2;
                float offsetY = (768 - height * cellSize) / 2;

                float x1 = offsetX + static_cast<float>(x * cellSize);
                float y1 = offsetY + static_cast<float>(y * cellSize);
                float x2 = offsetX + static_cast<float>((x + 1) * cellSize);
                float y2 = offsetY + static_cast<float>((y + 1) * cellSize);

                sf::RectangleShape wall;
                wall.setFillColor(sf::Color(100, 100, 100, 30));

                if (cell.wallTop) {
                    wall.setSize(sf::Vector2f(static_cast<float>(cellSize), 5.0f));
                    wall.setPosition(x1, y1);
                    window.draw(wall);
                }
                if (cell.wallBottom) {
                    wall.setSize(sf::Vector2f(static_cast<float>(cellSize), 5.0f));
                    wall.setPosition(x1, y2 - 3.0f);
                    window.draw(wall);
                }
                if (cell.wallLeft) {
                    wall.setSize(sf::Vector2f(5.0f, static_cast<float>(cellSize)));
                    wall.setPosition(x1, y1);
                    window.draw(wall);
                }
                if (cell.wallRight) {
                    wall.setSize(sf::Vector2f(5.0f, static_cast<float>(cellSize)));
                    wall.setPosition(x2 - 3.0f, y1);
                    window.draw(wall);
                }
            }
        }
    }
};

class StartMenu {
private:
    sf::Font font;
    sf::Text titleText;
    sf::RectangleShape playButton;
    sf::Text playText;
    sf::RectangleShape background;
    sf::RectangleShape buttonShadow;

    BackgroundMaze backgroundMaze;
    sf::VertexArray gradient;

    bool buttonHovered = false;

public:
    StartMenu() : backgroundMaze(15, 10, 45) {
        font.loadFromFile("C:/Windows/Fonts/arial.ttf");

        background.setSize(sf::Vector2f(1024, 768));
        background.setFillColor(sf::Color(25, 25, 35, 200));

        titleText.setFont(font);
        titleText.setString(Rus(" Meepo в лабиринте "));
        titleText.setCharacterSize(48);
        titleText.setFillColor(sf::Color(255, 183, 0));
        titleText.setStyle(sf::Text::Bold);
        sf::FloatRect titleBounds = titleText.getLocalBounds();
        titleText.setOrigin(titleBounds.width / 2, titleBounds.height / 2);
        titleText.setPosition(512, 200);

        buttonShadow.setSize(sf::Vector2f(290, 100));
        buttonShadow.setFillColor(sf::Color(0, 0, 0, 100));
        buttonShadow.setOrigin(145, 50);
        buttonShadow.setPosition(515, 405);

        playButton.setSize(sf::Vector2f(280, 90));
        playButton.setFillColor(sf::Color(70, 130, 200));
        playButton.setOutlineThickness(0);
        playButton.setOrigin(140, 45);
        playButton.setPosition(512, 400);

        gradient.setPrimitiveType(sf::Quads);
        gradient.resize(4);

        gradient[0].position = sf::Vector2f(512 - 140, 400 - 45);
        gradient[1].position = sf::Vector2f(512 + 140, 400 - 45);
        gradient[0].color = sf::Color(120, 180, 250, 230);
        gradient[1].color = sf::Color(120, 180, 250, 230);

        gradient[2].position = sf::Vector2f(512 + 140, 400 + 45);
        gradient[3].position = sf::Vector2f(512 - 140, 400 + 45);
        gradient[2].color = sf::Color(40, 100, 180, 230);
        gradient[3].color = sf::Color(40, 100, 180, 230);

        playText.setFont(font);
        playText.setString(Rus("ИГРАТЬ"));
        playText.setCharacterSize(42);
        playText.setFillColor(sf::Color::White);
        playText.setStyle(sf::Text::Bold);
        sf::FloatRect textBounds = playText.getLocalBounds();
        playText.setOrigin(textBounds.width / 2, textBounds.height / 2);
        playText.setPosition(512, 400);
    }

    bool update(sf::RenderWindow& window) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
                return false;
            }

            if (event.type == sf::Event::MouseMoved) {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                sf::FloatRect buttonBounds = playButton.getGlobalBounds();

                if (buttonBounds.contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                    buttonHovered = true;
                    gradient[0].color = sf::Color(150, 210, 255, 250);
                    gradient[1].color = sf::Color(150, 210, 255, 250);
                    gradient[2].color = sf::Color(70, 140, 220, 250);
                    gradient[3].color = sf::Color(70, 140, 220, 250);
                    playText.setFillColor(sf::Color(255, 255, 200));
                    playText.setCharacterSize(44);
                }
                else {
                    buttonHovered = false;
                    gradient[0].color = sf::Color(120, 180, 250, 230);
                    gradient[1].color = sf::Color(120, 180, 250, 230);
                    gradient[2].color = sf::Color(40, 100, 180, 230);
                    gradient[3].color = sf::Color(40, 100, 180, 230);
                    playText.setFillColor(sf::Color::White);
                    playText.setCharacterSize(42);
                }
            }

            if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                    sf::FloatRect buttonBounds = playButton.getGlobalBounds();

                    if (buttonBounds.contains(static_cast<float>(mousePos.x), static_cast<float>(mousePos.y))) {
                        playButton.setScale(0.95f, 0.95f);
                        playText.setScale(0.95f, 0.95f);
                        buttonShadow.setScale(0.95f, 0.95f);

                        window.display();
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));

                        playButton.setScale(1.0f, 1.0f);
                        playText.setScale(1.0f, 1.0f);
                        buttonShadow.setScale(1.0f, 1.0f);

                        return true;
                    }
                }
            }
        }

        backgroundMaze.update();
        return false;
    }

    void draw(sf::RenderWindow& window) {
        window.clear();
        backgroundMaze.draw(window);
        window.draw(background);
        window.draw(titleText);

        window.draw(buttonShadow);
        window.draw(gradient);
        window.draw(playButton);
        window.draw(playText);

        sf::Text infoText;
        infoText.setFont(font);
        infoText.setString(Rus("Помоги Meepo найти братьев!"));
        infoText.setCharacterSize(24);
        infoText.setFillColor(sf::Color(200, 200, 200));
        sf::FloatRect infoBounds = infoText.getLocalBounds();
        infoText.setOrigin(infoBounds.width / 2, infoBounds.height / 2);
        infoText.setPosition(512, 550);
        window.draw(infoText);

        window.display();
    }
};

int main()
{
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);

    ShowWindow(GetConsoleWindow(), SW_HIDE);

    sf::RenderWindow menuWindow(
        sf::VideoMode(1024, 768),
        "Meepo Maze",
        sf::Style::Titlebar | sf::Style::Close
    );

    StartMenu startMenu;

    bool startGame = false;
    while (menuWindow.isOpen() && !startGame) {
        startGame = startMenu.update(menuWindow);
        startMenu.draw(menuWindow);
    }

    if (!menuWindow.isOpen()) {
        return 0;
    }

    menuWindow.close();

    const int MAZE_WIDTH = 15;
    const int MAZE_HEIGHT = 10;
    const int CELL_SIZE = 45;

    sf::RenderWindow window(
        sf::VideoMode(MAZE_WIDTH * CELL_SIZE + 350, MAZE_HEIGHT * CELL_SIZE),
        "Maze Generator"
    );

    Maze maze(MAZE_WIDTH, MAZE_HEIGHT, CELL_SIZE);
    maze.generateRecursive();

    sf::Font font;
    font.loadFromFile("C:/Windows/Fonts/arial.ttf");

    sf::Text text;
    text.setFont(font);
    text.setCharacterSize(14);
    text.setFillColor(sf::Color(220, 220, 240));
    text.setPosition(static_cast<float>(MAZE_WIDTH * CELL_SIZE + 10), 10.0f);

    sf::Text titleText;
    titleText.setFont(font);
    titleText.setCharacterSize(18);
    titleText.setFillColor(sf::Color(47, 84, 158));
    titleText.setPosition(static_cast<float>(MAZE_WIDTH * CELL_SIZE + 10), 30.0f);
    titleText.setString(Rus("Помоги Meepo найти дорогу к братьям!"));

    bool mousePressed = false;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed) {
                switch (event.key.code) {
                case sf::Keyboard::Num1:
                    if (!maze.isAnimating()) maze.generateRecursive();
                    break;
                case sf::Keyboard::Num3:
                    if (!maze.isAnimating()) maze.generateEmpty();
                    break;
                case sf::Keyboard::Num4:
                    if (!maze.isAnimating()) maze.generateFull();
                    break;
                case sf::Keyboard::Space:
                    maze.solveBFS();
                    break;
                case sf::Keyboard::C:
                    if (!maze.isAnimating()) maze.clearPath();
                    break;
                case sf::Keyboard::Escape:
                    window.close();
                    break;
                default:
                    break;
                }
            }

            if (event.type == sf::Event::MouseButtonPressed && !maze.isAnimating()) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    mousePressed = true;
                    maze.handleMouseClick(event.mouseButton.x, event.mouseButton.y, true, false);
                }
                else if (event.mouseButton.button == sf::Mouse::Right) {
                    mousePressed = true;
                    maze.handleMouseClick(event.mouseButton.x, event.mouseButton.y, false, true);
                }
            }

            if (event.type == sf::Event::MouseButtonReleased) {
                mousePressed = false;
            }

            if (event.type == sf::Event::MouseMoved && mousePressed && !maze.isAnimating()) {
                if (sf::Mouse::isButtonPressed(sf::Mouse::Left)) {
                    maze.handleMouseClick(event.mouseMove.x, event.mouseMove.y, true, false);
                }
                else if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
                    maze.handleMouseClick(event.mouseMove.x, event.mouseMove.y, false, true);
                }
            }
        }

        window.clear(sf::Color(25, 25, 35));
        maze.draw(window);

        std::stringstream timeStream;
        timeStream << std::fixed << std::setprecision(2) << maze.getSolveTime();

        std::string info =
            "  УПРАВЛЕНИЕ:  \n"
            "==============\n"
            "[1] Рекурсивный лабиринт\n"
            "[3] Пустой лабиринт\n"
            "[4] Открытый лабиринт\n\n"
            "[Пробел] Найти путь (BFS)\n"
            "[C] Очистить путь\n"
            "[ESC] Выход\n\n"
            "МЫШЬ:\n"
            "Левая кнопка - переключить стену\n"
            "Правая кнопка - удалить стену\n"
            "Кликайте ТОЛЬКО по стенам!\n\n"
            "СТАТИСТИКА:\n"
            "==============\n"
            "  Генерация: " + maze.getGenAlgorithm() + "\n"
            "  Длина пути: " + std::to_string(maze.getPathLength()) + "\n"
            "  Время: " + timeStream.str() + " мс";

        text.setString(Rus(info.c_str()));
        text.setPosition(static_cast<float>(MAZE_WIDTH * CELL_SIZE + 10), 80.0f);

        window.draw(titleText);
        window.draw(text);

        window.display();
    }

    return 0;
}