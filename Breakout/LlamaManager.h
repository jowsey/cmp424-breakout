#pragma once

#define LLAMA_VER "b11946"
#define MODEL_FILE "d1-2B-Q8_0.gguf"
#define MMPROJ_FILE "mmproj-d1-2B-F16.gguf"

#define LLAMA_URL "https://github.com/ggml-org/llama.cpp/releases/download/" LLAMA_VER "/llama-" LLAMA_VER "-bin-win-vulkan-x64.zip"
#define MODEL_URL "https://huggingface.co/LiquidAI/d1-2B-GGUF/resolve/main/" MODEL_FILE "?download=true"
#define MMPROJ_URL "https://huggingface.co/LiquidAI/d1-2B-GGUF/resolve/main/" MMPROJ_FILE "?download=true"

#define SUBPROCESS_IMPLEMENTATION
#include "subprocess/subprocess.h"

#include <filesystem>

#include "GameManager.h"
#include "httplib/httplib.h"

struct DecisionState
{
	std::string screenshotFormat;
	std::string screenshotBase64;
	
	std::string lastDecision;
	sf::Vector2f ballVelocity;
	int livesRemaining;
};

class LlamaManager
{
	subprocess_s llamaSubprocess;
	std::unique_ptr<httplib::Client> httpClient;
	
	std::filesystem::path currentPath;
	std::filesystem::path cachePath;
	std::filesystem::path llamaPath;
	std::filesystem::path llamaServerBinPath;
	std::filesystem::path modelPath;
	std::filesystem::path mmprojPath;

public:
	LlamaManager();
	~LlamaManager();

	void cacheDeps();
	void launchServer();

	bool isServerActive();

	PaddleAction requestDecision(DecisionState state);
};
