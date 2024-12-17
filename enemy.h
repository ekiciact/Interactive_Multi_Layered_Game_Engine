#ifndef ENEMY_WRAPPER_H
#define ENEMY_WRAPPER_H

#include "world.h"
#include <memory>
#include <string>

class EnemyWrapper {
public:
    virtual ~EnemyWrapper() = default;

    explicit EnemyWrapper(std::unique_ptr<Enemy> e)
        : enemy(std::move(e)) {}

    int getXPos() const noexcept { return enemy->getXPos(); }
    int getYPos() const noexcept { return enemy->getYPos(); }
    float getStrength() const noexcept { return enemy->getValue(); }
    bool isDefeated() const noexcept { return enemy->getDefeated(); }
    void setDefeated(bool d) noexcept { enemy->setDefeated(d); }

    virtual std::string serialize() const noexcept {
        return "Enemy: " + enemy->serialize();
    }

    Enemy* getRaw() const noexcept { return enemy.get(); }

protected:
    std::unique_ptr<Enemy> enemy;
};

#endif // ENEMY_WRAPPER_H
