#pragma once

#include "Config.h"
#include "OverlayUI.h"
#include "GridAnalyzer.h"
#include "PortalManager.h"
#include "WindowManager.h"
#include "Renderer.h"
#include "RedirectionHelper.h"

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

                POINT globalPt; GetCursorPos(&globalPt);
                POINT clientPt = globalPt; ScreenToClient(gameHwnd, &clientPt);

                RECT rect; GetClientRect(gameHwnd, &rect);
                int w = rect.right, h = rect.bottom;

                if (w > 0 && h > 0) {
                    initOverlay(globalPt.x - clientPt.x, globalPt.y - clientPt.y, w, h);
                    MoveWindow(overlayHwnd, globalPt.x - clientPt.x, globalPt.y - clientPt.y, w, h, TRUE);
                    ui.updateLayout(w, h);

                    POINT overlayPt = globalPt; ScreenToClient(overlayHwnd, &overlayPt);
                    bool overMenu = ui.isInteractivePoint(cv::Point(overlayPt.x, overlayPt.y));

                    LONG exStyle = GetWindowLong(overlayHwnd, GWL_EXSTYLE);
                    if (overMenu) {
                        if (exStyle & WS_EX_TRANSPARENT) SetWindowLong(overlayHwnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);
                    } else {
                        if (!(exStyle & WS_EX_TRANSPARENT)) SetWindowLong(overlayHwnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
                    }

                    cv::Mat overlay = cv::Mat::zeros(h, w, CV_8UC4);
                    if (grid.valid) {
                        if (config.showGrid) Renderer::drawGrid(overlay, grid, w, h, cv::Scalar(0, 165, 255, 255), 1);

                        std::vector<SnappedPortal> activePortals = portalManager.getActivePortals(grid);
                        bool isPreviewMode = config.bindPreview.isPressed();

                        if (isPreviewMode) {
                            cv::Point2d pD((double)clientPt.x, (double)clientPt.y);
                            cv::Point2d uv = GridMath::screenToGridD(grid, pD);
                            int hU = (int)std::round(uv.x);
                            int hV = (int)std::round(uv.y);

                            bool exists = false;
                            SnappedPortal entrance;
                            for (auto& p : activePortals) {
                                if (p.cell.u == hU && p.cell.v == hV) {
                                    exists = true; entrance = p; break;
                                }
                            }

                            std::vector<SnappedPortal> previewNetwork = activePortals;

                            if (!exists) {
                                if (previewNetwork.size() == 4) {
                                    for (auto it = previewNetwork.begin(); it != previewNetwork.end(); ) {
                                        if (it->number == 1) it = previewNetwork.erase(it);
                                        else { it->number -= 1; it++; }
                                    }
                                }

                                entrance.valid = true;
                                entrance.cell.u = hU;
                                entrance.cell.v = hV;
                                entrance.cell.center = GridMath::gridToScreen(grid, hU, hV);
                                entrance.number = previewNetwork.size() + 1;
                                entrance.state = PortalState::OPEN;

                                previewNetwork.push_back(entrance);
                            }

                            std::vector<SnappedPortal> path = RedirectionHelper::calculatePath(previewNetwork, entrance);

                            for (const auto& sp : previewNetwork) {
                                cv::Scalar color;
                                if (sp.number == 1) color = cv::Scalar(0, 0, 255, 255);
                                else if (sp.number == 2) color = cv::Scalar(0, 85, 255, 255);
                                else color = cv::Scalar(0, 165, 255, 255);
                                if (sp.state == PortalState::CLOSED) color = cv::Scalar(128, 128, 128, 255);

                                Renderer::drawDiamond(overlay, sp.cell.center, grid.tileWidth, grid.tileHeight, color, 3);
                                Renderer::drawPortalNumber(overlay, sp.number, sp.cell.center, color);
                            }

                            Renderer::drawRedirectionPath(overlay, path, grid);

                        } else {
                            if (config.showDistances) Renderer::drawDistances(overlay, activePortals, cv::Scalar(255, 255, 0, 255));

                            for (const auto& sp : activePortals) {
                                cv::Scalar color;
                                if (sp.number == 1) color = cv::Scalar(0, 0, 255, 255);
                                else if (sp.number == 2) color = cv::Scalar(0, 85, 255, 255);
                                else color = cv::Scalar(0, 165, 255, 255);
                                if (sp.state == PortalState::CLOSED) color = cv::Scalar(128, 128, 128, 255);

                                Renderer::drawDiamond(overlay, sp.cell.center, grid.tileWidth, grid.tileHeight, color, 3);
                                Renderer::drawPortalNumber(overlay, sp.number, sp.cell.center, color);
                            }
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
            if (cv::waitKey(10) == 27) break;
        }
        cv::destroyAllWindows();
    }
};