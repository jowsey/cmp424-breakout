#include <filesystem>
#include <SFML/Graphics.hpp>
#include "GameManager.h"
#include <print>

#include "LlamaManager.h"
#include "base64/base64.hpp"

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

	sf::Texture screenshotTexture;
	screenshotTexture.create(window.getSize().x, window.getSize().y);

	while (window.isOpen())
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				window.close();
		}

		deltaTime = clock.restart().asSeconds();
		std::println("dt: {} sec", deltaTime);

		auto action = PaddleAction::None;

		if (llamaManager.isServerActive())
		{
			if (warnedServerInactive) warnedServerInactive = false;

			screenshotTexture.update(window);
			sf::Image screenshotImage = screenshotTexture.copyToImage();

			std::string screenshotFormat = "png";

			std::vector<sf::Uint8> memoryBuffer;
			if (!screenshotImage.saveToMemory(memoryBuffer, screenshotFormat))
			{
				throw std::runtime_error("failed to save screenshot image to memory!");
			}

			auto screenshotBase64 = base64::encode_into<std::string>(memoryBuffer.begin(), memoryBuffer.end());

			DecisionState state{
				.screenshotFormat = screenshotFormat,
				.screenshotBase64 = screenshotBase64
			};

			action = llamaManager.requestDecision(state);
			std::println("Got action {}", action == PaddleAction::Left ? "left" : action == PaddleAction::Right ? "right" : "none");
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
