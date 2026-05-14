#pragma once

#include "Types.h"
#include "GridMath.h"
#include <set>

class GridAnalyzer {
private:
    struct DetectionSample {
        std::vector<cv::Point> centers;
        std::vector<cv::Rect> rects;
    };

    IsoGrid lockedGrid;
    IsoGrid pendingGrid;

    int pendingFrames = 0;
    bool permanentlyLocked = false;

    const int FRAMES_TO_LOCK = 20;

    const cv::Vec3i GREEN_OUTER_BGR = cv::Vec3i(4, 163, 60);
    const cv::Vec3i GREEN_CENTER_BGR = cv::Vec3i(66, 194, 129);

    int colorDist2(const cv::Vec3b& pixel, const cv::Vec3i& color) {
        int db = pixel[0] - color[0];
        int dg = pixel[1] - color[1];
        int dr = pixel[2] - color[2];
        return db * db + dg * dg + dr * dr;
    }

    cv::Mat createMovementMask(const cv::Mat& frame) {
        cv::Mat mask(frame.rows, frame.cols, CV_8UC1, cv::Scalar(0));
        int tol2 = 45 * 45;
        for (int y = 0; y < frame.rows; y++) {
            const cv::Vec3b* src = frame.ptr<cv::Vec3b>(y);
            uchar* dst = mask.ptr<uchar>(y);
            for (int x = 0; x < frame.cols; x++) {
                if (colorDist2(src[x], GREEN_OUTER_BGR) <= tol2 || colorDist2(src[x], GREEN_CENTER_BGR) <= tol2) {
                    dst[x] = 255;
                }
            }
        }
        cv::Mat k = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
        cv::morphologyEx(mask, mask, cv::MORPH_OPEN, k);
        cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, k);
        return mask;
    }

    std::vector<cv::Point> mergeCenters(const std::vector<cv::Point>& centers) {
        std::vector<cv::Point2f> merged;
        std::vector<int> counts;
        for (const auto& p : centers) {
            bool found = false;
            for (size_t i = 0; i < merged.size(); i++) {
                double dist = std::sqrt(std::pow(merged[i].x - p.x, 2) + std::pow(merged[i].y - p.y, 2));
                if (dist < 22) {
                    float n = (float)counts[i];
                    merged[i].x = (merged[i].x * n + (float)p.x) / (n + 1.0f);
                    merged[i].y = (merged[i].y * n + (float)p.y) / (n + 1.0f);
                    counts[i]++;
                    found = true;
                    break;
                }
            }
            if (!found) {
                merged.push_back(cv::Point2f((float)p.x, (float)p.y));
                counts.push_back(1);
            }
        }
        std::vector<cv::Point> res;
        for (const auto& p : merged) res.push_back(cv::Point((int)std::round(p.x), (int)std::round(p.y)));
        return res;
    }

    DetectionSample extractCells(const cv::Mat& frame) {
        DetectionSample sample;
        cv::Mat mask = createMovementMask(frame);
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        for (const auto& cnt : contours) {
            std::vector<cv::Point> hull;
            cv::convexHull(cnt, hull);
            double area = cv::contourArea(hull);
            cv::Rect br = cv::boundingRect(hull);
            if (area < 650 || area > 6500) continue;
            if (br.width < 55 || br.width > 155) continue;
            if (br.height < 25 || br.height > 90) continue;
            float aspect = (float)br.width / br.height;
            if (aspect < 1.35f || aspect > 3.05f) continue;
            float fill = (float)area / (br.width * br.height);
            if (fill < 0.28f || fill > 0.92f) continue;
            sample.centers.push_back(cv::Point(br.x + br.width / 2, br.y + br.height / 2));
            sample.rects.push_back(br);
        }
        sample.centers = mergeCenters(sample.centers);
        return sample;
    }

    void estimateSize(const DetectionSample& s, int& w, int& h) {
        std::vector<double> dxs, dys;
        for (size_t i = 0; i < s.centers.size(); i++) {
            for (size_t j = i + 1; j < s.centers.size(); j++) {
                double dx = std::abs(s.centers[j].x - s.centers[i].x);
                double dy = std::abs(s.centers[j].y - s.centers[i].y);
                if (dx >= 30 && dx <= 100 && dy >= 15 && dy <= 60) {
                    dxs.push_back(dx); dys.push_back(dy);
                }
            }
        }
        if (dxs.size() >= 3 && dys.size() >= 3) {
            std::sort(dxs.begin(), dxs.end());
            std::sort(dys.begin(), dys.end());
            w = (int)std::round(2.0 * dxs[dxs.size() / 2]);
            h = (int)std::round(2.0 * dys[dys.size() / 2]);
        }
        w = std::max(80, std::min(w, 170));
        h = std::max(35, std::min(h, 95));
    }

    IsoGrid buildGrid(const std::vector<cv::Point>& centers, const cv::Point2d& origin, int tw, int th, double tol) {
        IsoGrid grid;
        grid.origin = origin;
        grid.tileWidth = tw;
        grid.tileHeight = th;
        std::set<std::pair<int, int>> used;
        double errs = 0;

        for (const auto& p : centers) {
            cv::Point2d pD(p.x, p.y);
            cv::Point2d uv = GridMath::screenToGridD(grid, pD);
            int u = (int)std::round(uv.x);
            int v = (int)std::round(uv.y);
            double err = GridMath::pointDistance(pD, GridMath::gridToScreenD(grid, u, v));
            if (err <= tol && used.insert({u, v}).second) {
                grid.greenCells.push_back({u, v, p});
                errs += err;
            }
        }

        grid.matchedGreenCells = (int)grid.greenCells.size();
        if (grid.matchedGreenCells > 0) grid.avgSnapError = errs / grid.matchedGreenCells;

        int links = 0;
        for (const auto& c : used) {
            if (used.count({c.first + 1, c.second})) links++;
            if (used.count({c.first, c.second + 1})) links++;
        }
        grid.adjacencyLinks = links;
        grid.valid = grid.matchedGreenCells >= 4 && links >= 3;
        return grid;
    }

    IsoGrid fitGrid(const DetectionSample& s) {
        IsoGrid best;
        if (s.centers.size() < 4) return best;
        int tw = 110, th = 55;
        estimateSize(s, tw, th);
        double tol = std::max(14.0, th * 0.36);
        double bestScore = -1e9;

        for (const auto& anchor : s.centers) {
            IsoGrid g = buildGrid(s.centers, cv::Point2d(anchor.x, anchor.y), tw, th, tol);
            if (!g.valid) continue;
            double score = g.matchedGreenCells * 1000.0 + g.adjacencyLinks * 150.0 - g.avgSnapError * 25.0;
            if (score > bestScore) {
                bestScore = score;
                best = g;
            }
        }
        return best;
    }

public:
    bool isLocked() const { return permanentlyLocked; }
    bool isLoading() const { return pendingFrames > 0 && !permanentlyLocked; }

    IsoGrid detect(const cv::Mat& frame) {
        if (permanentlyLocked) {
            return lockedGrid;
        }

        if (frame.empty()) return pendingGrid;

        DetectionSample s = extractCells(frame);

        if (s.centers.size() >= 4) {
            IsoGrid c = fitGrid(s);
            if (c.valid) {
                pendingFrames++;
                pendingGrid = c;

                if (pendingFrames >= FRAMES_TO_LOCK) {
                    permanentlyLocked = true;
                    lockedGrid = c;
                    pendingFrames = 0;
                }
            } else {
                pendingFrames = 0;
            }
        } else {
            pendingFrames = 0;
        }

        return permanentlyLocked ? lockedGrid : pendingGrid;
    }
};