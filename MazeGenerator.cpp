#pragma execution_character_set("utf-8")
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
#include <iostream>
#include <fstream>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#else
#include <unistd.h>
#define _getcwd getcwd
#define SetConsoleOutputCP(x)
#define SetConsoleCP(x)
#define GetConsoleWindow() nullptr
#define ShowWindow(a, b)
#endif

#ifdef _WIN32
const std::string FONT_PATH = "C:/Windows/Fonts/consola.ttf";
#else
const std::string FONT_PATH = "/usr/share/fonts/TTF/DejaVuSans.ttf";
#endif

sf::String Rus(const char* text) {
    #ifdef _WIN32
        int len = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
        wchar_t* wtext = new wchar_t[len];
        MultiByteToWideChar(CP_UTF8, 0, text, -1, wtext, len);
        sf::String result(wtext);
        delete[] wtext;
        return result;
    #else
        return sf::String::fromUtf8(text, text + std::strlen(text));
    #endif
}
void setAdaptiveFontSize(sf::Text& text, float maxWidth, int defaultSize = 16, int minSize = 8) {
    int size = defaultSize;
    text.setCharacterSize(size);
    while (size > minSize && text.getLocalBounds().width > maxWidth - 40) {
        size -= 1;
        text.setCharacterSize(size);
    }
}

// КЛАСС ЛЕНТЫ ПУТИ

class PathRibbon {
private:
    sf::VertexArray ribbon, glow;
    float progress;
    std::vector<std::pair<int, int>> pathCells;
    int cellSize;
    const float thickness = 12.0f;

public:
    PathRibbon(int cellSize) : cellSize(cellSize), progress(0.0f) {}

    void setPath(const std::vector<std::pair<int, int>>& path, float prog) {
        pathCells = path;
        progress = prog;
        updateRibbon();
    }

    void clear() {
        pathCells.clear();
        progress = 0.0f;
        ribbon.clear();
        glow.clear();
    }

    void updateRibbon() {
        if (pathCells.empty() || progress <= 0.001f) return;

        float totalSegments = pathCells.size() - 1;
        float visibleSegments = progress * totalSegments;
        if (visibleSegments < 0.01f) return;

        std::vector<sf::Vector2f> points;
        for (int i = 0; i <= (int)visibleSegments && i < (int)pathCells.size(); i++) {
            int x = pathCells[i].first, y = pathCells[i].second;
            points.push_back(sf::Vector2f(x * cellSize + cellSize / 2.0f, y * cellSize + cellSize / 2.0f));
        }

        if (visibleSegments < totalSegments && (int)visibleSegments < (int)pathCells.size() - 1) {
            int idx = (int)visibleSegments;
            float t = visibleSegments - idx;
            int x1 = pathCells[idx].first, y1 = pathCells[idx].second;
            int x2 = pathCells[idx + 1].first, y2 = pathCells[idx + 1].second;
            points.push_back(sf::Vector2f((x1 + (x2 - x1) * t) * cellSize + cellSize / 2.0f,
                (y1 + (y2 - y1) * t) * cellSize + cellSize / 2.0f));
        }

        ribbon.clear(); glow.clear();
        ribbon.setPrimitiveType(sf::TriangleStrip);
        glow.setPrimitiveType(sf::TriangleStrip);

        for (size_t i = 0; i < points.size(); i++) {
            sf::Vector2f pos = points[i], dir;
            if (i == 0 && points.size() > 1) dir = points[1] - pos;
            else if (i == points.size() - 1 && points.size() > 1) dir = pos - points[i - 1];
            else if (points.size() > 2) dir = points[i + 1] - points[i - 1];
            else dir = sf::Vector2f(1, 0);

            float len = std::hypot(dir.x, dir.y);
            if (len > 0.001f) dir /= len;
            sf::Vector2f perp(-dir.y, dir.x);
            float t = (float)i / std::max(1.0f, (float)points.size() - 1);

            sf::Color color;
            if (t < 0.5f) {
                float tt = t / 0.5f;
                color = sf::Color(50 + 100 * tt, 150 + 100 * tt, 255, 220);
            }
            else {
                float tt = (t - 0.5f) / 0.5f;
                color = sf::Color(150 + 105 * tt, 150 - 70 * tt, 255 - 155 * tt, 220);
            }
            sf::Color glowColor = color; glowColor.a = 50;

            ribbon.append(sf::Vertex(pos + perp * thickness * 0.5f, color));
            ribbon.append(sf::Vertex(pos - perp * thickness * 0.5f, color));
            glow.append(sf::Vertex(pos + perp * thickness * 1.2f, glowColor));
            glow.append(sf::Vertex(pos - perp * thickness * 1.2f, glowColor));
        }
    }

    void setProgress(float prog) { progress = prog; updateRibbon(); }

    void draw(sf::RenderWindow& window) {
        if (pathCells.empty() || progress <= 0.001f) return;
        sf::RenderStates states;
        states.blendMode = sf::BlendAdd;
        window.draw(glow, states);
        states.blendMode = sf::BlendAlpha;
        window.draw(ribbon, states);
    }
};

// структура клетки лабиринта
struct Cell {
    bool walls[4] = { true, true, true, true };
    bool visited = false;
    bool path = false;
    int parentX = -1, parentY = -1;
    float animProgress = 0.0f, targetProgress = 0.0f;
};

// ОСНОВНОЙ КЛАСС ЛАБИРИНТА

class Maze {
private:
    int width, height, cellSize;
    std::vector<std::vector<Cell>> grid;
    std::string currentAlgo = "Классический";
    int lastPathLength = 0;
    float lastSolveTime = 0.0f;
    bool isSolving = false, pathCompleted = false;
    float animDuration = 3.0f, globalProgress = 0.0f;
    sf::Clock animClock, frameTimer, finishTimer;
    std::vector<std::pair<int, int>> pathCells;
    PathRibbon pathRibbon;

    std::vector<sf::Texture> meepoFrames, finishFrames;
    int currentFrame = 0, finishFrame = 0;
    sf::Sprite meepoSprite, finishSprite;
    bool hasMeepo = false, hasFinish = false;

    sf::Font font;
    sf::Text startText, finishText;
    int startX, startY, finishX, finishY;

    const float ANIM_SPEED = 0.02f;
    const int CLICK_MARGIN = 8;

    bool isNear(int x1, int y1, int x2, int y2, int minDist = 5) {
        return abs(x1 - x2) + abs(y1 - y2) < minDist;
    }

    void resetPathData() {
        pathCells.clear();
        globalProgress = 0.0f;
        isSolving = false;
        pathCompleted = false;
        currentFrame = 0;
        frameTimer.restart();
        pathRibbon.clear();
        if (!meepoFrames.empty()) meepoSprite.setTexture(meepoFrames[0]);
        for (auto& row : grid)
            for (auto& cell : row)
                cell.path = false, cell.animProgress = 0.0f, cell.targetProgress = 0.0f;
    }

    void resetGrid() {
        for (auto& row : grid) for (auto& cell : row) cell = Cell();
        resetPathData();
    }

    void carve(int x, int y) {
        grid[y][x].visited = true;
        int dirs[4][3] = { {0,-1,0}, {1,0,1}, {0,1,2}, {-1,0,3} };
        for (int i = 0; i < 4; i++) {
            int r = rand() % 4;
            std::swap(dirs[i][0], dirs[r][0]);
            std::swap(dirs[i][1], dirs[r][1]);
            std::swap(dirs[i][2], dirs[r][2]);
        }
        for (auto& d : dirs) {
            int nx = x + d[0], ny = y + d[1];
            if (nx >= 0 && nx < width && ny >= 0 && ny < height && !grid[ny][nx].visited) {
                grid[y][x].walls[d[2]] = false;
                if (d[2] == 0) grid[ny][nx].walls[2] = false;
                if (d[2] == 1) grid[ny][nx].walls[3] = false;
                if (d[2] == 2) grid[ny][nx].walls[0] = false;
                if (d[2] == 3) grid[ny][nx].walls[1] = false;
                carve(nx, ny);
            }
        }
    }

    void generateClassicImpl() {
        resetGrid();
        std::srand(std::time(nullptr));
        startX = rand() % width;
        startY = rand() % height;
        do { finishX = rand() % width; finishY = rand() % height; } while (isNear(startX, startY, finishX, finishY));
        carve(startX, startY);
        currentAlgo = "Классический";
        lastPathLength = 0; lastSolveTime = 0.0f;
    }

    void generateCorridorImpl() {
        resetGrid();
        startX = 0; startY = height / 2;
        finishX = width - 1; finishY = height / 2;
        currentAlgo = "Прямой путь";
        for (int y = 0; y < height; y++)
            for (int x = 0; x < width; x++)
                for (int w = 0; w < 4; w++) grid[y][x].walls[w] = false;
        for (int x = 0; x < width; x++) grid[0][x].walls[0] = grid[height - 1][x].walls[2] = true;
        for (int y = 0; y < height; y++) grid[y][0].walls[3] = grid[y][width - 1].walls[1] = true;
        lastPathLength = 0; lastSolveTime = 0.0f;
    }

    float calcPathLength() {
        float len = 0;
        for (size_t i = 1; i < pathCells.size(); i++) {
            int x1 = pathCells[i - 1].first, y1 = pathCells[i - 1].second;
            int x2 = pathCells[i].first, y2 = pathCells[i].second;
            len += std::hypot(x2 - x1, y2 - y1) * cellSize;
        }
        return len;
    }

    void findPath() {
        resetPathData();
        std::queue<std::pair<int, int>> q;
        std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
        q.push({ startX, startY });
        visited[startY][startX] = true;
        grid[startY][startX].targetProgress = 1.0f;
        bool found = false;

        while (!q.empty() && !found) {
            int x = q.front().first, y = q.front().second; q.pop();
            if (!grid[y][x].walls[0] && y > 0 && !visited[y - 1][x]) {
                visited[y - 1][x] = true;
                grid[y - 1][x].parentX = x; grid[y - 1][x].parentY = y;
                grid[y - 1][x].targetProgress = 1.0f;
                if (x == finishX && y - 1 == finishY) found = true;
                q.push({ x, y - 1 });
            }
            if (!grid[y][x].walls[1] && x < width - 1 && !visited[y][x + 1]) {
                visited[y][x + 1] = true;
                grid[y][x + 1].parentX = x; grid[y][x + 1].parentY = y;
                grid[y][x + 1].targetProgress = 1.0f;
                if (x + 1 == finishX && y == finishY) found = true;
                q.push({ x + 1, y });
            }
            if (!grid[y][x].walls[2] && y < height - 1 && !visited[y + 1][x]) {
                visited[y + 1][x] = true;
                grid[y + 1][x].parentX = x; grid[y + 1][x].parentY = y;
                grid[y + 1][x].targetProgress = 1.0f;
                if (x == finishX && y + 1 == finishY) found = true;
                q.push({ x, y + 1 });
            }
            if (!grid[y][x].walls[3] && x > 0 && !visited[y][x - 1]) {
                visited[y][x - 1] = true;
                grid[y][x - 1].parentX = x; grid[y][x - 1].parentY = y;
                grid[y][x - 1].targetProgress = 1.0f;
                if (x - 1 == finishX && y == finishY) found = true;
                q.push({ x - 1, y });
            }
        }

        if (visited[finishY][finishX]) {
            int x = finishX, y = finishY;
            while (!(x == startX && y == startY)) {
                pathCells.push_back({ x, y });
                int px = grid[y][x].parentX, py = grid[y][x].parentY;
                if (px == -1) break;
                x = px; y = py;
            }
            pathCells.push_back({ startX, startY });
            std::reverse(pathCells.begin(), pathCells.end());
            lastPathLength = pathCells.size();
            float pathPixels = calcPathLength();
            animDuration = pathPixels / 150.0f;
            if (animDuration < 0.5f) animDuration = 0.5f;
            globalProgress = 0.0f; pathCompleted = false;
            animClock.restart();
        }
    }

    void updateAnimation() {
        bool animating = false;
        for (auto& row : grid)
            for (auto& cell : row)
                if (std::abs(cell.animProgress - cell.targetProgress) > 0.001f) {
                    cell.animProgress += (cell.targetProgress > cell.animProgress) ? ANIM_SPEED : -ANIM_SPEED;
                    if (std::abs(cell.animProgress - cell.targetProgress) < 0.01f) cell.animProgress = cell.targetProgress;
                    animating = true;
                }

        if (!pathCells.empty() && globalProgress < 1.0f) {
            globalProgress = std::min(1.0f, animClock.getElapsedTime().asSeconds() / animDuration);
            if (globalProgress >= 1.0f) {
                pathCompleted = true;
                lastSolveTime = animClock.getElapsedTime().asSeconds() * 1000.0f;
            }
            pathRibbon.setPath(pathCells, globalProgress);
            if (frameTimer.getElapsedTime().asSeconds() > 0.1f && !meepoFrames.empty()) {
                currentFrame = (currentFrame + 1) % meepoFrames.size();
                meepoSprite.setTexture(meepoFrames[currentFrame]);
                frameTimer.restart();
            }
            animating = true;
        }

        if (!finishFrames.empty() && finishTimer.getElapsedTime().asSeconds() > 0.1f) {
            finishFrame = (finishFrame + 1) % finishFrames.size();
            finishSprite.setTexture(finishFrames[finishFrame]);
            finishTimer.restart();
        }
        isSolving = animating;
    }

    sf::Vector2f getHeadPos() {
        if (pathCells.empty() || globalProgress <= 0.001f)
            return sf::Vector2f(startX * cellSize + cellSize / 2.0f, startY * cellSize + cellSize / 2.0f);
        float total = pathCells.size() - 1;
        float pos = globalProgress * total;
        int idx = (int)pos; float t = pos - idx;
        if (idx >= (int)pathCells.size() - 1) {
            int x = pathCells.back().first, y = pathCells.back().second;
            return sf::Vector2f(x * cellSize + cellSize / 2.0f, y * cellSize + cellSize / 2.0f);
        }
        int x1 = pathCells[idx].first, y1 = pathCells[idx].second;
        int x2 = pathCells[idx + 1].first, y2 = pathCells[idx + 1].second;
        return sf::Vector2f((x1 * (1 - t) + x2 * t) * cellSize + cellSize / 2.0f,
            (y1 * (1 - t) + y2 * t) * cellSize + cellSize / 2.0f);
    }

    void loadFrames() {
        for (int i = 1; i <= 30; i++) {
            std::stringstream ss; ss << "frames/frame_" << std::setw(4) << std::setfill('0') << i << ".png";
            sf::Texture tex; if (tex.loadFromFile(ss.str())) meepoFrames.push_back(tex);
        }
        if (!meepoFrames.empty()) {
            float scale = (cellSize - 2) / (float)meepoFrames[0].getSize().x;
            meepoSprite.setScale(scale, scale);
            meepoSprite.setOrigin(meepoFrames[0].getSize().x / 2.0f, meepoFrames[0].getSize().y / 2.0f);
            meepoSprite.setTexture(meepoFrames[0]);
            hasMeepo = true;
        }

        for (int i = 1; i <= 6; i++) {
            std::stringstream ss; ss << "finish_frames/frame_000" << i << ".png";
            sf::Texture tex; if (tex.loadFromFile(ss.str())) finishFrames.push_back(tex);
            else {
                ss.str(""); ss << "double_frames/frame_000" << i << ".png";
                if (tex.loadFromFile(ss.str())) finishFrames.push_back(tex);
            }
        }
        if (!finishFrames.empty()) {
            float scale = (cellSize - 2) / (finishFrames[0].getSize().x / 2.0f);
            finishSprite.setScale(scale, scale);
            finishSprite.setOrigin(finishFrames[0].getSize().x / 2.0f, finishFrames[0].getSize().y / 2.0f);
            finishSprite.setTexture(finishFrames[0]);
            hasFinish = true;
        }
    }

public:
    Maze(int w, int h, int cell = 48) : width(w), height(h), cellSize(cell), pathRibbon(cell) {
        grid.resize(height, std::vector<Cell>(width));
        startX = 0; startY = 0; finishX = width - 1; finishY = height - 1;
        font.loadFromFile(FONT_PATH);
        startText.setFont(font); startText.setString(Rus("СТАРТ")); startText.setCharacterSize(14);
        startText.setFillColor(sf::Color::White); startText.setStyle(sf::Text::Bold);
        finishText.setFont(font); finishText.setString(Rus("ФИНИШ")); finishText.setCharacterSize(14);
        finishText.setFillColor(sf::Color::White); finishText.setStyle(sf::Text::Bold);
        loadFrames();
        generateClassicImpl();
    }

    void generateClassic() { generateClassicImpl(); }
    void generateCorridor() { generateCorridorImpl(); }

    void solve() { if (isSolving) return; findPath(); if (!pathCells.empty()) isSolving = true; }
    void clearPath() { resetPathData(); pathRibbon.clear(); }

    void editWall(int mx, int my, bool left, bool right) {
        if (isSolving) return;
        int x = mx / cellSize, y = my / cellSize;
        if (x < 0 || x >= width || y < 0 || y >= height) return;
        int rx = mx % cellSize, ry = my % cellSize;
        bool top = ry < CLICK_MARGIN, bottom = ry > cellSize - CLICK_MARGIN;
        bool leftE = rx < CLICK_MARGIN, rightE = rx > cellSize - CLICK_MARGIN;
        if (!top && !bottom && !leftE && !rightE) return;

        if (left) {
            if (top) { grid[y][x].walls[0] = !grid[y][x].walls[0]; if (y > 0) grid[y - 1][x].walls[2] = grid[y][x].walls[0]; }
            else if (bottom) { grid[y][x].walls[2] = !grid[y][x].walls[2]; if (y < height - 1) grid[y + 1][x].walls[0] = grid[y][x].walls[2]; }
            else if (leftE) { grid[y][x].walls[3] = !grid[y][x].walls[3]; if (x > 0) grid[y][x - 1].walls[1] = grid[y][x].walls[3]; }
            else if (rightE) { grid[y][x].walls[1] = !grid[y][x].walls[1]; if (x < width - 1) grid[y][x + 1].walls[3] = grid[y][x].walls[1]; }
            clearPath();
        }
        else if (right) {
            if (top) { grid[y][x].walls[0] = false; if (y > 0) grid[y - 1][x].walls[2] = false; }
            else if (bottom) { grid[y][x].walls[2] = false; if (y < height - 1) grid[y + 1][x].walls[0] = false; }
            else if (leftE) { grid[y][x].walls[3] = false; if (x > 0) grid[y][x - 1].walls[1] = false; }
            else if (rightE) { grid[y][x].walls[1] = false; if (x < width - 1) grid[y][x + 1].walls[3] = false; }
            clearPath();
        }
    }

    bool save(const std::string& file) {
        std::ofstream f(file); if (!f) return false;
        f << width << " " << height << "\n" << startX << " " << startY << "\n" << finishX << " " << finishY << "\n";
        for (int y = 0; y < height; y++) for (int x = 0; x < width; x++)
            for (int w = 0; w < 4; w++) f << grid[y][x].walls[w] << " \n"[w == 3];
        return true;
    }

    bool load(const std::string& file) {
        std::ifstream f(file); if (!f) return false;
        int w, h; f >> w >> h; if (w != width || h != height) return false;
        f >> startX >> startY >> finishX >> finishY;
        for (int y = 0; y < height; y++) for (int x = 0; x < width; x++)
            for (int ww = 0; ww < 4; ww++) f >> grid[y][x].walls[ww];
        clearPath(); return true;
    }

    int getPathLen() { return lastPathLength; }
    float getSolveTime() { return lastSolveTime; }
    std::string getAlgo() { return currentAlgo; }
    bool isAnimating() { return isSolving; }

    void draw(sf::RenderWindow& window) {
        updateAnimation();

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                float x1 = x * cellSize, y1 = y * cellSize, x2 = x1 + cellSize, y2 = y1 + cellSize;
                sf::RectangleShape wall(sf::Vector2f(cellSize, 6)); wall.setFillColor(sf::Color(55, 65, 85));
                if (grid[y][x].walls[0]) { wall.setPosition(x1, y1); window.draw(wall); }
                if (grid[y][x].walls[2]) { wall.setPosition(x1, y2 - 5); window.draw(wall); }
                wall.setSize(sf::Vector2f(6, cellSize));
                if (grid[y][x].walls[3]) { wall.setPosition(x1, y1); window.draw(wall); }
                if (grid[y][x].walls[1]) { wall.setPosition(x2 - 5, y1); window.draw(wall); }
            }
        }

        pathRibbon.draw(window);

        sf::RectangleShape startRect(sf::Vector2f(cellSize - 6, cellSize - 6));
        startRect.setFillColor(sf::Color(70, 130, 200, 255));
        startRect.setPosition(startX * cellSize + 3, startY * cellSize + 3);
        window.draw(startRect);

        if (hasMeepo && !isSolving && !pathCompleted && (pathCells.empty() || globalProgress <= 0.001f)) {
            if (frameTimer.getElapsedTime().asSeconds() > 0.1f && !meepoFrames.empty()) {
                currentFrame = (currentFrame + 1) % meepoFrames.size();
                meepoSprite.setTexture(meepoFrames[currentFrame]);
                frameTimer.restart();
            }
            meepoSprite.setPosition(startX * cellSize + cellSize / 2.0f, startY * cellSize + cellSize / 2.0f);
            window.draw(meepoSprite);
        }

        startText.setOrigin(startText.getLocalBounds().width / 2, startText.getLocalBounds().height / 2);
        startText.setPosition(startX * cellSize + cellSize / 2.0f, startY * cellSize + cellSize / 2.0f - cellSize / 3.5f);
        window.draw(startText);

        if (hasMeepo && !pathCells.empty() && globalProgress > 0.001f) {
            sf::Vector2f pos = getHeadPos();
            meepoSprite.setPosition(pos.x, pos.y);
            window.draw(meepoSprite);
        }

        bool onFinish = false;
        if (!pathCells.empty() && globalProgress >= 0.99f) {
            sf::Vector2f head = getHeadPos();
            float fx = finishX * cellSize + cellSize / 2.0f, fy = finishY * cellSize + cellSize / 2.0f;
            onFinish = std::hypot(head.x - fx, head.y - fy) < cellSize / 2.0f;
        }
        if (!onFinish) {
            sf::RectangleShape finishRect(sf::Vector2f(cellSize - 6, cellSize - 6));
            finishRect.setFillColor(sf::Color(220, 80, 100, 255));
            finishRect.setPosition(finishX * cellSize + 3, finishY * cellSize + 3);
            window.draw(finishRect);
        }

        if (hasFinish) {
            finishSprite.setPosition(finishX * cellSize + cellSize / 2.0f, finishY * cellSize + cellSize / 2.0f);
            window.draw(finishSprite);
        }

        finishText.setOrigin(finishText.getLocalBounds().width / 2, finishText.getLocalBounds().height / 2);
        finishText.setPosition(finishX * cellSize + cellSize / 2.0f, finishY * cellSize + cellSize / 2.0f - cellSize / 3.5f);
        window.draw(finishText);
    }
};

// ФОНОВЫЙ ЛАБИРИНТ ДЛЯ МЕНЮ

class BackgroundMaze {
private:
    int w = 25, h = 18, cell = 35;
    struct BgCell { bool walls[4] = { true,true,true,true }; };
    std::vector<std::vector<BgCell>> grid;
    sf::Clock regenClock;
    const float REGEN_TIME = 3.0f;

    void generate() {
        grid.assign(h, std::vector<BgCell>(w));
        for (int y = 0; y < h; y++) for (int x = 0; x < w; x++)
            for (int i = 0; i < 4; i++) grid[y][x].walls[i] = (rand() % 100) > 70;
        for (int x = 0; x < w; x++) grid[0][x].walls[0] = grid[h - 1][x].walls[2] = true;
        for (int y = 0; y < h; y++) grid[y][0].walls[3] = grid[y][w - 1].walls[1] = true;
    }

public:
    BackgroundMaze() { generate(); }
    void update() { if (regenClock.getElapsedTime().asSeconds() >= REGEN_TIME) { generate(); regenClock.restart(); } }
    void draw(sf::RenderWindow& win) {
        int ox = (1024 - w * cell) / 2, oy = (768 - h * cell) / 2;
        sf::RectangleShape wall(sf::Vector2f(cell, 5)); wall.setFillColor(sf::Color(100, 100, 100, 40));
        for (int y = 0; y < h; y++) for (int x = 0; x < w; x++) {
            float x1 = ox + x * cell, y1 = oy + y * cell, x2 = x1 + cell, y2 = y1 + cell;
            if (grid[y][x].walls[0]) { wall.setPosition(x1, y1); win.draw(wall); }
            if (grid[y][x].walls[2]) { wall.setPosition(x1, y2 - 3); win.draw(wall); }
            wall.setSize(sf::Vector2f(5, cell));
            if (grid[y][x].walls[3]) { wall.setPosition(x1, y1); win.draw(wall); }
            if (grid[y][x].walls[1]) { wall.setPosition(x2 - 3, y1); win.draw(wall); }
            wall.setSize(sf::Vector2f(cell, 5));
        }
    }
};
// СТАРТОВОЕ МЕНЮ

class StartMenu {
private:
    sf::Font font;
    sf::Text title, playText, info;
    sf::RectangleShape playBtn, bg, shadow;
    BackgroundMaze bgMaze;
    bool hover = false;

public:
    StartMenu() {
        font.loadFromFile(FONT_PATH);
        bg.setSize(sf::Vector2f(1024, 768)); bg.setFillColor(sf::Color(25, 25, 35, 200));
        title.setFont(font); title.setString(Rus("Meepo Maze")); title.setCharacterSize(52);
        title.setFillColor(sf::Color(100, 180, 230)); title.setStyle(sf::Text::Bold);
        title.setOrigin(title.getLocalBounds().width / 2, title.getLocalBounds().height / 2);
        title.setPosition(512, 200);

        shadow.setSize(sf::Vector2f(280, 90)); shadow.setFillColor(sf::Color(0, 0, 0, 80));
        shadow.setOrigin(140, 45); shadow.setPosition(514, 406);
        playBtn.setSize(sf::Vector2f(280, 90)); playBtn.setFillColor(sf::Color(70, 130, 200));
        playBtn.setOutlineThickness(3); playBtn.setOutlineColor(sf::Color(130, 190, 240));
        playBtn.setOrigin(140, 45); playBtn.setPosition(512, 400);

        playText.setFont(font); playText.setString(Rus("ИГРАТЬ")); playText.setCharacterSize(38);
        playText.setFillColor(sf::Color::White); playText.setStyle(sf::Text::Bold);
        playText.setOrigin(playText.getLocalBounds().width / 2, playText.getLocalBounds().height / 2);
        playText.setPosition(512, 400);

        info.setFont(font); info.setString(Rus("Помоги Meepo найти братьев!"));
        info.setCharacterSize(24); info.setFillColor(sf::Color(180, 180, 200));
        info.setOrigin(info.getLocalBounds().width / 2, info.getLocalBounds().height / 2);
        info.setPosition(512, 560);
    }

    bool run(sf::RenderWindow& win) {
        while (win.isOpen()) {
            sf::Event ev;
            while (win.pollEvent(ev)) {
                if (ev.type == sf::Event::Closed) { win.close(); return false; }
                if (ev.type == sf::Event::MouseMoved) {
                    sf::Vector2i m = sf::Mouse::getPosition(win);
                    if (playBtn.getGlobalBounds().contains(m.x, m.y)) {
                        if (!hover) {
                            hover = true; playBtn.setFillColor(sf::Color(100, 170, 230));
                            playBtn.setOutlineColor(sf::Color(180, 220, 255));
                        }
                    }
                    else if (hover) {
                        hover = false; playBtn.setFillColor(sf::Color(70, 130, 200));
                        playBtn.setOutlineColor(sf::Color(130, 190, 240));
                    }
                }
                if (ev.type == sf::Event::MouseButtonPressed && ev.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2i m = sf::Mouse::getPosition(win);
                    if (playBtn.getGlobalBounds().contains(m.x, m.y)) return true;
                }
            }
            bgMaze.update();
            win.clear(); bgMaze.draw(win); win.draw(bg); win.draw(title);
            win.draw(shadow); win.draw(playBtn); win.draw(playText); win.draw(info);
            win.display();
        }
        return false;
    }
};

// ГЛАВНАЯ ФУНКЦИЯ

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    ShowWindow(GetConsoleWindow(), SW_HIDE);

    sf::RenderWindow menuWin(sf::VideoMode(1024, 768), "Meepo Maze", sf::Style::Titlebar | sf::Style::Close);
    sf::Image icon; if (icon.loadFromFile("frames/frame_0001.png")) menuWin.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());

    StartMenu menu;
    if (!menu.run(menuWin)) return 0;
    menuWin.close();

    int W = 15, H = 10, CELL_SIZE = 48, PANEL = 370;

    sf::RenderWindow win(sf::VideoMode(W * CELL_SIZE + PANEL, H * CELL_SIZE), "Meepo Maze", sf::Style::Titlebar | sf::Style::Close);
    win.setFramerateLimit(60);
    if (icon.loadFromFile("frames/frame_0001.png")) win.setIcon(icon.getSize().x, icon.getSize().y, icon.getPixelsPtr());

    Maze maze(W, H, CELL_SIZE);

    sf::Font font; font.loadFromFile(FONT_PATH);
    sf::Text title, info;
    title.setFont(font); title.setString(Rus("Помоги Meepo найти дорогу к братьям!"));
    title.setFillColor(sf::Color(100, 180, 220)); title.setStyle(sf::Text::Bold);
    info.setFont(font); info.setCharacterSize(14); info.setFillColor(sf::Color(220, 220, 240));

    bool mousePressed = false;
    std::string saveFile = "maze_save.txt";

    while (win.isOpen()) {
        sf::Event ev;
        while (win.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) win.close();
            if (ev.type == sf::Event::KeyPressed) {
                switch (ev.key.code) {
                case sf::Keyboard::Num1: if (!maze.isAnimating()) maze.generateClassic(); break;
                case sf::Keyboard::Num2: if (!maze.isAnimating()) maze.generateCorridor(); break;
                case sf::Keyboard::Space: maze.solve(); break;
                case sf::Keyboard::C: maze.clearPath(); break;
                case sf::Keyboard::S: maze.save(saveFile); break;
                case sf::Keyboard::L: maze.load(saveFile); break;
                case sf::Keyboard::Escape: win.close(); break;
                default: break;
                }
            }
            if (ev.type == sf::Event::MouseButtonPressed && !maze.isAnimating()) {
                if (ev.mouseButton.button == sf::Mouse::Left || ev.mouseButton.button == sf::Mouse::Right) {
                    mousePressed = true;
                    maze.editWall(ev.mouseButton.x, ev.mouseButton.y, ev.mouseButton.button == sf::Mouse::Left, ev.mouseButton.button == sf::Mouse::Right);
                }
            }
            if (ev.type == sf::Event::MouseButtonReleased) mousePressed = false;
            if (ev.type == sf::Event::MouseMoved && mousePressed && !maze.isAnimating()) {
                if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
                    maze.editWall(ev.mouseMove.x, ev.mouseMove.y, true, false);
                else if (sf::Mouse::isButtonPressed(sf::Mouse::Right))
                    maze.editWall(ev.mouseMove.x, ev.mouseMove.y, false, true);
            }
        }

        win.clear(sf::Color(20, 22, 27));
        maze.draw(win);

        sf::RectangleShape panel(sf::Vector2f(PANEL, H * CELL_SIZE));
        panel.setFillColor(sf::Color(28, 30, 36));
        panel.setPosition(W * CELL_SIZE, 0);
        win.draw(panel);

        std::string infoStr =
            "  УПРАВЛЕНИЕ:\n"
            "━━━━━━━━━━━━━━━━\n"
            "[1] Классический\n"
            "[2] Прямой путь\n\n"
            "[Пробел] Найти путь BFS\n"
            "[C] Очистить путь\n"
            "[S] Сохранить\n"
            "[L] Загрузить\n"
            "[ESC] Выход\n\n"
            "  МЫШЬ:\n"
            "ЛКМ - переключить стену\n"
            "ПКМ - удалить стену\n\n"
            "  СТАТИСТИКА:\n"
            "━━━━━━━━━━━━━━━━\n"
            "Генерация: " + maze.getAlgo() + "\n"
            "Длина пути: " + std::to_string(maze.getPathLen()) + "\n"
            "Время: " + std::to_string(maze.getSolveTime()) + " мс\n\n"
            "  РАЗМЕР: " + std::to_string(W) + "x" + std::to_string(H);

        info.setString(Rus(infoStr.c_str()));
        info.setPosition(W * CELL_SIZE + 10, 80);

        float titleMaxWidth = PANEL - 20;
        setAdaptiveFontSize(title, titleMaxWidth, 16, 10);
        title.setPosition(W * CELL_SIZE + 10, 10);

        win.draw(title); win.draw(info);
        win.display();
    }
    return 0;
}