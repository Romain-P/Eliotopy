#pragma once

#include "Types.h"
#include "GridMath.h"

#include <vector>
#include <cmath>
#include <limits>
#include <algorithm>

class RedirectionHelper {
private:
    enum AngleRelation {
        SAME = 0,
        OPPOSITE = 1,
        TRIGONOMETRIC = 2,
        COUNTER_TRIGONOMETRIC = 3
    };

    struct Coord {
        int x = 0;
        int y = 0;
    };

    struct Candidate {
        int index = -1;
        Coord coord;
        double angle = 0.0;
    };

    static constexpr double PI = 3.1415926535897932384626433832795;
    static constexpr double TWO_PI = 2.0 * PI;
    static constexpr double EPS = 1e-9;

private:
    static bool sameCell(const SnappedPortal& a, const SnappedPortal& b) {
        return a.cell.u == b.cell.u && a.cell.v == b.cell.v;
    }

    static Coord toCoord(const SnappedPortal& p) {
        return { p.cell.u, p.cell.v };
    }

    static int portalDistance(const SnappedPortal& a, const SnappedPortal& b) {
        const Coord ca = toCoord(a);
        const Coord cb = toCoord(b);

        return std::abs(ca.x - cb.x) + std::abs(ca.y - cb.y);
    }

    static Coord vectorFromTo(const Coord& start, const Coord& end) {
        return {
                end.x - start.x,
                end.y - start.y
        };
    }

    static int determinant(const Coord& a, const Coord& b) {
        return (a.x * b.y) - (a.y * b.x);
    }

    static double euclideanDistance(const Coord& a, const Coord& b) {
        const double dx = (double)a.x - (double)b.x;
        const double dy = (double)a.y - (double)b.y;
        return std::sqrt(dx * dx + dy * dy);
    }

    static double clampDouble(double v, double minVal, double maxVal) {
        if (v < minVal) return minVal;
        if (v > maxVal) return maxVal;
        return v;
    }

    static AngleRelation compareAngles(
            const Coord& ref,
            const Coord& start,
            const Coord& end
    ) {
        const Coord aVec = vectorFromTo(ref, start);
        const Coord bVec = vectorFromTo(ref, end);

        const int det = determinant(aVec, bVec);

        if (det != 0) {
            return det > 0 ? TRIGONOMETRIC : COUNTER_TRIGONOMETRIC;
        }

        const bool sameXSign = (aVec.x >= 0) == (bVec.x >= 0);
        const bool sameYSign = (aVec.y >= 0) == (bVec.y >= 0);

        return (sameXSign && sameYSign) ? SAME : OPPOSITE;
    }

    static double getAngle(
            const Coord& ref,
            const Coord& a,
            const Coord& b
    ) {
        const double sideA = euclideanDistance(a, b);
        const double sideB = euclideanDistance(ref, a);
        const double sideC = euclideanDistance(ref, b);

        const double denom = 2.0 * sideB * sideC;

        if (denom <= EPS) {
            return 0.0;
        }

        double cosValue = ((sideB * sideB) + (sideC * sideC) - (sideA * sideA)) / denom;
        cosValue = clampDouble(cosValue, -1.0, 1.0);

        return std::acos(cosValue);
    }

    static double getPositiveOrientedAngle(
            const Coord& ref,
            const Coord& start,
            const Coord& end
    ) {
        switch (compareAngles(ref, start, end)) {
            case SAME:
                return 0.0;

            case OPPOSITE:
                return PI;

            case TRIGONOMETRIC:
                return getAngle(ref, start, end);

            case COUNTER_TRIGONOMETRIC:
                return TWO_PI - getAngle(ref, start, end);

            default:
                return 0.0;
        }
    }

    static int getBestPortalWhenRefIsNotInsideClosests(
            const Coord& ref,
            const std::vector<Candidate>& sortedClosests
    ) {
        if (sortedClosests.size() < 2) {
            return -1;
        }

        int prevLocalIndex = (int)sortedClosests.size() - 1;

        for (int i = 0; i < (int)sortedClosests.size(); ++i) {
            const AngleRelation relation = compareAngles(
                    ref,
                    sortedClosests[prevLocalIndex].coord,
                    sortedClosests[i].coord
            );

            if (relation == OPPOSITE) {
                if (sortedClosests.size() <= 2) {
                    return -1;
                }
                return sortedClosests[prevLocalIndex].index;
            }

            if (relation == COUNTER_TRIGONOMETRIC) {
                return sortedClosests[prevLocalIndex].index;
            }

            prevLocalIndex = i;
        }

        return -1;
    }

    static int getBestNextPortal(
            const SnappedPortal& refPortal,
            const std::vector<SnappedPortal>& unvisited,
            const std::vector<int>& closestIndices
    ) {
        if (closestIndices.size() < 2) {
            return closestIndices.empty() ? -1 : closestIndices[0];
        }

        const Coord refCoord = toCoord(refPortal);
        const Coord nudge = { refCoord.x, refCoord.y + 1 };

        std::vector<Candidate> sortedClosests;
        sortedClosests.reserve(closestIndices.size());

        for (const int idx : closestIndices) {
            const Coord c = toCoord(unvisited[idx]);

            sortedClosests.push_back({
                                             idx,
                                             c,
                                             getPositiveOrientedAngle(refCoord, nudge, c)
                                     });
        }

        std::sort(sortedClosests.begin(), sortedClosests.end(),
                  [](const Candidate& a, const Candidate& b) {
                      if (std::abs(a.angle - b.angle) > EPS) {
                          return a.angle < b.angle;
                      }
                      return a.index < b.index;
                  }
        );

        const int specialResult = getBestPortalWhenRefIsNotInsideClosests(
                refCoord,
                sortedClosests
        );

        if (specialResult != -1) {
            return specialResult;
        }

        return sortedClosests[0].index;
    }

    static int getClosestPortal(
            const SnappedPortal& refPortal,
            const std::vector<SnappedPortal>& unvisited
    ) {
        int bestDist = std::numeric_limits<int>::max();
        std::vector<int> closestIndices;

        for (int i = 0; i < (int)unvisited.size(); ++i) {
            const int dist = portalDistance(refPortal, unvisited[i]);

            if (dist < bestDist) {
                bestDist = dist;
                closestIndices.clear();
                closestIndices.push_back(i);
            }
            else if (dist == bestDist) {
                closestIndices.push_back(i);
            }
        }

        if (closestIndices.empty()) {
            return -1;
        }

        if (closestIndices.size() == 1) {
            return closestIndices[0];
        }

        return getBestNextPortal(refPortal, unvisited, closestIndices);
    }

public:
    static std::vector<SnappedPortal> calculatePath(
            const std::vector<SnappedPortal>& network,
            SnappedPortal entrance
    ) {
        std::vector<SnappedPortal> path;

        if (network.empty()) {
            return path;
        }

        std::vector<SnappedPortal> openPortals;
        openPortals.reserve(network.size());

        for (const auto& p : network) {
            if (p.valid && p.state == PortalState::OPEN) {
                openPortals.push_back(p);
            }
        }

        if (openPortals.empty()) {
            return path;
        }

        bool entranceFound = false;

        for (const auto& p : openPortals) {
            if (sameCell(p, entrance)) {
                entrance = p;
                entranceFound = true;
                break;
            }
        }

        if (!entranceFound) {
            return path;
        }

        std::vector<SnappedPortal> unvisited;
        unvisited.reserve(openPortals.size());

        for (const auto& p : openPortals) {
            if (!sameCell(p, entrance)) {
                unvisited.push_back(p);
            }
        }

        SnappedPortal current = entrance;

        int maxTry = (int)unvisited.size() + 1;

        while (maxTry > 0) {
            --maxTry;

            path.push_back(current);

            if (unvisited.empty()) {
                break;
            }

            const int nextIdx = getClosestPortal(current, unvisited);

            if (nextIdx < 0) {
                break;
            }

            current = unvisited[nextIdx];
            unvisited.erase(unvisited.begin() + nextIdx);
        }

        if (path.size() < 2) {
            return std::vector<SnappedPortal>{ entrance };
        }

        return path;
    }
};