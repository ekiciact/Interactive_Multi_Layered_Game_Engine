#include "defaultautoplaystrategy.h"
#include "gamemodel.h"
#include "protagonist.h"
#include "enemy.h"
#include "penemy.h"
#include "xenemy.h"
#include "healthpack.h"
#include "portal.h"
#include <cmath>
#include <limits>
#include <random>

void DefaultAutoPlayStrategy::start(GameModel *m) {
    model = m;
    autoPath.clear();
    autoPathIndex = 0;
    currentTarget = TargetType::None;
}

void DefaultAutoPlayStrategy::stop() {
    autoPath.clear();
    autoPathIndex = 0;
    currentTarget = TargetType::None;
}

AutoPlayMove DefaultAutoPlayStrategy::nextStep() {
    if (!model) return {0,0};

    auto *p = model->getProtagonist();
    if (p->getHealth() <= 0 || p->getEnergy() <= 0) {
        return {0,0};
    }

    if (autoPathIndex >= (int)autoPath.size()) {
        // No steps left
        return {0,0};
    }

    int move = autoPath[autoPathIndex++];
    int dx=0,dy=0;
    switch(move) {
    case 0: dy = -1; break;
    case 1: dx = 1; dy = -1; break;
    case 2: dx = 1; break;
    case 3: dx = 1; dy = 1; break;
    case 4: dy = 1; break;
    case 5: dx = -1; dy = 1; break;
    case 6: dx = -1; break;
    case 7: dx = -1; dy = -1; break;
    }

    return {dx, dy};
}

void DefaultAutoPlayStrategy::decideNextAction() {
    if (!model) return;
    auto *p = model->getProtagonist();
    EnemyWrapper* targetEnemy = findNextTargetEnemy();
    autoPath.clear();
    autoPathIndex = 0;

    if (targetEnemy) {
        float enemyDamage = targetEnemy->getStrength();
        // Need health or not
        if (p->getHealth() <= enemyDamage) {
            // Need health pack first
            if (!computePathToHealthPack()) {
                // no HP found => just try enemy anyway
                computePathToEnemy();
                currentTarget = TargetType::Enemy;
            } else {
                currentTarget = TargetType::HealthPack;
            }
        } else if (p->getHealth() < 70.0f) {
            // Try health pack first if possible
            if (!computePathToHealthPack()) {
                computePathToEnemy();
                currentTarget = TargetType::Enemy;
            } else {
                currentTarget = TargetType::HealthPack;
            }
        } else {
            // Just go to enemy
            computePathToEnemy();
            currentTarget = TargetType::Enemy;
        }
    } else {
        // No enemy alive
        // Only compute path to portal if no enemies
        if (computePathToPortal()) {
            currentTarget = TargetType::Portal;
        } else {
            currentTarget = TargetType::None;
        }
    }
}

bool DefaultAutoPlayStrategy::computePathToEnemy() {
    EnemyWrapper* e = findNextTargetEnemy();
    if (!e) {
        autoPath.clear();
        return false;
    }

    auto *p = model->getProtagonist();
    resetNodes();
    int cols = model->getCols();
    Node &startNode = nodes[p->getYPos()*cols + p->getXPos()];
    Node &endNode = nodes[e->getYPos()*cols + e->getXPos()];

    // Avoid portal if enemies remain
    bool enemiesAlive = (findNextTargetEnemy() != nullptr);

    auto costFunc = [this, e, enemiesAlive](const Node &a, const Node &b) {
        for (auto &enemy : model->getEnemies()) {
            if (!enemy->isDefeated() && enemy.get() != e &&
                enemy->getXPos()==b.getXPos() && enemy->getYPos()==b.getYPos()) {
                return std::numeric_limits<float>::infinity();
            }
        }
        // Avoid portal if enemies still alive
        if (enemiesAlive) {
            for (auto &port : model->getPortals()) {
                if (port->getXPos()==b.getXPos() && port->getYPos()==b.getYPos()) {
                    return std::numeric_limits<float>::infinity();
                }
            }
        }

        return defaultCostFunc(b);
    };

    auto heuristicFunc = defaultHeuristicFunc;

    autoPath = findPath(startNode.getXPos(), startNode.getYPos(),
                        endNode.getXPos(), endNode.getYPos(),
                        costFunc, heuristicFunc);

    autoPathIndex = 0;
    return !autoPath.empty();
}

bool DefaultAutoPlayStrategy::computePathToHealthPack() {
    resetNodes();
    auto *p = model->getProtagonist();
    HealthPack *nearestHP = nullptr;
    float minDist = std::numeric_limits<float>::max();
    for (auto &hp : model->getHealthPacks()) {
        float dist = std::sqrt((hp->getXPos()-p->getXPos())*(hp->getXPos()-p->getXPos())
                               + (hp->getYPos()-p->getYPos())*(hp->getYPos()-p->getYPos()));
        if (dist < minDist) {
            minDist = dist;
            nearestHP = hp.get();
        }
    }

    if (!nearestHP) {
        autoPath.clear();
        return false;
    }

    bool enemiesAlive = (findNextTargetEnemy() != nullptr);

    int cols = model->getCols();
    Node &startNode = nodes[p->getYPos()*cols + p->getXPos()];
    Node &endNode = nodes[nearestHP->getYPos()*cols + nearestHP->getXPos()];

    auto costFunc = [this, enemiesAlive](const Node &a, const Node &b) {
        for (auto &e : model->getEnemies()) {
            if(!e->isDefeated() && e->getXPos()==b.getXPos() && e->getYPos()==b.getYPos()) {
                return std::numeric_limits<float>::infinity();
            }
        }
        if (enemiesAlive) {
            for (auto &port : model->getPortals()) {
                if (port->getXPos()==b.getXPos() && port->getYPos()==b.getYPos()) {
                    return std::numeric_limits<float>::infinity();
                }
            }
        }
        return defaultCostFunc(b);
    };
    auto heuristicFunc = defaultHeuristicFunc;

    autoPath = findPath(startNode.getXPos(), startNode.getYPos(),
                        endNode.getXPos(), endNode.getYPos(),
                        costFunc, heuristicFunc);
    autoPathIndex = 0;
    return !autoPath.empty();
}

bool DefaultAutoPlayStrategy::computePathToPortal() {
    resetNodes();
    // If no portals or enemies alive (this method only called if no enemies), just go portal
    if (model->getPortals().empty()) {
        autoPath.clear();
        return false;
    }

    auto *p = model->getProtagonist();
    Portal *portal = model->getPortals().front().get();
    int cols = model->getCols();
    Node &startNode = nodes[p->getYPos()*cols + p->getXPos()];
    Node &endNode = nodes[portal->getYPos()*cols + portal->getXPos()];

    // If decideNextAction calls computePathToPortal, it means no enemies alive, so no need to avoid portal
    bool enemiesAlive = (findNextTargetEnemy() != nullptr);

    auto costFunc = [this, enemiesAlive](const Node &a, const Node &b) {
        for (auto &e : model->getEnemies()) {
            if (!e->isDefeated() && e->getXPos()==b.getXPos() && e->getYPos()==b.getYPos()) {
                return std::numeric_limits<float>::infinity();
            }
        }
        // If by any chance enemiesAlive is true here, avoid portal:
        if (enemiesAlive) {
            for (auto &port : model->getPortals()) {
                if (port->getXPos()==b.getXPos() && port->getYPos()==b.getYPos()) {
                    return std::numeric_limits<float>::infinity();
                }
            }
        }

        return defaultCostFunc(b);
    };
    auto heuristicFunc = defaultHeuristicFunc;

    autoPath = findPath(startNode.getXPos(), startNode.getYPos(),
                        endNode.getXPos(), endNode.getYPos(),
                        costFunc, heuristicFunc);
    autoPathIndex = 0;
    return !autoPath.empty();
}

bool DefaultAutoPlayStrategy::computePathToTile(int x, int y) {
    resetNodes();
    auto *p = model->getProtagonist();
    if (x<0||x>=model->getCols()||y<0||y>=model->getRows()) {
        autoPath.clear();
        return false;
    }

    bool enemiesAlive = (findNextTargetEnemy() != nullptr);

    int cols = model->getCols();
    Node &startNode = nodes[p->getYPos()*cols + p->getXPos()];
    Node &endNode = nodes[y*cols + x];

    auto costFunc = [this, enemiesAlive](const Node &a, const Node &b) {
        for (auto &e : model->getEnemies()) {
            if (!e->isDefeated() && e->getXPos()==b.getXPos() && e->getYPos()==b.getYPos()) {
                return std::numeric_limits<float>::infinity();
            }
        }
        // Avoid portal if enemies alive
        if (enemiesAlive) {
            for (auto &port : model->getPortals()) {
                if (port->getXPos()==b.getXPos() && port->getYPos()==b.getYPos()) {
                    return std::numeric_limits<float>::infinity();
                }
            }
        }

        return defaultCostFunc(b);
    };
    auto heuristicFunc = defaultHeuristicFunc;

    autoPath = findPath(startNode.getXPos(), startNode.getYPos(),
                        endNode.getXPos(), endNode.getYPos(),
                        costFunc, heuristicFunc);
    autoPathIndex = 0;
    return !autoPath.empty();
}

EnemyWrapper* DefaultAutoPlayStrategy::findNextTargetEnemy() {
    EnemyWrapper *targetEnemy = nullptr;
    float minDistance = std::numeric_limits<float>::max();
    auto *p = model->getProtagonist();

    for (auto &e : model->getEnemies()) {
        if (!e->isDefeated()) {
            float dist = std::sqrt((e->getXPos() - p->getXPos())*(e->getXPos()-p->getXPos())
                                   + (e->getYPos() - p->getYPos())*(e->getYPos()-p->getYPos()));
            if (dist < minDistance) {
                minDistance = dist;
                targetEnemy = e.get();
            }
        }
    }

    return targetEnemy;
}

void DefaultAutoPlayStrategy::resetNodes() {
    nodes.clear();
    const auto &tiles = model->getTiles();
    nodes.reserve(tiles.size());

    for (auto &tw : tiles) {
        Node n(tw->getXPos(), tw->getYPos(), tw->getValue());
        n.f = n.g = n.h = 0;
        n.visited = n.closed = false;
        n.prev = nullptr;
        nodes.push_back(n);
    }
}

std::vector<int> DefaultAutoPlayStrategy::findPath(
    int startX, int startY, int endX, int endY,
    std::function<float(const Node&,const Node&)> costFunc,
    std::function<float(const Node&,const Node&)> heuristicFunc)
{
    int cols = model->getCols();
    Node &startNode = nodes[startY*cols + startX];
    Node &endNode = nodes[endY*cols + endX];

    PathFinder<Node, Node> pathfinder(nodes, &startNode, &endNode, nodeComparator,(unsigned int)cols,costFunc,heuristicFunc,1.0f);
    return pathfinder.A_star();
}

float DefaultAutoPlayStrategy::defaultCostFunc(const Node &b) {
    return 1.0f/(b.getValue()+1.0f)*0.1f;
}

float DefaultAutoPlayStrategy::defaultHeuristicFunc(const Node &a, const Node &b) {
    return std::sqrt((a.getXPos()-b.getXPos())*(a.getXPos()-b.getXPos())
                     + (a.getYPos()-b.getYPos())*(a.getYPos()-b.getYPos()));
}
