#pragma once

#include "Types.h"
#include "GridMath.h"

class Renderer {
public:
    static void drawDiamond(cv::Mat& img, cv::Point center, int width, int height, cv::Scalar color, int thickness) {
        int halfWidth = width / 2;
        int halfHeight = height / 2;
        std::vector<cv::Point> pts = {
                cv::Point(center.x, center.y - halfHeight),
                cv::Point(center.x + halfWidth, center.y),
                cv::Point(center.x, center.y + halfHeight),
                cv::Point(center.x - halfWidth, center.y)
        };
        const cv::Point* ppt[1] = { pts.data() };
        int npt[] = { 4 };
        cv::polylines(img, ppt, npt, 1, true, color, thickness, cv::LINE_AA);
    }

    static void drawGrid(cv::Mat& img, const IsoGrid& grid, int width, int height, cv::Scalar color, int thickness) {
        if (!grid.valid) return;
        std::vector<cv::Point2d> corners = {
                cv::Point2d(-grid.tileWidth, -grid.tileHeight),
                cv::Point2d(width + grid.tileWidth, -grid.tileHeight),
                cv::Point2d(-grid.tileWidth, height + grid.tileHeight),
                cv::Point2d(width + grid.tileWidth, height + grid.tileHeight)
        };

        double minU = std::numeric_limits<double>::infinity();
        double maxU = -std::numeric_limits<double>::infinity();
        double minV = std::numeric_limits<double>::infinity();
        double maxV = -std::numeric_limits<double>::infinity();

        for (const auto& c : corners) {
            cv::Point2d uv = GridMath::screenToGridD(grid, c);
            minU = std::min(minU, uv.x); maxU = std::max(maxU, uv.x);
            minV = std::min(minV, uv.y); maxV = std::max(maxV, uv.y);
        }

        for (int u = (int)std::floor(minU) - 4; u <= (int)std::ceil(maxU) + 4; u++) {
            for (int v = (int)std::floor(minV) - 4; v <= (int)std::ceil(maxV) + 4; v++) {
                cv::Point center = GridMath::gridToScreen(grid, u, v);
                if (center.x < -grid.tileWidth || center.x > width + grid.tileWidth ||
                    center.y < -grid.tileHeight || center.y > height + grid.tileHeight) continue;
                drawDiamond(img, center, grid.tileWidth, grid.tileHeight, color, thickness);
            }
        }
    }

    static void drawText(cv::Mat& img, const std::string& text, cv::Point center, cv::Scalar color) {
        int baseline = 0;
        cv::Size textSize = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.75, 2, &baseline);
        cv::Point origin(center.x - textSize.width / 2, center.y + textSize.height / 2);
        cv::putText(img, text, origin, cv::FONT_HERSHEY_SIMPLEX, 0.75, cv::Scalar(35, 35, 35, 255), 5, cv::LINE_AA);
        cv::putText(img, text, origin, cv::FONT_HERSHEY_SIMPLEX, 0.75, color, 2, cv::LINE_AA);
    }

    static void drawDistances(cv::Mat& img, const std::vector<SnappedPortal>& portals, cv::Scalar color) {
        for (size_t i = 0; i < portals.size(); i++) {
            for (size_t j = i + 1; j < portals.size(); j++) {
                int dist = GridMath::gridDistance(portals[i].cell, portals[j].cell);
                if (dist <= 0) continue;
                cv::line(img, portals[i].cell.center, portals[j].cell.center, color, 3, cv::LINE_AA);
                cv::Point mid((portals[i].cell.center.x + portals[j].cell.center.x) / 2,
                              (portals[i].cell.center.y + portals[j].cell.center.y) / 2 - 12);
                drawText(img, std::to_string(dist), mid, color);
            }
        }
    }

    static void drawPortalNumber(cv::Mat& img, int number, cv::Point center, cv::Scalar color) {
        std::string text = std::to_string(number);
        int baseline = 0;
        cv::Size textSize = cv::getTextSize(text, cv::FONT_HERSHEY_DUPLEX, 1.1, 2, &baseline);
        cv::Point origin(center.x - textSize.width / 2, center.y + textSize.height / 2 - 2);
        cv::putText(img, text, origin, cv::FONT_HERSHEY_DUPLEX, 1.1, cv::Scalar(15, 15, 15, 255), 6, cv::LINE_AA);
        cv::putText(img, text, origin, cv::FONT_HERSHEY_DUPLEX, 1.1, color, 2, cv::LINE_AA);
    }
};