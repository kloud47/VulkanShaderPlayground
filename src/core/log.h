#pragma once

#include <iostream>
#include <format>
#include <utility>

namespace vsp::log {

	template <typename... Args>
	void info( std::format_string<Args...> fmt, Args&&... args )
	{
		std::cout << "[Info] " << std::format(fmt, std::forward<Args>(args)...) << "\n";
	}

	template <typename... Args>
	void warn( std::format_string<Args...> fmt, Args&&... args )
	{
		std::cout << "[Warn] " << std::format(fmt, std::forward<Args>(args)...) << "\n";
	}

	template <typename... Args>
	void error( std::format_string<Args...> fmt, Args&&... args )
	{
		std::cerr << "[Error] " << std::format(fmt, std::forward<Args>(args)...) << "\n";
	}
}