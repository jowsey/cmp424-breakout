#include "LlamaManager.h"

#include <array>
#include <filesystem>
#include <print>

#include "httplib/httplib.h"
#include "json/json.hpp"


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
		const int dlResult = std::system(std::format("curl -L# --create-dirs {} -o {}", LLAMA_URL, (cachePath / "llama.zip").string()).c_str());
		if (dlResult != 0) throw std::runtime_error("Failed to download llama.cpp!");

		std::println("Unzipping...");
		std::filesystem::create_directories(llamaPath);
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

		std::println("Downloading model to: ", modelPath.generic_string());
		const int dlResult = std::system(std::format("curl -L# --create-dirs {} -o {}", MODEL_URL, modelPath.string()).c_str());
		if (dlResult != 0) throw std::runtime_error("Failed to download model!");
	}

	// mmproj
	if (std::filesystem::exists(mmprojPath))
	{
		std::println("Mmproj already downloaded.");
	}
	else
	{
		std::println("Mmproj not found. Downloading...");

		std::println("Downloading mmproj to: ", mmprojPath.generic_string());
		const int dlResult = std::system(std::format("curl -L# --create-dirs {} -o {}", MMPROJ_URL, mmprojPath.string()).c_str());
		if (dlResult != 0) throw std::runtime_error("Failed to download mmproj!");
	}
}

LlamaManager::LlamaManager()
{
	currentPath = std::filesystem::current_path();
	cachePath = currentPath / "breakout-cache/";
	llamaPath = cachePath / "llama-" LLAMA_VER "/";
	llamaServerBinPath = llamaPath / "llama-server.exe";
	modelPath = cachePath / MODEL_FILE;
	mmprojPath = cachePath / MMPROJ_FILE;
}

LlamaManager::~LlamaManager()
{
	std::println("Terminating llama.cpp server...");
	subprocess_terminate(&llamaSubprocess);
}

void LlamaManager::launchServer()
{
	std::print("Launching llama.cpp server...");

	const std::string binPathStr = llamaServerBinPath.string();
	const std::string modelPathStr = modelPath.string();
	const std::string mmprojPathStr = mmprojPath.string();

	const char* args[] = {
		binPathStr.c_str(),
		"-m", modelPathStr.c_str(),
		"--mmproj", mmprojPathStr.c_str(),
		"--ctx-size", "8192",
		"-b", "8192",
		"-ub", "8192",
		"-np", "1",
		"-ngl", "all",
		"--metrics",
		nullptr
	};

	const int result = subprocess_create(
		args,
		subprocess_option_combined_stdout_stderr | subprocess_option_inherit_environment | subprocess_option_enable_async,
		&llamaSubprocess
	);
	if (result != 0) throw std::runtime_error("Failed to launch llama.cpp server!");

	// read until server listening
	std::array<char, 1024> buffer{};
	std::string accumulator;

	while (true)
	{
		const std::uint32_t numBytes = subprocess_read_stdout(&llamaSubprocess, buffer.data(), buffer.size());

		if (numBytes > 0)
		{
			accumulator.append(buffer.data(), numBytes);

			bool found = false;
			while (accumulator.find('\n') != std::string::npos)
			{
				const auto newlinePos = accumulator.find('\n');

				std::string line = accumulator.substr(0, newlinePos);

				const auto targetPos = line.find("listening on");
				if (targetPos != std::string::npos)
				{
					std::println(" {}", line.substr(targetPos));
					found = true;
					break;
				}

				accumulator.erase(0, newlinePos + 1);
			}

			if (found) break;
		}
		else
		{
			throw std::runtime_error("llama.cpp server process failed to start listening!");
		}
	}

	// test request
	httpClient = std::make_unique<httplib::Client>("http://127.0.0.1:9931");

	const std::string jsonData = R"(
	{
  		"state": "I was charged twice this month, please refund one of them.",
  		"questions": {
  		  "refund":  {"type": "noul", "instructions": "Is the customer asking for a refund?"},
  		  "team":    {"type": "choice", "instructions": "Which team should handle this?",
  		              "criteria": {"billing": "Charges, refunds, invoices", "technical": "App or site faults", "fraud": "Suspected unauthorised use"}},
  		  "urgency": {"type": "score", "instructions": "How urgent is this?",
  		              "criteria": ["Can wait", "Today", "Blocking the customer now"]}
  		}
	})";

	if (const auto res = httpClient->Post("/v1/systemone", jsonData, "application/json"))
	{
		std::println("test decision succeeded");
	}
	else
	{
		std::println("test decision failed, error code: {}", static_cast<int>(res.error()));
	}
}

bool LlamaManager::isServerActive()
{
	return subprocess_alive(&llamaSubprocess);
}

PaddleAction LlamaManager::requestDecision(DecisionState state)
{
	std::string prevScreenshotString = state.prevScreenshotBase64.empty()
		                                   ? ""
		                                   : std::format("\"data:image/{};base64,{}\", ", state.screenshotFormat, state.prevScreenshotBase64);

	const std::string jsonPayload = std::format(R"(
{{
	"state": "You are playing breakout. Move your paddle to hit the blue ball and prevent it falling off the screen. Two attached images are provided for context. The first attached image is a screenshot of the *previous* frame, and the second is a screenshot of the *current* frame, for comparison.",
	"images": [{2}"data:image/{0};base64,{1}"],
	"questions": {{
		"move": {{ "type": "choice", "instructions": "Which direction should the paddle move?", "criteria": {{ "left": "It should move to the left", "right": "It should move to the right", "none": "It should stay still" }} }}
	}}
}}
	)", state.screenshotFormat, state.screenshotBase64, prevScreenshotString);

	// std::println("{}", jsonPayload);

	if (const auto res = httpClient->Post("/v1/systemone", jsonPayload, "application/json"))
	{
		// std::println("Received decision: {}", res->body);

		std::string choice;

		try
		{
			nlohmann::json j = nlohmann::json::parse(res->body);
			choice = j["answers"]["move"]["choice"];
		}
		catch (const std::exception& e)
		{
			std::println("Error parsing decision: {} (body was: {})", e.what(), res->body);
			return PaddleAction::None;
		}

		if (choice == "left")
			return PaddleAction::Left;
		if (choice == "right")
			return PaddleAction::Right;
		if (choice == "none")
			return PaddleAction::None;

		std::println("Model returned unusual choice '{}'!", choice);
		return PaddleAction::None;
	}
	else
	{
		std::println("decision error ({}): {}", static_cast<int>(res.error()), res->body);
		return PaddleAction::None;
	}
}
