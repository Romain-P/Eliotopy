#pragma once

#include "Types.h"
#include <fstream>
#include <string>

class Config {
public:
    bool showGrid = false;
    bool showDistances = false;
    int menuX = -1;
    int menuY = -1;
    std::string configPath;

    KeyBind bindNextPortal = { VK_TAB, false, false, false };
    KeyBind bindClear = { VK_TAB, true, false, false };
    KeyBind bindPreviewNext = { VK_MBUTTON, false, false, false };
    KeyBind bindShowCells = { VK_XBUTTON1, false, false, false };
    KeyBind bindPortal[5];

    static std::string getDirectory() {
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);
        std::string fullPath(path);
        size_t pos = fullPath.find_last_of("\\/");
        if (pos == std::string::npos) return ".";
        return fullPath.substr(0, pos);
    }

    void load() {
        showGrid = true;
        showDistances = true;
        menuX = -1;
        menuY = -1;
        bindPortal[1] = { 0x31, true, false, false };
        bindPortal[2] = { 0x32, true, false, false };
        bindPortal[3] = { 0x33, true, false, false };
        bindPortal[4] = { 0x34, true, false, false };

        configPath = getDirectory() + "\\eliotopy_config.ini";
        std::ifstream file(configPath);
        if (!file.is_open()) { save(); return; }

        std::string line;
        while (std::getline(file, line)) {
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq);
            std::string value = line.substr(eq + 1);

            if (key == "show_grid") showGrid = (value == "1");
            else if (key == "show_distances") showDistances = (value == "1");
            else if (key == "menu_x") menuX = std::stoi(value);
            else if (key == "menu_y") menuY = std::stoi(value);
            else if (key == "bind_next") bindNextPortal.fromString(value);
            else if (key == "bind_clear") bindClear.fromString(value);
            else if (key == "bind_preview_next") bindPreviewNext.fromString(value);
            else if (key == "bind_show_cells") bindShowCells.fromString(value);
            else if (key == "bind_p1") bindPortal[1].fromString(value);
            else if (key == "bind_p2") bindPortal[2].fromString(value);
            else if (key == "bind_p3") bindPortal[3].fromString(value);
            else if (key == "bind_p4") bindPortal[4].fromString(value);
        }
    }

    void save() const {
        std::ofstream file(configPath, std::ios::trunc);
        if (!file.is_open()) return;
        file << "show_grid=" << (showGrid ? "1" : "0") << "\n";
        file << "show_distances=" << (showDistances ? "1" : "0") << "\n";
        file << "menu_x=" << menuX << "\n";
        file << "menu_y=" << menuY << "\n";
        file << "bind_next=" << bindNextPortal.toString() << "\n";
        file << "bind_clear=" << bindClear.toString() << "\n";
        file << "bind_preview_next=" << bindPreviewNext.toString() << "\n";
        file << "bind_show_cells=" << bindShowCells.toString() << "\n";
        file << "bind_p1=" << bindPortal[1].toString() << "\n";
        file << "bind_p2=" << bindPortal[2].toString() << "\n";
        file << "bind_p3=" << bindPortal[3].toString() << "\n";
        file << "bind_p4=" << bindPortal[4].toString() << "\n";
    }
};