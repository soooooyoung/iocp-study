#include "pch.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"

#include "DebugLogger.h"

namespace NetworkLib
{

	DebugLogger::DebugLogger()
	{
	}

	DebugLogger::~DebugLogger()
	{
	}

	void DebugLogger::Initialize(const char* loggerName)
	{
		// Console
		mConsoleLogger = spdlog::stdout_color_mt(loggerName);

		// ErrorHandler
		mErrorLogger = spdlog::stderr_color_mt("ERROR");

		spdlog::set_error_handler([](const std::string& msg)
			{
				spdlog::get("ERROR")->error("Logger Error: {}", msg);
			});

		//Rotating File 5MB
		mFileLogger = spdlog::rotating_logger_mt("FILE", "logs/" + std::string(loggerName) + ".txt", 1048576 * 5, 3);

		spdlog::set_default_logger(mConsoleLogger);
	}

	void DebugLogger::InitializeAsync(const char* loggerName)
	{
		spdlog::init_thread_pool(8192, 1);

		auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("logs/" + std::string(loggerName) + ".txt", 1048576 * 5, 3);

		std::vector<spdlog::sink_ptr> sinks{ consoleSink, fileSink };

		mAsyncLogger = std::make_shared<spdlog::async_logger>(loggerName, sinks.begin(), sinks.end(), spdlog::thread_pool(), spdlog::async_overflow_policy::block);

		spdlog::register_logger(mAsyncLogger);
		spdlog::set_default_logger(mAsyncLogger);
		spdlog::set_error_handler([](const std::string& msg)
			{
				spdlog::default_logger()->error("Logger Error: {}", msg);
			});
	}
}
