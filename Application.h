#pragma once

#include "Config.h"
#include "OverlayUI.h"
#include "GridAnalyzer.h"
#include "PortalManager.h"
#include "WindowManager.h"
#include "Renderer.h"
#include "RedirectionHelper.h"
#include <algorithm>

class Application {
private:
    Config config;
    OverlayUI ui;
    GridAnalyzer gridAnalyzer;
    PortalManager portalManager;
    HWND gameHwnd = NULL;
    HWND overlayHwnd = NULL;

    bool wasBlocking = false;
    bool wasShowCellsPressed = false;
    bool isShowCellsMode = false;
    SnappedPortal targetExitPortal;

    void initOverlay(int x, int y, int w, int h) {
        if (overlayHwnd != NULL) return;
        WNDCLASSA wc = { 0 };
        wc.lpfnWndProc = globalOverlayProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = "DofusOverlay";
        RegisterClassA(&wc);
        overlayHwnd = CreateWindowExA(WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                                      "DofusOverlay", "Dofus Overlay", WS_POPUP,
                                      x, y, w, h, NULL, NULL, wc.hInstance, NULL);

        SetLayeredWindowAttributes(overlayHwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
        ShowWindow(overlayHwnd, SW_SHOWNA);
    }

public:
    void run() {
        if (FindWindowA("DofusOverlay", "Dofus Overlay")) {
            MessageBoxA(NULL, "Eliotopy is already running.", "Information", MB_OK | MB_ICONINFORMATION);
            return;
        }

        DWORD pid = WindowManager::getProcessId("Dofus.exe");
        if (pid == 0) return;
        gameHwnd = WindowManager::getHwnd(pid);
        if (gameHwnd == NULL) return;

        config.load();
        ui.config = &config;
        g_overlayUI = &ui;

        while (!ui.quitRequested) {
            if (IsIconic(gameHwnd)) {
                if (overlayHwnd && IsWindowVisible(overlayHwnd)) ShowWindow(overlayHwnd, SW_HIDE);
                if (cv::waitKey(10) == 27) break;
                continue;
            } else {
                if (overlayHwnd && !IsWindowVisible(overlayHwnd)) ShowWindow(overlayHwnd, SW_SHOWNA);
            }

            cv::Mat frame;
            if (!gridAnalyzer.isLocked()) frame = WindowManager::capture(gameHwnd);

            IsoGrid grid = gridAnalyzer.detect(frame);
            bool currentlyLoading = gridAnalyzer.isLoading();
            bool blockGameInputs = currentlyLoading || ui.showWelcomeScreen;

            if (!blockGameInputs) {
                if (ui.capturingBindIndex != -1) ui.processKeyCapture();
                else portalManager.processInput(grid, gameHwnd, config);
            }

            POINT globalPt; GetCursorPos(&globalPt);
            POINT clientPt = globalPt; ScreenToClient(gameHwnd, &clientPt);
            RECT rect; GetClientRect(gameHwnd, &rect);
            int w = rect.right, h = rect.bottom;

            if (w > 0 && h > 0) {
                initOverlay(globalPt.x - clientPt.x, globalPt.y - clientPt.y, w, h);

                int ox = globalPt.x - clientPt.x;
                int oy = globalPt.y - clientPt.y;
                HWND prev = GetWindow(gameHwnd, GW_HWNDPREV);
                HWND insertAfter = prev;
                UINT flags = SWP_NOACTIVATE;

                if (prev == overlayHwnd) {
                    flags |= SWP_NOZORDER;
                    insertAfter = NULL;
                } else if (prev == NULL) {
                    insertAfter = HWND_TOP;
                }

                SetWindowPos(overlayHwnd, insertAfter, ox, oy, w, h, flags);

                ui.updateLayout(w, h);

                LONG exStyle = GetWindowLong(overlayHwnd, GWL_EXSTYLE);
                if (blockGameInputs != wasBlocking) {
                    if (blockGameInputs) SetLayeredWindowAttributes(overlayHwnd, 0, 180, LWA_ALPHA);
                    else SetLayeredWindowAttributes(overlayHwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
                    wasBlocking = blockGameInputs;
                }

                if (blockGameInputs) {
                    if (exStyle & WS_EX_TRANSPARENT) SetWindowLong(overlayHwnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);
                } else {
                    POINT overlayPt = globalPt; ScreenToClient(overlayHwnd, &overlayPt);
                    if (ui.isInteractivePoint(cv::Point(overlayPt.x, overlayPt.y))) {
                        if (exStyle & WS_EX_TRANSPARENT) SetWindowLong(overlayHwnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);
                    } else {
                        if (!(exStyle & WS_EX_TRANSPARENT)) SetWindowLong(overlayHwnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
                    }
                }

                cv::Mat overlay = cv::Mat::zeros(h, w, CV_8UC4);
                if (ui.showWelcomeScreen) {
                    overlay.setTo(cv::Scalar(15, 15, 15, 255));
                    int cx = w / 2, cy = h / 2, b = 0;
                    cv::Size sz1 = cv::getTextSize("Eliotopy Loaded!", cv::FONT_HERSHEY_DUPLEX, 1.2, 2, &b);
                    cv::putText(overlay, "Eliotopy Loaded!", cv::Point(cx - sz1.width / 2, cy - 50), cv::FONT_HERSHEY_DUPLEX, 1.2, cv::Scalar(0, 215, 255, 255), 2, cv::LINE_AA);
                    cv::Size sz2 = cv::getTextSize("Please launch a fight to calibrate the tactical grid.", cv::FONT_HERSHEY_SIMPLEX, 0.7, 2, &b);
                    cv::putText(overlay, "Please launch a fight to calibrate the tactical grid.", cv::Point(cx - sz2.width / 2, cy), cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(220, 220, 220, 255), 2, cv::LINE_AA);
                    cv::Size sz3 = cv::getTextSize("https://github.com/Romain-P/Eliotopy", cv::FONT_HERSHEY_SIMPLEX, 0.6, 1, &b);
                    cv::putText(overlay, "https://github.com/Romain-P/Eliotopy", cv::Point(cx - sz3.width / 2, cy + 40), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(255, 165, 0, 255), 1, cv::LINE_AA);
                    cv::Size sz4 = cv::getTextSize("(Click anywhere to close this screen)", cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &b);
                    cv::putText(overlay, "(Click anywhere to close this screen)", cv::Point(cx - sz4.width / 2, cy + 80), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(120, 120, 120, 255), 1, cv::LINE_AA);
                }
                else if (currentlyLoading) {
                    overlay.setTo(cv::Scalar(15, 15, 15, 255));
                    int cx = w / 2, cy = h / 2, b = 0;
                    double a = (GetTickCount() % 1500) / 1500.0 * 360.0;
                    cv::ellipse(overlay, cv::Point(cx, cy - 40), cv::Size(30, 30), a, 0, 280, cv::Scalar(0, 200, 255, 255), 4, cv::LINE_AA);
                    cv::ellipse(overlay, cv::Point(cx, cy - 40), cv::Size(30, 30), a, 0, 360, cv::Scalar(60, 60, 60, 255), 1, cv::LINE_AA);
                    std::string t = "CALIBRATING TACTICAL GRID";
                    for (int i = 0; i < (GetTickCount() / 300) % 4; i++) t += ".";
                    cv::Size szT = cv::getTextSize(t, cv::FONT_HERSHEY_DUPLEX, 0.8, 2, &b);
                    cv::putText(overlay, t, cv::Point(cx - szT.width / 2, cy + 30), cv::FONT_HERSHEY_DUPLEX, 0.8, cv::Scalar(0, 215, 255, 255), 2, cv::LINE_AA);
                    cv::Size szS = cv::getTextSize("Please wait a moment... User inputs suspended.", cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, &b);
                    cv::putText(overlay, "Please wait a moment... User inputs suspended.", cv::Point(cx - szS.width / 2, cy + 60), cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(160, 160, 160, 255), 1, cv::LINE_AA);
                }
                else if (grid.valid) {
                    if (config.showGrid) Renderer::drawGrid(overlay, grid, w, h, cv::Scalar(0, 165, 255, 255), 1);
                    std::vector<SnappedPortal> active = portalManager.getActivePortals(grid);

                    bool showCellsPressed = config.bindShowCells.isPressed();
                    if (showCellsPressed && !wasShowCellsPressed) {
                        cv::Point2d pD((double)clientPt.x, (double)clientPt.y);
                        cv::Point2d uv = GridMath::screenToGridD(grid, pD);
                        int hU = (int)std::round(uv.x), hV = (int)std::round(uv.y);

                        bool found = false;
                        SnappedPortal hoveredPortal;
                        for (const auto& p : active) {
                            if (p.cell.u == hU && p.cell.v == hV) {
                                found = true; hoveredPortal = p; break;
                            }
                        }

                        if (found) {
                            if (isShowCellsMode && targetExitPortal.cell.u == hoveredPortal.cell.u && targetExitPortal.cell.v == hoveredPortal.cell.v) {
                                isShowCellsMode = false;
                            } else {
                                isShowCellsMode = true;
                                targetExitPortal = hoveredPortal;
                            }
                        } else {
                            isShowCellsMode = false;
                        }
                    }
                    wasShowCellsPressed = showCellsPressed;

                    if (isShowCellsMode) {
                        bool stillExists = false;
                        for (const auto& p : active) {
                            if (p.cell.u == targetExitPortal.cell.u && p.cell.v == targetExitPortal.cell.v && p.number == targetExitPortal.number) {
                                stillExists = true; break;
                            }
                        }
                        if (!stillExists) isShowCellsMode = false;
                    }

                    if (config.bindPreviewNext.isPressed()) {
                        cv::Point2d pD((double)clientPt.x, (double)clientPt.y);
                        cv::Point2d uv = GridMath::screenToGridD(grid, pD);
                        int hU = (int)std::round(uv.x), hV = (int)std::round(uv.y);

                        SnappedPortal entrance; bool exists = false;
                        for (auto& p : active) if (p.cell.u == hU && p.cell.v == hV) { exists = true; entrance = p; break; }
                        std::vector<SnappedPortal> net = active;

                        if (!exists) {
                            if (net.size() == 4) for (auto it = net.begin(); it != net.end(); ) if (it->number == 1) it = net.erase(it); else { it->number -= 1; it++; }
                            entrance.valid = true; entrance.cell.u = hU; entrance.cell.v = hV;
                            entrance.cell.center = GridMath::gridToScreen(grid, hU, hV);
                            entrance.number = net.size() + 1; entrance.state = PortalState::OPEN;
                            net.push_back(entrance);
                        }

                        std::vector<SnappedPortal> path = RedirectionHelper::calculatePath(net, entrance);

                        for (const auto& sp : net) {
                            cv::Scalar col = (sp.number == 1) ? cv::Scalar(0,0,255,255) : (sp.number == 2) ? cv::Scalar(0,85,255,255) : cv::Scalar(0,165,255,255);
                            if (sp.state == PortalState::CLOSED) col = cv::Scalar(128,128,128,255);
                            Renderer::drawDiamond(overlay, sp.cell.center, grid.tileWidth, grid.tileHeight, col, 2);
                            Renderer::drawPortalNumber(overlay, sp.number, sp.cell.center, col);
                        }

                        Renderer::drawPathWithDistances(overlay, path, grid, GetTickCount());

                        int totalDist = 0;
                        for (size_t i = 0; i < path.size(); i++) {
                            if (i < path.size() - 1) totalDist += GridMath::gridDistance(path[i].cell, path[i+1].cell);
                        }
                        if (totalDist > 0) {
                            Renderer::drawCursorTooltip(overlay, "+" + std::to_string(totalDist * 2) + "%", cv::Point(clientPt.x, clientPt.y), cv::Scalar(255, 255, 0, 255));
                        }
                    }
                    else if (isShowCellsMode) {
                        cv::Point2d pD((double)clientPt.x, (double)clientPt.y);
                        cv::Point2d hoverUv = GridMath::screenToGridD(grid, pD);
                        int hU = (int)std::round(hoverUv.x), hV = (int)std::round(hoverUv.y);

                        bool hoverHasPortal = false;
                        for (const auto& p : active) {
                            if (p.cell.u == hU && p.cell.v == hV) { hoverHasPortal = true; break; }
                        }

                        bool drawSimulatedHover = false;
                        std::vector<SnappedPortal> hoverPath;
                        std::vector<SnappedPortal> hoverNet;

                        cv::Point2d tl = GridMath::screenToGridD(grid, cv::Point2d(0, 0));
                        cv::Point2d tr = GridMath::screenToGridD(grid, cv::Point2d(w, 0));
                        cv::Point2d bl = GridMath::screenToGridD(grid, cv::Point2d(0, h));
                        cv::Point2d br = GridMath::screenToGridD(grid, cv::Point2d(w, h));

                        int minU = std::min({(int)tl.x, (int)tr.x, (int)bl.x, (int)br.x}) - 2;
                        int maxU = std::max({(int)tl.x, (int)tr.x, (int)bl.x, (int)br.x}) + 2;
                        int minV = std::min({(int)tl.y, (int)tr.y, (int)bl.y, (int)br.y}) - 2;
                        int maxV = std::max({(int)tl.y, (int)tr.y, (int)bl.y, (int)br.y}) + 2;

                        for (int u = minU; u <= maxU; u++) {
                            for (int v = minV; v <= maxV; v++) {
                                cv::Point pt = GridMath::gridToScreen(grid, u, v);
                                if (pt.x < -grid.tileWidth || pt.x > w + grid.tileWidth || pt.y < -grid.tileHeight || pt.y > h + grid.tileHeight) continue;

                                bool hasPortal = false;
                                for (const auto& p : active) {
                                    if (p.cell.u == u && p.cell.v == v) { hasPortal = true; break; }
                                }
                                if (hasPortal) continue;

                                std::vector<SnappedPortal> simNetwork = active;
                                bool targetRemoved = false;
                                if (simNetwork.size() == 4) {
                                    for (auto it = simNetwork.begin(); it != simNetwork.end(); ) {
                                        if (it->number == 1) {
                                            if (it->cell.u == targetExitPortal.cell.u && it->cell.v == targetExitPortal.cell.v) targetRemoved = true;
                                            it = simNetwork.erase(it);
                                        } else {
                                            it->number -= 1; it++;
                                        }
                                    }
                                }
                                if (targetRemoved) continue;

                                SnappedPortal entrance;
                                entrance.valid = true; entrance.cell.u = u; entrance.cell.v = v;
                                entrance.cell.center = pt; entrance.number = simNetwork.size() + 1; entrance.state = PortalState::OPEN;
                                simNetwork.push_back(entrance);

                                std::vector<SnappedPortal> path = RedirectionHelper::calculatePath(simNetwork, entrance);
                                if (!path.empty() && path.back().cell.u == targetExitPortal.cell.u && path.back().cell.v == targetExitPortal.cell.v) {
                                    if (u == hU && v == hV && !hoverHasPortal) {
                                        drawSimulatedHover = true;
                                        hoverPath = path;
                                        hoverNet = simNetwork;
                                    } else {
                                        Renderer::drawHatchedDiamond(overlay, pt, grid.tileWidth, grid.tileHeight, cv::Scalar(255, 240, 40, 255), 3);
                                        Renderer::drawDiamond(overlay, pt, grid.tileWidth, grid.tileHeight, cv::Scalar(255, 240, 40, 255), 1);
                                    }
                                }
                            }
                        }

                        std::vector<SnappedPortal>* renderNet = drawSimulatedHover ? &hoverNet : &active;

                        for (const auto& sp : *renderNet) {
                            cv::Scalar col = (sp.number == 1) ? cv::Scalar(0,0,255,255) : (sp.number == 2) ? cv::Scalar(0,85,255,255) : cv::Scalar(0,165,255,255);
                            if (sp.state == PortalState::CLOSED) col = cv::Scalar(128,128,128,255);

                            if (sp.cell.u == targetExitPortal.cell.u && sp.cell.v == targetExitPortal.cell.v) {
                                Renderer::drawPulsingHighlight(overlay, sp.cell.center, grid, GetTickCount(), cv::Scalar(255, 240, 40, 255));
                            } else {
                                if (!drawSimulatedHover) {
                                    std::vector<SnappedPortal> path = RedirectionHelper::calculatePath(*renderNet, sp);
                                    if (!path.empty() && path.back().cell.u == targetExitPortal.cell.u && path.back().cell.v == targetExitPortal.cell.v) {
                                        Renderer::drawDiamond(overlay, sp.cell.center, grid.tileWidth + 6, grid.tileHeight + 6, cv::Scalar(100, 255, 100, 255), 2);
                                    }
                                }
                                Renderer::drawDiamond(overlay, sp.cell.center, grid.tileWidth, grid.tileHeight, col, 2);
                            }
                            Renderer::drawPortalNumber(overlay, sp.number, sp.cell.center, col);
                        }

                        if (drawSimulatedHover) {
                            Renderer::drawPathWithDistances(overlay, hoverPath, grid, GetTickCount());
                            int totalDist = 0;
                            for (size_t i = 0; i < hoverPath.size(); i++) {
                                if (i < hoverPath.size() - 1) totalDist += GridMath::gridDistance(hoverPath[i].cell, hoverPath[i+1].cell);
                            }
                            if (totalDist > 0) {
                                Renderer::drawCursorTooltip(overlay, "+" + std::to_string(totalDist * 2) + "%", cv::Point(clientPt.x, clientPt.y), cv::Scalar(255, 255, 0, 255));
                            }
                        }
                    }
                    else {
                        if (config.showDistances) Renderer::drawDistances(overlay, active, cv::Scalar(230, 220, 50, 255), grid);
                        for (const auto& sp : active) {
                            cv::Scalar col = (sp.number == 1) ? cv::Scalar(0,0,255,255) : (sp.number == 2) ? cv::Scalar(0,85,255,255) : cv::Scalar(0,165,255,255);
                            if (sp.state == PortalState::CLOSED) col = cv::Scalar(128,128,128,255);
                            Renderer::drawDiamond(overlay, sp.cell.center, grid.tileWidth, grid.tileHeight, col, 2);
                            Renderer::drawPortalNumber(overlay, sp.number, sp.cell.center, col);
                        }
                    }
                }
                if (!blockGameInputs) ui.render(overlay);
                HDC hdc = GetDC(overlayHwnd);
                BITMAPINFOHEADER bi = { sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB };
                SetDIBitsToDevice(hdc, 0, 0, w, h, 0, 0, 0, h, overlay.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
                ReleaseDC(overlayHwnd, hdc);
                MSG msg; while (PeekMessage(&msg, overlayHwnd, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessage(&msg); }
            }
            if (cv::waitKey(10) == 27) break;
        }
        cv::destroyAllWindows();
    }
};