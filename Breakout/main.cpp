#include <filesystem>
#include <SFML/Graphics.hpp>
#include "GameManager.h"
#include <print>

#include "LlamaManager.h"

int main()
{
	LlamaManager llamaManager;
	llamaManager.cacheDeps();
	llamaManager.launchServer();

	sf::RenderWindow window(sf::VideoMode(1000, 800), "Breakout");
	GameManager gameManager(&window);
	gameManager.initialize();

	sf::Clock clock;
	float deltaTime;

	constexpr float fixedStepDeltaTime = 1.0f / 60.0f;

	bool warnedServerInactive = false;
	
	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}

		deltaTime = clock.restart().asSeconds();
		// std::println("dt: {} sec", deltaTime);

		auto action = PaddleAction::None;

		if (llamaManager.isServerActive())
		{
			if (warnedServerInactive) warnedServerInactive = false;
			
			action = llamaManager.requestDecision(nullptr);
		}
		else
		{
			if (!warnedServerInactive)
			{
				std::println("Llama server not active? Can't poll decision!");
				warnedServerInactive = true;
			}

			// keyboard override
			if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) action = PaddleAction::Left;
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) action = PaddleAction::Right;
		}

		gameManager.update(deltaTime, action);

		window.clear();
		gameManager.render();
		window.display();
	}

	return 0;
}
