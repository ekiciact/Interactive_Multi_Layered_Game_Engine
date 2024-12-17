#ifndef DEFAULTAUTOPLAYSTRATEGY_H
#define DEFAULTAUTOPLAYSTRATEGY_H

#include "autoplaystrategy.h"
#include "node.h"
#include "pathfinder_class.h"
#include <functional>
#include <limits>

class EnemyWrapper;
class GameModel;

class DefaultAutoPlayStrategy : public AutoPlayStrategy {
public:
    DefaultAutoPlayStrategy(std::function<bool(const Node&,const Node&)> nodeComparator)
        : model(nullptr), autoPathIndex(0), nodeComparator(nodeComparator)
    {}

    void start(GameModel *model) override;
    void stop() override;
    AutoPlayMove nextStep() override;

    // Decide next action based on conditions (if used in full auto-play mode)
    void decideNextAction() override;

    // Direct compute path methods (no logic beyond finding the path):
    bool computePathToEnemy();        // Finds path to nearest enemy
    bool computePathToHealthPack();   // Finds path to nearest health pack
    bool computePathToPortal();       // Finds path to portal
    bool computePathToTile(int x, int y);


    void planPathToTile(int x, int y) override {
        // For compatibility with interface
        computePathToTile(x,y);
    }

    void planPathToEnemy() override {
        computePathToEnemy();
    }

    void planPathToHealthPack() override {
        computePathToHealthPack();
    }

    void planPathToPortal() override {
        computePathToPortal();
    }

    void planPathToEnemyWithHealthPacks() override {
        decideNextAction();
    }

private:
    GameModel *model;
    std::vector<int> autoPath;
    int autoPathIndex;
    std::function<bool(const Node&, const Node&)> nodeComparator;

    enum class TargetType { None, Enemy, HealthPack, Portal };
    TargetType currentTarget = TargetType::None;

    std::vector<Node> nodes;

    EnemyWrapper* findNextTargetEnemy();
    void resetNodes();

    // Path planning helpers
    std::vector<int> findPath(int startX, int startY, int endX, int endY,
                              std::function<float(const Node&,const Node&)> costFunc,
                              std::function<float(const Node&,const Node&)> heuristicFunc);

    // Heuristic and cost functions reused in multiple methods
    float defaultCostFunc(const Node &b);
    static float defaultHeuristicFunc(const Node &a, const Node &b);
};

#endif // DEFAULTAUTOPLAYSTRATEGY_H
