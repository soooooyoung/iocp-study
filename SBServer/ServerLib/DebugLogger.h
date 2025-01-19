#pragma once
#include "spdlog/spdlog.h"
#include "spdlog/async.h"

namespace NetworkLib {
	class DebugLogger
	{
	public:
		DebugLogger();
		virtual ~DebugLogger();

		void Initialize(const char* loggerName);
		void InitializeAsync(const char* loggerName = "AsyncLog");

		template<typename... Args>
		void LogAsync(const char* format, Args&&... args)
		{
			mAsyncLogger->info(fmt::runtime(format), std::forward<Args>(args)...);
		}

		template<typename... Args>
		void Log(const char* format, Args&&... args)
		{
			mConsoleLogger->info(fmt::runtime(format), std::forward<Args>(args)...);
			mFileLogger->info(fmt::runtime(format), std::forward<Args>(args)...);
		}

		template<typename... Args>
		void LogError(const char* format, Args&&... args)
		{
			mErrorLogger->error(fmt::runtime(format), std::forward<Args>(args)...);
			mFileLogger->info(fmt::runtime(format), std::forward<Args>(args)...);
		}

	private:
		// Sync Loggers
		std::shared_ptr<spdlog::logger> mConsoleLogger;
		std::shared_ptr<spdlog::logger> mFileLogger;
		std::shared_ptr<spdlog::logger> mErrorLogger;

		// Async Logger
		std::shared_ptr<spdlog::async_logger> mAsyncLogger;
	};
}