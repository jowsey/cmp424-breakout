#include "LlamaManager.h"

#include <array>
#include <filesystem>
#include <print>
#include <thread>


void LlamaManager::cacheDeps()
{
	// download llama.cpp runtime & decision model weights
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
		const int dlResult = std::system(std::format("curl -L# --create-dirs {} -o {}", LlamaUrl, (cachePath / "llama.zip").string()).c_str());
		if (dlResult != 0) throw std::runtime_error("Failed to download llama.cpp!");

		std::println("Unzipping...");
		const int unzipResult = std::system(
			std::format("tar -xf {} -C {}", (cachePath / "llama.zip").string(), llamaPath.lexically_relative(currentPath).string()).c_str()
		);
		if (unzipResult != 0) throw std::runtime_error("Failed to unzip llama.cpp!");

		const int rmResult = std::system(std::format("rm {}", (cachePath / "llama.zip").string()).c_str());
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
		const int dlResult = std::system(std::format("curl -L# --create-dirs {} -o {}", ModelUrl, modelPath.string()).c_str());
		if (dlResult != 0) throw std::runtime_error("Failed to download model!");
	}
}

LlamaManager::LlamaManager()
{
	const std::string llamaVer = "b11946";
	std::string llamaUrl = "https://github.com/ggml-org/llama.cpp/releases/download/" + llamaVer + "/llama-" + llamaVer + "-bin-win-vulkan-x64.zip";
	std::string modelUrl = "https://huggingface.co/LiquidAI/d1-2B-GGUF/resolve/main/d1-2B-Q8_0.gguf?download=true";

	currentPath = std::filesystem::current_path();
	cachePath = currentPath / "breakout-cache/";
	llamaPath = cachePath / ("llama-" + llamaVer + "/");
	llamaServerBinPath = llamaPath / "llama-server.exe";
	modelPath = cachePath / "d1-2B-Q8_0.gguf";
}

LlamaManager::~LlamaManager()
{
	std::println("Terminating llama.cpp server...");
	subprocess_terminate(&llamaSubprocess);
}

void LlamaManager::launchServer()
{
	std::println("Launching llama.cpp server...");

	const std::string binPathStr = llamaServerBinPath.string();
	const std::string modelPathStr = modelPath.string();

	const char* args[] = {
		binPathStr.c_str(),
		"-m", modelPathStr.c_str(),
		"-b", "8192",
		"-ub", "8192",
		"-c", "16384",
		"-np", "1",
		nullptr
	};

	const int result = subprocess_create(args, subprocess_option_combined_stdout_stderr | subprocess_option_inherit_environment, &llamaSubprocess);
	if (result != 0) throw std::runtime_error("Failed to launch llama.cpp server!");

	// read the output of the server
	while (true)
	{
		std::array<char, 1024> buffer{};
		const unsigned int numBytes = subprocess_read_stdout(&llamaSubprocess, buffer.data(), buffer.size());
		if (numBytes > 0)
		{
			std::print("{}", std::string(buffer.data(), numBytes));
		}
	}
}
