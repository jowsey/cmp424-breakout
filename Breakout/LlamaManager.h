#pragma once

#define LLAMA_VER "b11946"

#define SUBPROCESS_IMPLEMENTATION
#include <filesystem>

#include "subprocess/subprocess.h"

class LlamaManager
{
	subprocess_s llamaSubprocess;

	static constexpr std::string_view LlamaUrl = "https://github.com/ggml-org/llama.cpp/releases/download/" LLAMA_VER "/llama-" LLAMA_VER "-bin-win-vulkan-x64.zip";
	static constexpr std::string_view ModelUrl = "https://huggingface.co/LiquidAI/d1-2B-GGUF/resolve/main/d1-2B-Q8_0.gguf?download=true";
	
	std::filesystem::path currentPath;
	std::filesystem::path cachePath;
	std::filesystem::path llamaPath;
	std::filesystem::path llamaServerBinPath;
	std::filesystem::path modelPath;

public:
	LlamaManager();
	~LlamaManager();

	void cacheDeps();
	void launchServer();
};
