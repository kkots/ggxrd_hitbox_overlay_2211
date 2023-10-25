#pragma once
#include <string>

class WinError {
public:
    LPTSTR message = NULL;
    DWORD code = 0;
    WinError();
    void moveFrom(WinError& src) noexcept;
    void copyFrom(const WinError& src);
    WinError(const WinError& src);
    WinError(WinError&& src) noexcept;
    std::string getMessage();
    void clear();
    ~WinError();
    WinError& operator=(const WinError& src);
    WinError& operator=(WinError&& src) noexcept;
};
