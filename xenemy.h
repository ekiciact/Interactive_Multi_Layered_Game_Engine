#ifndef XENEMY_WRAPPER_H
#define XENEMY_WRAPPER_H

#include "enemy.h"
#include <random>

class XEnemyWrapper : public EnemyWrapper {
public:
    virtual ~XEnemyWrapper() = default;

    explicit XEnemyWrapper(std::unique_ptr<Enemy> e, int cols, int rows)
        : EnemyWrapper(std::move(e)), mapCols(cols), mapRows(rows), timesHit(0), justTeleported(false)
    {}

    void hit() {
        if (timesHit == 0) {
            // First hit: teleport and (optional: double strength)
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
            justTeleported = true;
        } else {
            // Second hit: defeated
            setDefeated(true);
            timesHit++;
        }
    }

    int getTimesHit() const { return timesHit; }
    bool hasJustTeleported() const { return justTeleported; }
    int getOldX() const { return oldX; }
    int getOldY() const { return oldY; }
    void clearJustTeleported() { justTeleported = false; }

    std::string serialize() const noexcept override {
        return "XEnemy: " + EnemyWrapper::serialize() + ", timesHit=" + std::to_string(timesHit);
    }

private:
    int mapCols;
    int mapRows;
    int timesHit;
    bool justTeleported;
    int oldX;
    int oldY;
};

#endif // XENEMY_WRAPPER_H
