#include <iostream>
#include <vector>
#include <windows.h>
#include <ctime>
#include <cstdlib>
#include <conio.h>
#include <fstream>  // для работы с файлами
#include <string>
#include <iomanip>  // для форматирования вывода
using namespace std;

struct Cell {
    bool visited = false;
    bool wallTop = true;
    bool wallRight = true;
    bool wallBottom = true;
    bool wallLeft = true;
};

class Maze {
private:
    int width;
    int height;
    vector<vector<Cell>> grid;

    void carvePath(int x, int y) {
        grid[y][x].visited = true;

        int directions[4][3] = {
            {0, -1, 0}, // вверх
            {1, 0, 1},  // вправо
            {0, 1, 2},  // вниз
            {-1, 0, 3}  // влево
        };

        for (int i = 0; i < 4; i++) {
            int r = rand() % 4;
            swap(directions[i][0], directions[r][0]);
            swap(directions[i][1], directions[r][1]);
            swap(directions[i][2], directions[r][2]);
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

    bool findPathDFS(int x, int y, int targetX, int targetY) {
        if (x < 0 || x >= width || y < 0 || y >= height || grid[y][x].visited) {
            return false;
        }

        if (x == targetX && y == targetY) {
            grid[y][x].visited = true;
            return true;
        }

        grid[y][x].visited = true;

        if (!grid[y][x].wallTop && findPathDFS(x, y - 1, targetX, targetY)) return true;
        if (!grid[y][x].wallRight && findPathDFS(x + 1, y, targetX, targetY)) return true;
        if (!grid[y][x].wallBottom && findPathDFS(x, y + 1, targetX, targetY)) return true;
        if (!grid[y][x].wallLeft && findPathDFS(x - 1, y, targetX, targetY)) return true;

        return false;
    }

    bool findPathBFS(int startX, int startY, int targetX, int targetY) {
        vector<pair<int, int>> queue;
        vector<vector<pair<int, int>>> parent(height,
            vector<pair<int, int>>(width, { -1, -1 }));
        vector<vector<bool>> visited(height, vector<bool>(width, false));

        queue.push_back({ startX, startY });
        visited[startY][startX] = true;

        int index = 0;
        bool found = false;

        while (index < queue.size() && !found) {
            int x = queue[index].first;
            int y = queue[index].second;
            index++;

            // Вверх
            if (!grid[y][x].wallTop && y > 0 && !visited[y - 1][x]) {
                visited[y - 1][x] = true;
                parent[y - 1][x] = { x, y };

                if (x == targetX && y - 1 == targetY) {
                    found = true;
                    break;
                }
                queue.push_back({ x, y - 1 });
            }

            // Вправо
            if (!grid[y][x].wallRight && x < width - 1 && !visited[y][x + 1]) {
                visited[y][x + 1] = true;
                parent[y][x + 1] = { x, y };

                if (x + 1 == targetX && y == targetY) {
                    found = true;
                    break;
                }
                queue.push_back({ x + 1, y });
            }

            // Вниз
            if (!grid[y][x].wallBottom && y < height - 1 && !visited[y + 1][x]) {
                visited[y + 1][x] = true;
                parent[y + 1][x] = { x, y };

                if (x == targetX && y + 1 == targetY) {
                    found = true;
                    break;
                }
                queue.push_back({ x, y + 1 });
            }

            // Влево
            if (!grid[y][x].wallLeft && x > 0 && !visited[y][x - 1]) {
                visited[y][x - 1] = true;
                parent[y][x - 1] = { x, y };

                if (x - 1 == targetX && y == targetY) {
                    found = true;
                    break;
                }
                queue.push_back({ x - 1, y });
            }
        }

        if (found || visited[targetY][targetX]) {
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    grid[y][x].visited = false;
                }
            }

            int x = targetX;
            int y = targetY;

            while (!(x == startX && y == startY)) {
                grid[y][x].visited = true;

                int prevX = parent[y][x].first;
                int prevY = parent[y][x].second;

                if (prevX == -1 || prevY == -1) break;

                x = prevX;
                y = prevY;
            }

            return true;
        }

        return false;
    }

public:
    Maze(int w, int h) : width(w), height(h) {
        grid.resize(height, vector<Cell>(width));
    }

    int getWidth() const { return width; }
    int getHeight() const { return height; }

    // НОВЫЙ МЕТОД: Сохранение лабиринта в файл
    bool saveToFile(const string& filename) {
        ofstream file(filename);
        if (!file.is_open()) {
            cout << "Ошибка: не удалось создать файл!\n";
            return false;
        }

        // Сохраняем размеры
        file << width << " " << height << endl;

        // Сохраняем состояние всех ячеек
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                Cell& cell = grid[y][x];
                // Упаковываем стены в одно число (битовая маска)
                int walls = 0;
                if (cell.wallTop) walls |= 1;
                if (cell.wallRight) walls |= 2;
                if (cell.wallBottom) walls |= 4;
                if (cell.wallLeft) walls |= 8;
                file << walls << " ";
            }
            file << endl;
        }

        file.close();
        cout << "Лабиринт сохранен в файл: " << filename << endl;
        return true;
    }

    // НОВЫЙ МЕТОД: Загрузка лабиринта из файла
    bool loadFromFile(const string& filename) {
        ifstream file(filename);
        if (!file.is_open()) {
            cout << "Ошибка: файл не найден!\n";
            return false;
        }

        // Загружаем размеры
        file >> width >> height;

        // Изменяем размер сетки
        grid.resize(height, vector<Cell>(width));

        // Загружаем состояние ячеек
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int walls;
                file >> walls;

                // Распаковываем стены
                grid[y][x].wallTop = (walls & 1) != 0;
                grid[y][x].wallRight = (walls & 2) != 0;
                grid[y][x].wallBottom = (walls & 4) != 0;
                grid[y][x].wallLeft = (walls & 8) != 0;
                grid[y][x].visited = false;
            }
        }

        file.close();
        cout << "Лабиринт загружен из файла: " << filename << endl;
        return true;
    }

    // НОВЫЙ МЕТОД: Подсчет длины пути (количество ячеек на пути)
    int countPathLength() {
        int count = 0;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (grid[y][x].visited) {
                    count++;
                }
            }
        }
        return count;
    }

    void generateRecursiveBacktracker() {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                grid[y][x].visited = false;
                grid[y][x].wallTop = true;
                grid[y][x].wallRight = true;
                grid[y][x].wallBottom = true;
                grid[y][x].wallLeft = true;
            }
        }

        srand(time(NULL));
        int startX = rand() % width;
        int startY = rand() % height;
        carvePath(startX, startY);
    }

    void generateEller() {
        srand(time(NULL));

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                grid[y][x].wallTop = (y == 0);
                grid[y][x].wallBottom = (y == height - 1);
                grid[y][x].wallLeft = (x == 0);
                grid[y][x].wallRight = (x == width - 1);
            }
        }

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width - 1; x++) {
                if (rand() % 2 == 0) {
                    grid[y][x].wallRight = false;
                    grid[y][x + 1].wallLeft = false;
                }
            }
        }

        for (int y = 0; y < height - 1; y++) {
            for (int x = 0; x < width; x++) {
                if (rand() % 3 == 0) {
                    grid[y][x].wallBottom = false;
                    grid[y + 1][x].wallTop = false;
                }
            }
        }
    }

    bool solveDFS() {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                grid[y][x].visited = false;
            }
        }
        return findPathDFS(0, 0, width - 1, height - 1);
    }

    bool solveBFS() {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                grid[y][x].visited = false;
            }
        }
        return findPathBFS(0, 0, width - 1, height - 1);
    }

    void display() {
        cout << " ";
        for (int x = 0; x < width; x++) {
            cout << "___";
        }
        cout << " " << endl;

        for (int y = 0; y < height; y++) {
            cout << "|";

            for (int x = 0; x < width; x++) {
                Cell& cell = grid[y][x];

                if (cell.wallBottom) {
                    cout << "___";
                }
                else {
                    cout << "   ";
                }

                if (cell.wallRight) {
                    cout << "|";
                }
                else {
                    cout << " ";
                }
            }
            cout << endl;
        }
    }

    void displayWithSolution() {
        cout << " ";
        for (int x = 0; x < width; x++) {
            cout << "___";
        }
        cout << " " << endl;

        for (int y = 0; y < height; y++) {
            cout << "|";

            for (int x = 0; x < width; x++) {
                Cell& cell = grid[y][x];

                if (cell.visited && !(x == 0 && y == 0) && !(x == width - 1 && y == height - 1)) {
                    if (cell.wallBottom) {
                        cout << "_*_";
                    }
                    else {
                        cout << " * ";
                    }
                }
                else {
                    if (cell.wallBottom) {
                        cout << "___";
                    }
                    else {
                        cout << "   ";
                    }
                }

                if (cell.wallRight) {
                    cout << "|";
                }
                else {
                    cout << " ";
                }
            }
            cout << endl;
        }

        cout << "\nВход: левый верхний угол, Выход: правый нижний угол\n";
        cout << "Путь отмечен символами '*'\n";
    }

    void printInfo() {
        cout << "Размер: " << width << " x " << height << endl;
    }
};

void clearScreen() {
    system("cls");
}

// НОВАЯ ФУНКЦИЯ: Сравнение алгоритмов
void compareAlgorithms(Maze& maze) {
    cout << "=====================================\n";
    cout << "       СРАВНЕНИЕ АЛГОРИТМОВ\n";
    cout << "=====================================\n\n";

    // Клонируем лабиринт для чистоты эксперимента
    Maze mazeDFS = maze;
    Maze mazeBFS = maze;

    // Тестируем DFS
    clock_t start = clock();
    bool dfsFound = mazeDFS.solveDFS();
    clock_t end = clock();
    double dfsTime = double(end - start) / CLOCKS_PER_SEC * 1000; // в миллисекундах
    int dfsLength = mazeDFS.countPathLength();

    // Тестируем BFS
    start = clock();
    bool bfsFound = mazeBFS.solveBFS();
    end = clock();
    double bfsTime = double(end - start) / CLOCKS_PER_SEC * 1000;
    int bfsLength = mazeBFS.countPathLength();

    // Выводим результаты
    cout << left << setw(15) << "Алгоритм"
        << setw(15) << "Время (мс)"
        << setw(15) << "Длина пути"
        << setw(15) << "Найден?" << endl;
    cout << "--------------------------------------------------------\n";

    cout << left << setw(15) << "DFS"
        << setw(15) << fixed << setprecision(3) << dfsTime
        << setw(15) << dfsLength
        << setw(15) << (dfsFound ? "Да" : "Нет") << endl;

    cout << left << setw(15) << "BFS"
        << setw(15) << fixed << setprecision(3) << bfsTime
        << setw(15) << bfsLength
        << setw(15) << (bfsFound ? "Да" : "Нет") << endl;

    cout << "\nВывод: ";
    if (bfsFound && dfsFound) {
        if (bfsLength < dfsLength) {
            cout << "BFS нашел более короткий путь\n";
        }
        else if (bfsLength > dfsLength) {
            cout << "DFS нашел более короткий путь (неожиданно!)\n";
        }
        else {
            cout << "Оба алгоритма нашли пути одинаковой длины\n";
        }

        if (bfsTime < dfsTime) {
            cout << "BFS быстрее на " << dfsTime - bfsTime << " мс\n";
        }
        else {
            cout << "DFS быстрее на " << bfsTime - dfsTime << " мс\n";
        }
    }
}

void showMenu() {
    cout << "=====================================\n";
    cout << "     ГЕНЕРАТОР И РЕШАТЕЛЬ ЛАБИРИНТОВ\n";
    cout << "=====================================\n\n";

    cout << "1. Сгенерировать (Recursive Backtracker)\n";
    cout << "2. Сгенерировать (Алгоритм Эллера)\n";
    cout << "3. Найти путь (DFS)\n";
    cout << "4. Найти путь (BFS - кратчайший)\n";
    cout << "5. Показать текущий лабиринт\n";
    cout << "6. Изменить размер\n";
    cout << "7. СОХРАНИТЬ лабиринт в файл\n";
    cout << "8. ЗАГРУЗИТЬ лабиринт из файла\n";
    cout << "9. СРАВНИТЬ алгоритмы (DFS vs BFS)\n";
    cout << "0. Выход\n\n";
    cout << "Выберите действие: ";
}

int main()
{
    SetConsoleOutputCP(1251);

    int width = 10, height = 10;
    Maze myMaze(width, height);
    bool mazeGenerated = false;
    string filename;

    while (true) {
        clearScreen();
        showMenu();

        char choice = _getch();
        cout << choice << "\n\n";

        switch (choice) {
        case '1':
            myMaze = Maze(width, height);
            myMaze.generateRecursiveBacktracker();
            mazeGenerated = true;
            cout << "Лабиринт сгенерирован алгоритмом Recursive Backtracker!\n\n";
            myMaze.display();
            cout << "\nНажмите любую клавишу...";
            _getch();
            break;

        case '2':
            myMaze = Maze(width, height);
            myMaze.generateEller();
            mazeGenerated = true;
            cout << "Лабиринт сгенерирован алгоритмом Эллера!\n\n";
            myMaze.display();
            cout << "\nНажмите любую клавишу...";
            _getch();
            break;

        case '3':
            if (!mazeGenerated) {
                cout << "Сначала сгенерируйте лабиринт!\n";
            }
            else {
                if (myMaze.solveDFS()) {
                    cout << "Путь найден (DFS)!\n\n";
                    myMaze.displayWithSolution();
                }
                else {
                    cout << "Путь не найден!\n\n";
                    myMaze.display();
                }
            }
            cout << "\nНажмите любую клавишу...";
            _getch();
            break;

        case '4':
            if (!mazeGenerated) {
                cout << "Сначала сгенерируйте лабиринт!\n";
            }
            else {
                if (myMaze.solveBFS()) {
                    cout << "Путь найден (BFS)!\n\n";
                    myMaze.displayWithSolution();
                }
                else {
                    cout << "Путь не найден!\n\n";
                    myMaze.display();
                }
            }
            cout << "\nНажмите любую клавишу...";
            _getch();
            break;

        case '5':
            if (!mazeGenerated) {
                cout << "Сначала сгенерируйте лабиринт!\n";
            }
            else {
                cout << "Текущий лабиринт:\n\n";
                myMaze.display();
            }
            cout << "\nНажмите любую клавишу...";
            _getch();
            break;

        case '6':
            cout << "Введите ширину и высоту (мин: 3, макс: 30 20): ";
            cin >> width >> height;
            if (width < 3) width = 3;
            if (height < 3) height = 3;
            if (width > 30) width = 30;
            if (height > 20) height = 20;

            myMaze = Maze(width, height);
            mazeGenerated = false;
            cout << "Размер изменен на " << width << " x " << height << "\n";
            cout << "Теперь сгенерируйте новый лабиринт!\n";
            cout << "\nНажмите любую клавишу...";
            _getch();
            break;

        case '7': // СОХРАНЕНИЕ
            if (!mazeGenerated) {
                cout << "Сначала сгенерируйте лабиринт!\n";
            }
            else {
                cout << "Введите имя файла (например: maze.txt): ";
                cin >> filename;
                myMaze.saveToFile(filename);
            }
            cout << "\nНажмите любую клавишу...";
            _getch();
            break;

        case '8': // ЗАГРУЗКА
            cout << "Введите имя файла для загрузки: ";
            cin >> filename;
            if (myMaze.loadFromFile(filename)) {
                mazeGenerated = true;
                width = myMaze.getWidth();
                height = myMaze.getHeight();
                cout << "\nЗагруженный лабиринт:\n\n";
                myMaze.display();
            }
            cout << "\nНажмите любую клавишу...";
            _getch();
            break;

        case '9': // СРАВНЕНИЕ
            if (!mazeGenerated) {
                cout << "Сначала сгенерируйте лабиринт!\n";
            }
            else {
                compareAlgorithms(myMaze);
            }
            cout << "\nНажмите любую клавишу...";
            _getch();
            break;

        case '0':
            cout << "До свидания!\n";
            return 0;

        default:
            cout << "Неверный выбор! Попробуйте снова.\n";
            cout << "Нажмите любую клавишу...";
            _getch();
        }
    }

    return 0;
}