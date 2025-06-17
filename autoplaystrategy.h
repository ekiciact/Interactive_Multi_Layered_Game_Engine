#ifndef AUTOPLAYSTRATEGY_H
#define AUTOPLAYSTRATEGY_H

#include <vector>

class GameModel;

// Represents a move direction: dx, dy
struct AutoPlayMove {
    int dx;
    int dy;
};

// Interface for different autoplay strategies
class AutoPlayStrategy {
public:
    virtual ~AutoPlayStrategy() = default;

    // Called when starting autoplay (e.g., to initialize or reset internal state)
    virtual void start(GameModel *model) = 0;

    // Called when stopping autoplay
    virtual void stop() = 0;

    // Request the strategy to decide the next move (step).
    // If no move is needed (autoplay done or stuck), return {0,0} and handle that.
    virtual AutoPlayMove nextStep() = 0;

    // Decide next action: called when we need a new path or target after reaching a goal.
    virtual void decideNextAction() = 0;

    // Called to plan going to a specific tile (like gotoXY)
    virtual void planPathToTile(int x, int y) = 0;

    // Expose methods that were previously in GameController for direct calls:
    virtual void planPathToEnemy() = 0;
    virtual void planPathToHealthPack() = 0;
    virtual void planPathToPortal() = 0;
    virtual void planPathToEnemyWithHealthPacks() = 0;
};

#endif // AUTOPLAYSTRATEGY_H
