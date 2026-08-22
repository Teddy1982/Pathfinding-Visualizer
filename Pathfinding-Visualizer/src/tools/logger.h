#pragma once
#include <cstdio>

// Loggerklasse mit Leveleinstellungsmöglichkeiten für die Ausgabe von Informationen
class Logger {
  public:
    template <typename... Args>
    static void log(unsigned int logLevel, Args ... args) {
      if (logLevel <= mLogLevel) {
        std::printf(args ...);
        std::fflush(stdout);
      }
    }

    static void setLogLevel(unsigned int inLogLevel) {
      inLogLevel <= 9 ? mLogLevel = inLogLevel : mLogLevel = 9;
    }

  private:
    static unsigned int mLogLevel;
};
