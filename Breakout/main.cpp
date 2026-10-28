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

	std::vector<sf::Uint8> screenshotMemoryBuffer;

	std::string prevAction{"none"};

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

		if (llamaManager.isServerActive() && gameManager.getLives() > 0)
		{
			if (warnedServerInactive) warnedServerInactive = false;

			screenshotTexture.update(window);
			sf::Image screenshotImage = screenshotTexture.copyToImage();

			std::string screenshotFormat = "jpeg";

			screenshotMemoryBuffer.clear();
			if (!screenshotImage.saveToMemory(screenshotMemoryBuffer, screenshotFormat))
			{
				throw std::runtime_error("failed to save screenshot image to memory!");
			}

			auto screenshotBase64 = base64::encode_into<std::string>(screenshotMemoryBuffer.begin(), screenshotMemoryBuffer.end());

			DecisionState state{
				.screenshotFormat = screenshotFormat,
				.screenshotBase64 = screenshotBase64,
				.lastDecision = prevAction,
				.ballVelocity = gameManager.getBall()->getVelocity(),
				.livesRemaining = gameManager.getLives()
			};

			action = llamaManager.requestDecision(state);
			std::string actionStr = action == PaddleAction::Left ? "left" : action == PaddleAction::Right ? "right" : "none";
			std::println("Got action {}", actionStr);
			prevAction = actionStr;
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
