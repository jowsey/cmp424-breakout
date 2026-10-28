#include <SFML/Graphics.hpp>
#include "GameManager.h"
#include <iostream>

int main()
{

    sf::RenderWindow window(sf::VideoMode(1000, 800), "Breakout");
    GameManager gameManager(&window);
    gameManager.initialize();

    sf::Clock clock;
    float deltaTime;

	constexpr float fixedStepDeltaTime = 1.0f / 60.0f;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
		}

		deltaTime = clock.restart().asSeconds();
		std::cout << "dt: " << deltaTime << " sec\n";

		// keyboard override
		auto action = PaddleAction::None;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) action = PaddleAction::Left;
		else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) action = PaddleAction::Right;

		// todo model inference
		gameManager.update(deltaTime, action);

		window.clear();
		gameManager.render();
        window.display();
    }

    return 0;
}
