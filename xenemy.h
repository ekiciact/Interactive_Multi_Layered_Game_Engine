#ifndef XENEMY_WRAPPER_H
#define XENEMY_WRAPPER_H

#include "enemy.h"
#include <random>

class XEnemyWrapper : public EnemyWrapper {
public:
    virtual ~XEnemyWrapper() = default; // polymorphic

    explicit XEnemyWrapper(std::unique_ptr<Enemy> e, int cols, int rows)
        : EnemyWrapper(std::move(e)), resurrected(false), mapCols(cols), mapRows(rows)
    {}

    void hit() {
        if (!isDefeated()) {
            setDefeated(true);
        } else if (!resurrected) {
            resurrected = true;
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> distX(0, mapCols - 1);
            std::uniform_int_distribution<> distY(0, mapRows - 1);
            int x = distX(gen);
            int y = distY(gen);
            getRaw()->setXPos(x);
            getRaw()->setYPos(y);
            float newStr = getStrength() * 2.0f;
            getRaw()->setValue(newStr);
            setDefeated(false);
        } else {
            setDefeated(true);
        }
    }

    bool hasResurrected() const noexcept { return resurrected; }

    std::string serialize() const noexcept {
        return "XEnemy: " + EnemyWrapper::serialize() + (resurrected ? ", resurrected" : ", not resurrected");
    }

private:
    bool resurrected;
    int mapCols;
    int mapRows;
};

#endif // XENEMY_WRAPPER_H
