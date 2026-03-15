#include <SFML/Graphics.hpp>

int main()
{
    // Создаем окно
    sf::RenderWindow window(sf::VideoMode(800, 600), "SFML 2.6.1");

    // Создаем круг
    sf::CircleShape shape(100.f);
    shape.setFillColor(sf::Color::Green);
    shape.setPosition(350, 250);

    // Главный цикл
    while (window.isOpen())
    {
        // Обрабатываем события
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        // Рисуем
        window.clear();
        window.draw(shape);
        window.display();
    }

    return 0;
}