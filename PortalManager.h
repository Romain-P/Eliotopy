#pragma once

#include "Types.h"
#include "Config.h"
#include "GridMath.h"

class PortalManager {
private:
    SnappedPortal portals[5];
    bool prevNext = false;
    bool prevClear = false;
    bool prevP[5] = { false, false, false, false, false };

public:
    PortalManager() {
        for (int i = 0; i < 5; i++) portals[i].valid = false;
    }

    void processInput(const IsoGrid& grid, HWND dofusWindow, Config& config) {
        if (!grid.valid) return;

        POINT pt;
        if (!GetCursorPos(&pt)) return;
        ScreenToClient(dofusWindow, &pt);

        cv::Point2d pD((double)pt.x, (double)pt.y);
        cv::Point2d uv = GridMath::screenToGridD(grid, pD);
        int hoveredU = (int)std::round(uv.x);
        int hoveredV = (int)std::round(uv.y);

        int hoveredExisting = -1;
        for (int i = 1; i <= 4; i++) {
            if (portals[i].valid && portals[i].cell.u == hoveredU && portals[i].cell.v == hoveredV) {
                hoveredExisting = i;
                break;
            }
        }

        bool isClearPressed = config.bindClear.isPressed();
        if (isClearPressed && !prevClear) {
            for (int i = 1; i <= 4; i++) portals[i].valid = false;
            hoveredExisting = -1;
        }
        prevClear = isClearPressed;

        bool isNextPressed = config.bindNextPortal.isPressed();
        if (isNextPressed && !prevNext) {
            if (hoveredExisting != -1) {
                portals[hoveredExisting].state = (portals[hoveredExisting].state == PortalState::OPEN) ? PortalState::CLOSED : PortalState::OPEN;
            } else {
                int targetIndex = -1;
                for (int i = 1; i <= 4; i++) {
                    if (!portals[i].valid) { targetIndex = i; break; }
                }

                if (targetIndex != -1) {
                    portals[targetIndex].valid = true;
                    portals[targetIndex].cell.u = hoveredU;
                    portals[targetIndex].cell.v = hoveredV;
                    portals[targetIndex].number = targetIndex;
                    portals[targetIndex].state = PortalState::OPEN;
                } else {
                    portals[1] = portals[2]; portals[1].number = 1;
                    portals[2] = portals[3]; portals[2].number = 2;
                    portals[3] = portals[4]; portals[3].number = 3;
                    portals[4].valid = true;
                    portals[4].cell.u = hoveredU;
                    portals[4].cell.v = hoveredV;
                    portals[4].number = 4;
                    portals[4].state = PortalState::OPEN;
                }
            }
        }
        prevNext = isNextPressed;

        for (int i = 1; i <= 4; i++) {
            bool isPortalPressed = config.bindPortal[i].isPressed();
            if (isPortalPressed && !prevP[i]) {
                if (hoveredExisting == i) {
                    portals[i].valid = false;
                    hoveredExisting = -1;
                } else {
                    if (hoveredExisting != -1) portals[hoveredExisting].valid = false;
                    portals[i].valid = true;
                    portals[i].cell.u = hoveredU;
                    portals[i].cell.v = hoveredV;
                    portals[i].number = i;
                    portals[i].state = PortalState::OPEN;
                    hoveredExisting = i;
                }
            }
            prevP[i] = isPortalPressed;
        }
    }

    std::vector<SnappedPortal> getActivePortals(const IsoGrid& grid) {
        std::vector<SnappedPortal> active;
        for (int i = 1; i <= 4; i++) {
            if (portals[i].valid) {
                portals[i].cell.center = GridMath::gridToScreen(grid, portals[i].cell.u, portals[i].cell.v);
                active.push_back(portals[i]);
            }
        }
        return active;
    }
};