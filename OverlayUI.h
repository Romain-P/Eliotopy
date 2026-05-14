#pragma once

#include "Types.h"
#include "Config.h"
#include <windowsx.h>
#include <array>
#include <cmath>

class OverlayUI {
public:
    Config* config = nullptr;
    bool menuOpen = false;
    bool showWelcomeScreen = true;
    bool quitRequested = false;
    cv::Point mousePos = cv::Point(-1, -1);
    int width = 0;
    int height = 0;
    int capturingBindIndex = -1;
    bool waitingForRelease = false;

    cv::Point menuAnchor = cv::Point(-1, -1);
    bool isDragging = false;
    cv::Point dragStartPos;
    cv::Point dragOffset;
    bool hasMovedDuringDrag = false;

    cv::Rect arrowRect, panelRect, showGridRect, showDistancesRect, exitRect;
    std::array<cv::Rect, 8> bindRects;

    void updateLayout(int w, int h) {
        width = w; height = h;

        if (menuAnchor.x == -1) {
            if (config && config->menuX != -1 && config->menuY != -1) {
                menuAnchor = cv::Point(config->menuX, config->menuY);
            } else {
                menuAnchor = cv::Point(w / 2, 4);
            }
        }

        menuAnchor.x = std::max(30, std::min(w - 30, menuAnchor.x));
        menuAnchor.y = std::max(0, std::min(h - 430, menuAnchor.y));

        int cx = menuAnchor.x;
        int cy = menuAnchor.y;

        arrowRect = cv::Rect(cx - 30, cy, 60, 24);
        panelRect = cv::Rect(cx - 120, cy + 26, 240, 397);

        showGridRect = cv::Rect(panelRect.x + 12, panelRect.y + 12, panelRect.width - 24, 30);
        showDistancesRect = cv::Rect(panelRect.x + 12, panelRect.y + 47, panelRect.width - 24, 30);

        for(int i = 0; i < 8; i++) {
            bindRects[i] = cv::Rect(panelRect.x + 12, panelRect.y + 82 + (i * 32), panelRect.width - 24, 28);
        }

        exitRect = cv::Rect(panelRect.x + 12, panelRect.y + panelRect.height - 40, panelRect.width - 24, 30);
    }

    bool contains(const cv::Rect& r, const cv::Point& p) const {
        return p.x >= r.x && p.y >= r.y && p.x < r.x + r.width && p.y < r.y + r.height;
    }

    bool isInteractivePoint(const cv::Point& p) const {
        if (showWelcomeScreen || isDragging) return true;
        return contains(arrowRect, p) || (menuOpen && contains(panelRect, p));
    }

    void drawMenuText(cv::Mat& img, const std::string& text, cv::Point pos, cv::Scalar color) const {
        cv::putText(img, text, pos, cv::FONT_HERSHEY_SIMPLEX, 0.42, cv::Scalar(25, 25, 25, 255), 3, cv::LINE_AA);
        cv::putText(img, text, pos, cv::FONT_HERSHEY_SIMPLEX, 0.42, color, 1, cv::LINE_AA);
    }

    void drawButton(cv::Mat& img, const cv::Rect& rect, const std::string& label, const std::string& value, bool hovered, bool isCapturing, cv::Scalar customColor = cv::Scalar(-1)) const {
        cv::Scalar bg = hovered ? cv::Scalar(70, 70, 70, 255) : cv::Scalar(45, 45, 45, 255);
        cv::Scalar border = cv::Scalar(120, 120, 120, 255);
        cv::Scalar textColor = cv::Scalar(230, 230, 230, 255);

        if (customColor[0] != -1) {
            bg = hovered ? customColor * 0.8 : customColor * 0.6;
            border = customColor;
            textColor = cv::Scalar(255, 255, 255, 255);
        } else if (isCapturing) {
            bg = cv::Scalar(0, 100, 180, 255);
            border = cv::Scalar(0, 200, 255, 255);
            textColor = cv::Scalar(255, 255, 255, 255);
        }

        cv::rectangle(img, rect, bg, -1, cv::LINE_AA);
        cv::rectangle(img, rect, border, 1, cv::LINE_AA);

        drawMenuText(img, label, cv::Point(rect.x + 8, rect.y + 19), textColor);

        if (!value.empty()) {
            int baseline = 0;
            cv::Size sz = cv::getTextSize(value, cv::FONT_HERSHEY_SIMPLEX, 0.42, 1, &baseline);
            drawMenuText(img, value, cv::Point(rect.x + rect.width - sz.width - 8, rect.y + 19), cv::Scalar(0, 230, 255, 255));
        }
    }

    void render(cv::Mat& img) {
        if (showWelcomeScreen) return;

        bool arrowHovered = contains(arrowRect, mousePos) || isDragging;
        cv::Scalar arrowBg = arrowHovered ? cv::Scalar(85, 85, 85, 255) : (menuOpen ? cv::Scalar(65, 65, 65, 255) : cv::Scalar(40, 40, 40, 255));

        cv::rectangle(img, arrowRect, arrowBg, -1, cv::LINE_AA);
        cv::rectangle(img, arrowRect, cv::Scalar(0, 165, 255, 255), 1, cv::LINE_AA);

        int cx = arrowRect.x + arrowRect.width / 2;
        int cy = arrowRect.y + arrowRect.height / 2;
        std::vector<cv::Point> arrow = menuOpen ?
                                       std::vector<cv::Point>{cv::Point(cx - 8, cy + 4), cv::Point(cx + 8, cy + 4), cv::Point(cx, cy - 6)} :
                                       std::vector<cv::Point>{cv::Point(cx - 8, cy - 4), cv::Point(cx + 8, cy - 4), cv::Point(cx, cy + 6)};
        cv::fillConvexPoly(img, arrow, cv::Scalar(0, 165, 255, 255), cv::LINE_AA);

        if (!menuOpen || !config) return;

        cv::rectangle(img, panelRect, cv::Scalar(35, 35, 35, 255), -1, cv::LINE_AA);
        cv::rectangle(img, panelRect, cv::Scalar(0, 165, 255, 255), 1, cv::LINE_AA);

        drawButton(img, showGridRect, "Show Grid", config->showGrid ? "ON" : "OFF", contains(showGridRect, mousePos), false);
        drawButton(img, showDistancesRect, "Show Distances", config->showDistances ? "ON" : "OFF", contains(showDistancesRect, mousePos), false);

        std::string labels[8] = { "Next Portal", "Clear All", "Preview Next", "Show Cells", "Portal 1", "Portal 2", "Portal 3", "Portal 4" };
        KeyBind* binds[8] = { &config->bindNextPortal, &config->bindClear, &config->bindPreviewNext, &config->bindShowCells, &config->bindPortal[1], &config->bindPortal[2], &config->bindPortal[3], &config->bindPortal[4] };

        for (int i = 0; i < 8; i++) {
            std::string valStr = (capturingBindIndex == i) ? "PRESS KEY..." : binds[i]->getDisplayName();
            drawButton(img, bindRects[i], labels[i], valStr, contains(bindRects[i], mousePos), capturingBindIndex == i);
        }

        drawButton(img, exitRect, "Exit Eliotopy", "", contains(exitRect, mousePos), false, cv::Scalar(0, 0, 180, 255));
    }

    void processKeyCapture() {
        if (capturingBindIndex == -1) return;

        bool anyPressed = false;
        for (int vk = 1; vk < 256; vk++) {
            if (GetAsyncKeyState(vk) & 0x8000) { anyPressed = true; break; }
        }

        if (waitingForRelease && !anyPressed) {
            waitingForRelease = false;
        } else if (!waitingForRelease) {
            for (int vk = 1; vk < 256; vk++) {
                if (vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT ||
                    vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL ||
                    vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU ||
                    vk == VK_LWIN || vk == VK_RWIN) continue;

                if (GetAsyncKeyState(vk) & 0x8000) {
                    if (vk == VK_ESCAPE) { capturingBindIndex = -1; break; }

                    KeyBind newBind;
                    newBind.vkCode = vk;
                    newBind.shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                    newBind.ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
                    newBind.alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

                    if (capturingBindIndex == 0) config->bindNextPortal = newBind;
                    else if (capturingBindIndex == 1) config->bindClear = newBind;
                    else if (capturingBindIndex == 2) config->bindPreviewNext = newBind;
                    else if (capturingBindIndex == 3) config->bindShowCells = newBind;
                    else if (capturingBindIndex >= 4 && capturingBindIndex <= 7)
                        config->bindPortal[capturingBindIndex - 3] = newBind;

                    config->save();
                    capturingBindIndex = -1;
                    break;
                }
            }
        }
    }
};

inline OverlayUI* g_overlayUI = nullptr;

inline LRESULT CALLBACK globalOverlayProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_NCHITTEST) {
        if (!g_overlayUI) return HTTRANSPARENT;
        POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        ScreenToClient(hwnd, &pt);
        if (g_overlayUI->isInteractivePoint(cv::Point(pt.x, pt.y))) return HTCLIENT;
        return HTTRANSPARENT;
    }
    if (msg == WM_MOUSEMOVE) {
        if (g_overlayUI) {
            cv::Point p(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            g_overlayUI->mousePos = p;

            if (g_overlayUI->isDragging) {
                if (std::abs(p.x - g_overlayUI->dragStartPos.x) > 3 || std::abs(p.y - g_overlayUI->dragStartPos.y) > 3) {
                    g_overlayUI->hasMovedDuringDrag = true;
                }
                g_overlayUI->menuAnchor = cv::Point(p.x + g_overlayUI->dragOffset.x, p.y + g_overlayUI->dragOffset.y);
            } else {
                TRACKMOUSEEVENT tme = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hwnd, 0 };
                TrackMouseEvent(&tme);
            }
        }
        return 0;
    }
    if (msg == WM_MOUSELEAVE) {
        if (g_overlayUI && !g_overlayUI->isDragging) {
            g_overlayUI->mousePos = cv::Point(-1, -1);
        }
        return 0;
    }
    if (msg == WM_LBUTTONDOWN) {
        if (!g_overlayUI) return 0;

        if (g_overlayUI->showWelcomeScreen) {
            g_overlayUI->showWelcomeScreen = false;
            return 0;
        }

        cv::Point p(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));

        if (g_overlayUI->contains(g_overlayUI->arrowRect, p)) {
            g_overlayUI->isDragging = true;
            g_overlayUI->dragStartPos = p;
            g_overlayUI->dragOffset = cv::Point(g_overlayUI->menuAnchor.x - p.x, g_overlayUI->menuAnchor.y - p.y);
            g_overlayUI->hasMovedDuringDrag = false;
            SetCapture(hwnd);
            return 0;
        }

        if (!g_overlayUI->menuOpen) return 0;

        if (g_overlayUI->contains(g_overlayUI->exitRect, p)) {
            g_overlayUI->quitRequested = true;
            return 0;
        }

        if (!g_overlayUI->config || g_overlayUI->capturingBindIndex != -1) return 0;

        if (g_overlayUI->contains(g_overlayUI->showGridRect, p)) {
            g_overlayUI->config->showGrid = !g_overlayUI->config->showGrid;
            g_overlayUI->config->save();
        } else if (g_overlayUI->contains(g_overlayUI->showDistancesRect, p)) {
            g_overlayUI->config->showDistances = !g_overlayUI->config->showDistances;
            g_overlayUI->config->save();
        } else {
            for (int i = 0; i < 8; i++) {
                if (g_overlayUI->contains(g_overlayUI->bindRects[i], p)) {
                    g_overlayUI->capturingBindIndex = i;
                    g_overlayUI->waitingForRelease = true;
                    break;
                }
            }
        }
        return 0;
    }
    if (msg == WM_LBUTTONUP) {
        if (g_overlayUI && g_overlayUI->isDragging) {
            g_overlayUI->isDragging = false;
            ReleaseCapture();
            if (!g_overlayUI->hasMovedDuringDrag) {
                g_overlayUI->menuOpen = !g_overlayUI->menuOpen;
            } else if (g_overlayUI->config) {
                g_overlayUI->config->menuX = g_overlayUI->menuAnchor.x;
                g_overlayUI->config->menuY = g_overlayUI->menuAnchor.y;
                g_overlayUI->config->save();
            }
            return 0;
        }
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}