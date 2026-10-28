#include <filesystem>
#include <SFML/Graphics.hpp>
#include "GameManager.h"
#include <print>

int main()
{
	// download llama.cpp runtime & decision model weights
	std::string llamaVer = "b11946";
	std::string llamaUrl = "https://github.com/ggml-org/llama.cpp/releases/download/" + llamaVer + "/llama-" + llamaVer + "-bin-win-vulkan-x64.zip";
	std::string modelUrl = "https://huggingface.co/LiquidAI/d1-2B-GGUF/resolve/main/d1-2B-Q8_0.gguf?download=true";

	std::filesystem::path currentPath = std::filesystem::current_path();
	std::filesystem::path cachePath = currentPath / "breakout-cache/";
	std::filesystem::path llamaPath = cachePath / ("llama-" + llamaVer + "/");
	std::filesystem::path llamaServerBinPath = llamaPath / "llama-server.exe";
	std::filesystem::path modelPath = cachePath / "d1-2B-Q8_0.gguf";

	std::filesystem::create_directories(cachePath);

	// llama.cpp
	if (std::filesystem::exists(llamaServerBinPath))
	{
		std::println("llama.cpp already downloaded.");
	}
	else
	{
		std::println("llama.cpp not found. Downloading...");

		std::println("Downloading llama.cpp to: ", cachePath.generic_string());
		int dlResult = std::system(std::format("curl -L# --create-dirs {} -o {}", llamaUrl, (cachePath / "llama.zip").string()).c_str());
		if (dlResult != 0) throw std::runtime_error("Failed to download llama.cpp!");

		std::println("Unzipping...");
		int unzipResult = std::system(
			std::format("tar -xf {} -C {}", (cachePath / "llama.zip").string(), llamaPath.lexically_relative(currentPath).string()).c_str()
		);
		if (unzipResult != 0) throw std::runtime_error("Failed to unzip llama.cpp!");

		int rmResult = std::system(std::format("rm {}", (cachePath / "llama.zip").string()).c_str());
		if (rmResult != 0) throw std::runtime_error("Failed to remove llama.zip");

		std::println("llama.cpp downloaded and unzipped successfully.");
	}

	// model
	if (std::filesystem::exists(modelPath))
	{
		std::println("Model already downloaded.");
	}
	else
	{
		std::println("Model not found. Downloading...");

		std::println("Downloading model to: ", cachePath.generic_string());
		int dlResult = std::system(std::format("curl -L# --create-dirs {} -o {}", modelUrl, modelPath.string()).c_str());
		if (dlResult != 0) throw std::runtime_error("Failed to download model!");
	}

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
		// std::println("dt: {} sec", deltaTime);

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
