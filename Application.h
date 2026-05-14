#pragma once

#include "Config.h"
#include "OverlayUI.h"
#include "GridAnalyzer.h"
#include "PortalManager.h"
#include "WindowManager.h"
#include "Renderer.h"

class Application {
private:
    Config config;
    OverlayUI ui;
    GridAnalyzer gridAnalyzer;
    PortalManager portalManager;
    HWND gameHwnd = NULL;
    HWND overlayHwnd = NULL;

    void initOverlay(int x, int y, int w, int h) {
        if (overlayHwnd != NULL) return;
        WNDCLASSA wc = { 0 };
        wc.lpfnWndProc = globalOverlayProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = "DofusOverlay";
        RegisterClassA(&wc);
        overlayHwnd = CreateWindowExA(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                                      "DofusOverlay", "Dofus Overlay", WS_POPUP,
                                      x, y, w, h, NULL, NULL, wc.hInstance, NULL);
        SetLayeredWindowAttributes(overlayHwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
        ShowWindow(overlayHwnd, SW_SHOW);
    }

public:
    void run() {
        DWORD pid = WindowManager::getProcessId("Dofus.exe");
        if (pid == 0) return;
        gameHwnd = WindowManager::getHwnd(pid);
        if (gameHwnd == NULL) return;

        config.load();
        ui.config = &config;
        g_overlayUI = &ui;

        while (true) {
            cv::Mat frame = WindowManager::capture(gameHwnd);
            if (!frame.empty()) {
                IsoGrid grid = gridAnalyzer.detect(frame);

                if (ui.capturingBindIndex != -1) {
                    ui.processKeyCapture();
                } else {
                    portalManager.processInput(grid, gameHwnd, config);
                }

                POINT pt = { 0, 0 };
                ClientToScreen(gameHwnd, &pt);
                RECT rect; GetClientRect(gameHwnd, &rect);
                int w = rect.right, h = rect.bottom;

                if (w > 0 && h > 0) {
                    initOverlay(pt.x, pt.y, w, h);
                    MoveWindow(overlayHwnd, pt.x, pt.y, w, h, TRUE);
                    ui.updateLayout(w, h);

                    cv::Mat overlay = cv::Mat::zeros(h, w, CV_8UC4);
                    if (grid.valid) {
                        if (config.showGrid) Renderer::drawGrid(overlay, grid, w, h, cv::Scalar(0, 165, 255, 255), 1);
                        std::vector<SnappedPortal> active = portalManager.getActivePortals(grid);
                        if (config.showDistances) Renderer::drawDistances(overlay, active, cv::Scalar(255, 255, 0, 255));

                        for (const auto& sp : active) {
                            cv::Scalar color;
                            if (sp.number == 1) color = cv::Scalar(0, 0, 255, 255);
                            else if (sp.number == 2) color = cv::Scalar(0, 85, 255, 255);
                            else color = cv::Scalar(0, 165, 255, 255);
                            if (sp.state == PortalState::CLOSED) color = cv::Scalar(128, 128, 128, 255);

                            Renderer::drawDiamond(overlay, sp.cell.center, grid.tileWidth, grid.tileHeight, color, 3);
                            Renderer::drawPortalNumber(overlay, sp.number, sp.cell.center, color);
                        }
                    }

                    ui.render(overlay);

                    HDC hdc = GetDC(overlayHwnd);
                    BITMAPINFOHEADER bi = { sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB };
                    SetDIBitsToDevice(hdc, 0, 0, w, h, 0, 0, 0, h, overlay.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
                    ReleaseDC(overlayHwnd, hdc);

                    MSG msg;
                    while (PeekMessage(&msg, overlayHwnd, 0, 0, PM_REMOVE)) {
                        TranslateMessage(&msg); DispatchMessage(&msg);
                    }
                }
            }
        }
        cv::destroyAllWindows();
    }
};