#pragma once

#include <windows.h>
#include <tlhelp32.h>
#include <opencv2/opencv.hpp>
#include <string>

class WindowManager {
public:
    static DWORD getProcessId(const std::string& name) {
        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(PROCESSENTRY32);
        HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snap == INVALID_HANDLE_VALUE) return 0;
        if (Process32First(snap, &pe)) {
            do {
                if (_stricmp(name.c_str(), pe.szExeFile) == 0) {
                    CloseHandle(snap);
                    return pe.th32ProcessID;
                }
            } while (Process32Next(snap, &pe));
        }
        CloseHandle(snap);
        return 0;
    }

    struct EnumData { DWORD pid; HWND hwnd; };

    static BOOL CALLBACK enumCallback(HWND hwnd, LPARAM lParam) {
        EnumData& data = *(EnumData*)lParam;
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (data.pid == pid && IsWindowVisible(hwnd)) {
            char title[256];
            GetWindowTextA(hwnd, title, sizeof(title));
            if (strlen(title) > 0) {
                data.hwnd = hwnd;
                return FALSE;
            }
        }
        return TRUE;
    }

    static HWND getHwnd(DWORD pid) {
        EnumData data = { pid, NULL };
        EnumWindows(enumCallback, (LPARAM)&data);
        return data.hwnd;
    }

    static cv::Mat capture(HWND hwnd) {
        RECT rect;
        GetClientRect(hwnd, &rect);
        int w = rect.right, h = rect.bottom;
        if (w == 0 || h == 0) return cv::Mat();

        cv::Mat src(h, w, CV_8UC4);
        HDC hdcScreen = GetDC(NULL);
        HDC hdcMem = CreateCompatibleDC(hdcScreen);
        HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, w, h);
        HGDIOBJ old = SelectObject(hdcMem, hBitmap);
        PrintWindow(hwnd, hdcMem, 3);

        BITMAPINFOHEADER bi = { sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB };
        GetDIBits(hdcMem, hBitmap, 0, h, src.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

        SelectObject(hdcMem, old);
        DeleteObject(hBitmap);
        DeleteDC(hdcMem);
        ReleaseDC(NULL, hdcScreen);

        cv::Mat finalSrc;
        cv::cvtColor(src, finalSrc, cv::COLOR_BGRA2BGR);
        return finalSrc;
    }
};