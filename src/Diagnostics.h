#pragma once

namespace Diagnostics {

void Initialize();
void Log(const char* message);
void LogFormat(const char* format, ...);
void Shutdown();

}  // namespace Diagnostics
