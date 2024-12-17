#ifndef XENEMY_WRAPPER_H
#define XENEMY_WRAPPER_H

#include "enemy.h"
#include <random>

/**
 * XEnemy logic:
 * - On first hit: Teleports to random location, doubles strength, no damage to protagonist.
 * - On second hit: Enemy is defeated.
 */
class XEnemyWrapper : public EnemyWrapper {
public:
    virtual ~XEnemyWrapper() = default;

    explicit XEnemyWrapper(std::unique_ptr<Enemy> e, int cols, int rows)
        : EnemyWrapper(std::move(e)), mapCols(cols), mapRows(rows), timesHit(0)
    {}

    void hit() {
        if (timesHit == 0) {
            // First hit: teleport and (optional double strength)
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> distX(0, mapCols - 1);
            std::uniform_int_distribution<> distY(0, mapRows - 1);
            int x = distX(gen);
            int y = distY(gen);

            getRaw()->setXPos(x);
            getRaw()->setYPos(y);

            float newStr = getStrength() * 1.0f;
            getRaw()->setValue(newStr);

            timesHit++;
        } else {
            // Second hit: defeated
            setDefeated(true);
        }
    }

    std::string serialize() const noexcept override {
        return "XEnemy: " + EnemyWrapper::serialize() + ", timesHit=" + std::to_string(timesHit);
    }

private:
    int mapCols;
    int mapRows;
    int timesHit;
};

#endif // XENEMY_WRAPPER_H
