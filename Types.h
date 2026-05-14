#pragma once

#include <windows.h>
#include <opencv2/opencv.hpp>
#include <string>
#include <sstream>
#include <vector>

enum class PortalState { OPEN, CLOSED };

struct GridCell {
    int u = 0;
    int v = 0;
    cv::Point center;
};

struct IsoGrid {
    bool valid = false;
    cv::Point2d origin = cv::Point2d(0.0, 0.0);
    int tileWidth = 110;
    int tileHeight = 55;
    std::vector<GridCell> greenCells;
    int matchedGreenCells = 0;
    int adjacencyLinks = 0;
    double avgSnapError = 0.0;
};

struct SnappedPortal {
    cv::Point detectedPos;
    GridCell cell;
    bool valid = false;
    int number = -1;
    double numberScore = 0.0;
    PortalState state = PortalState::OPEN;
};

struct KeyBind {
    int vkCode = 0;
    bool shift = false;
    bool ctrl = false;
    bool alt = false;

    bool isPressed() const {
        if (vkCode == 0) return false;
        bool k = (GetAsyncKeyState(vkCode) & 0x8000) != 0;
        bool s = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        bool c = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        bool a = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
        return k && (s == shift) && (c == ctrl) && (a == alt);
    }

    std::string toString() const {
        return std::to_string(vkCode) + "," + std::to_string(shift) + "," + std::to_string(ctrl) + "," + std::to_string(alt);
    }

    void fromString(const std::string& str) {
        std::stringstream ss(str);
        std::string item;
        if (std::getline(ss, item, ',')) vkCode = std::stoi(item);
        if (std::getline(ss, item, ',')) shift = std::stoi(item);
        if (std::getline(ss, item, ',')) ctrl = std::stoi(item);
        if (std::getline(ss, item, ',')) alt = std::stoi(item);
    }

    std::string getDisplayName() const {
        if (vkCode == 0) return "NONE";
        std::string name = "";
        if (ctrl) name += "Ctrl+";
        if (alt) name += "Alt+";
        if (shift) name += "Shift+";

        if (vkCode >= '0' && vkCode <= '9') name += (char)vkCode;
        else if (vkCode >= 'A' && vkCode <= 'Z') name += (char)vkCode;
        else if (vkCode >= VK_NUMPAD0 && vkCode <= VK_NUMPAD9) name += "Num" + std::to_string(vkCode - VK_NUMPAD0);
        else if (vkCode == VK_TAB) name += "TAB";
        else if (vkCode == VK_LBUTTON) name += "LClick";
        else if (vkCode == VK_RBUTTON) name += "RClick";
        else if (vkCode == VK_MBUTTON) name += "MClick";
        else if (vkCode == VK_XBUTTON1) name += "Mouse4";
        else if (vkCode == VK_XBUTTON2) name += "Mouse5";
        else if (vkCode == VK_ESCAPE) name += "ESC";
        else if (vkCode == VK_SPACE) name += "SPACE";
        else name += "VK" + std::to_string(vkCode);
        return name;
    }
};