#ifndef PENEMY_WRAPPER_H
#define PENEMY_WRAPPER_H

#include "enemy.h"
#include <memory>

class PEnemyWrapper : public EnemyWrapper {
public:
    virtual ~PEnemyWrapper() = default; // ensure polymorphic

    explicit PEnemyWrapper(std::unique_ptr<PEnemy> e)
        : EnemyWrapper(std::unique_ptr<Enemy>(e.release())), penemy(dynamic_cast<PEnemy*>(getRaw()))
    {}

    float getPoisonLevel() const noexcept { return penemy->getPoisonLevel(); }
    bool poison() { return penemy->poison(); }

    std::string serialize() const noexcept {
        return "PEnemy: " + penemy->serialize();
    }

private:
    PEnemy* penemy;
};

#endif // PENEMY_WRAPPER_H
