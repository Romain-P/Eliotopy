#pragma once

#include "Types.h"
#include "GridMath.h"
#include <string>
#include <cmath>
#include <windows.h>

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

        if (thickness < 0) {
            cv::fillPoly(img, ppt, npt, 1, color, cv::LINE_AA);
        } else {
            cv::polylines(img, ppt, npt, 1, true, color, thickness, cv::LINE_AA);
        }
    }

    static void drawHatchedDiamond(cv::Mat& img, cv::Point center, int width, int height, cv::Scalar color, int step = 3) {
        int halfWidth = width / 2;
        int halfHeight = height / 2;
        cv::Vec4b col((uchar)color[0], (uchar)color[1], (uchar)color[2], 255);

        for (int y = -halfHeight + 1; y < halfHeight; ++y) {
            int rowWidth = halfWidth - (std::abs(y) * halfWidth) / halfHeight;
            for (int x = -rowWidth + 1; x < rowWidth; ++x) {
                if (x % step == 0 && y % step == 0) {
                    int px = center.x + x;
                    int py = center.y + y;
                    if (px >= 0 && px < img.cols && py >= 0 && py < img.rows) {
                        img.at<cv::Vec4b>(py, px) = col;
                    }
                }
            }
        }
    }

    static void drawPulsingHighlight(cv::Mat& img, cv::Point center, const IsoGrid& grid, DWORD tickCount, cv::Scalar color) {
        double pulse = (std::sin(tickCount / 100.0) + 1.0) / 2.0;
        int outerThick = 2 + (int)(pulse * 3);
        drawDiamond(img, center, grid.tileWidth + outerThick * 2, grid.tileHeight + outerThick * 2, color, outerThick);

        int innerW = (int)(grid.tileWidth * pulse * 0.7);
        int innerH = (int)(grid.tileHeight * pulse * 0.7);
        if (innerW > 5 && innerH > 5) {
            drawDiamond(img, center, innerW, innerH, color, -1);
        }
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

    static void drawTextBadge(cv::Mat& img, const std::string& text, cv::Point center, cv::Scalar textColor, cv::Scalar bgColor) {
        int baseline = 0;
        cv::Size sz = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.5, 2, &baseline);
        int padX = 8;
        int padY = 4;
        cv::Rect badge(center.x - sz.width / 2 - padX, center.y - sz.height / 2 - padY, sz.width + padX * 2, sz.height + padY * 2);

        cv::rectangle(img, badge, bgColor, cv::FILLED, cv::LINE_AA);
        cv::rectangle(img, badge, textColor * 0.7, 1, cv::LINE_AA);

        cv::Point textOrg(center.x - sz.width / 2, center.y + sz.height / 2 - 1);
        cv::putText(img, text, textOrg, cv::FONT_HERSHEY_SIMPLEX, 0.5, textColor, 2, cv::LINE_AA);
    }

    static void drawCursorTooltip(cv::Mat& img, const std::string& text, cv::Point cursor, cv::Scalar color) {
        int baseline = 0;
        cv::Size sz = cv::getTextSize(text, cv::FONT_HERSHEY_DUPLEX, 0.65, 2, &baseline);
        cv::Point pos(cursor.x + 20, cursor.y - 20);

        cv::putText(img, text, pos, cv::FONT_HERSHEY_DUPLEX, 0.65, cv::Scalar(20, 20, 20, 255), 4, cv::LINE_AA);
        cv::putText(img, text, pos, cv::FONT_HERSHEY_DUPLEX, 0.65, color, 2, cv::LINE_AA);
    }

    static void drawDistances(cv::Mat& img, const std::vector<SnappedPortal>& portals, cv::Scalar color, const IsoGrid& grid) {
        double trim = grid.tileHeight * 0.7;

        for (size_t i = 0; i < portals.size(); i++) {
            for (size_t j = i + 1; j < portals.size(); j++) {
                int dist = GridMath::gridDistance(portals[i].cell, portals[j].cell);
                if (dist <= 0) continue;

                cv::Point p1 = portals[i].cell.center;
                cv::Point p2 = portals[j].cell.center;

                double dx = p2.x - p1.x;
                double dy = p2.y - p1.y;
                double len = std::sqrt(dx * dx + dy * dy);

                if (len < trim * 2) continue;

                cv::Point start(p1.x + (dx / len) * trim, p1.y + (dy / len) * trim);
                cv::Point end(p2.x - (dx / len) * trim, p2.y - (dy / len) * trim);

                cv::line(img, start, end, cv::Scalar(230, 220, 50, 255), 2, cv::LINE_AA);

                double offsetRatio = 0.35 + 0.05 * (i % 2);
                double nx = -dy / len;
                double ny = dx / len;

                cv::Point badgePos(p1.x + dx * offsetRatio + nx * 22, p1.y + dy * offsetRatio + ny * 22);

                drawTextBadge(img, std::to_string(dist), badgePos, color, cv::Scalar(20, 20, 20, 255));
            }
        }
    }

    static void drawPortalNumber(cv::Mat& img, int number, cv::Point center, cv::Scalar color) {
        cv::circle(img, center, 14, cv::Scalar(20, 20, 20, 255), cv::FILLED, cv::LINE_AA);
        cv::circle(img, center, 14, color, 2, cv::LINE_AA);

        std::string text = std::to_string(number);
        int baseline = 0;
        cv::Size sz = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.55, 2, &baseline);

        cv::Point origin(center.x - sz.width / 2, center.y + sz.height / 2 - 1);
        cv::putText(img, text, origin, cv::FONT_HERSHEY_SIMPLEX, 0.55, color, 2, cv::LINE_AA);
    }

    static void drawPathWithDistances(cv::Mat& img, const std::vector<SnappedPortal>& path, const IsoGrid& grid, DWORD tickCount) {
        if (path.size() < 2) return;

        cv::Scalar arrowColor(230, 220, 50, 255);
        double trim = grid.tileHeight * 0.7;

        for (size_t i = 0; i < path.size() - 1; i++) {
            cv::Point p1 = path[i].cell.center;
            cv::Point p2 = path[i+1].cell.center;

            int dist = GridMath::gridDistance(path[i].cell, path[i+1].cell);

            double dx = p2.x - p1.x;
            double dy = p2.y - p1.y;
            double len = std::sqrt(dx * dx + dy * dy);

            if (len >= trim * 2) {
                cv::Point start(p1.x + (dx / len) * trim, p1.y + (dy / len) * trim);
                cv::Point end(p2.x - (dx / len) * trim, p2.y - (dy / len) * trim);

                cv::arrowedLine(img, start, end, arrowColor, 3, cv::LINE_AA, 0, 0.04);

                cv::Point mid((p1.x + p2.x) / 2, (p1.y + p2.y) / 2);
                drawTextBadge(img, std::to_string(dist), mid, cv::Scalar(255, 255, 255, 255), cv::Scalar(30, 30, 30, 240));
            }
        }

        cv::Point exitPt = path.back().cell.center;
        drawPulsingHighlight(img, exitPt, grid, tickCount, cv::Scalar(255, 240, 40, 255));
    }
};