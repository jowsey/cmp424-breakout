#pragma once

#define LLAMA_VER "b11946"

#define SUBPROCESS_IMPLEMENTATION
#include "subprocess/subprocess.h"

#include <filesystem>
#include <variant>

#include "GameManager.h"
#include "httplib/httplib.h"
#include "json/json.hpp"

struct Question
{
	std::string type;
	std::string instructions;
};

struct Choice : Question
{
	std::unordered_map<std::string, std::string> criteria;
};

struct DecisionRequestBody
{
	std::string state;
	// std::string image;
	std::unordered_map<std::string, std::variant<Question, Choice>> questions;
};

class LlamaManager
{
	subprocess_s llamaSubprocess;
	httplib::Client httpClient;

	static constexpr std::string_view LlamaUrl = "https://github.com/ggml-org/llama.cpp/releases/download/" LLAMA_VER "/llama-" LLAMA_VER
		"-bin-win-vulkan-x64.zip";
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

	bool isServerActive();

	PaddleAction requestDecision(std::nullptr_t state);
};
