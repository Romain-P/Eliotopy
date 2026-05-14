#pragma once

#include "Types.h"
#include <cmath>

class GridMath {
public:
    static cv::Point2d gridToScreenD(const IsoGrid& grid, int u, int v) {
        double halfW = grid.tileWidth / 2.0;
        double halfH = grid.tileHeight / 2.0;
        return cv::Point2d(grid.origin.x + (u + v) * halfW, grid.origin.y + (u - v) * halfH);
    }

    static cv::Point gridToScreen(const IsoGrid& grid, int u, int v) {
        cv::Point2d p = gridToScreenD(grid, u, v);
        return cv::Point((int)std::round(p.x), (int)std::round(p.y));
    }

    static cv::Point2d screenToGridD(const IsoGrid& grid, const cv::Point2d& p) {
        double halfW = grid.tileWidth / 2.0;
        double halfH = grid.tileHeight / 2.0;
        double dx = p.x - grid.origin.x;
        double dy = p.y - grid.origin.y;
        double a = dx / halfW;
        double b = dy / halfH;
        return cv::Point2d((a + b) / 2.0, (a - b) / 2.0);
    }

    static double pointDistance(const cv::Point2d& a, const cv::Point2d& b) {
        double dx = a.x - b.x;
        double dy = a.y - b.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    static int gridDistance(const GridCell& a, const GridCell& b) {
        return std::abs(a.u - b.u) + std::abs(a.v - b.v);
    }
};