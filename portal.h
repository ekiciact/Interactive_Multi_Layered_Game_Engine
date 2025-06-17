#ifndef PORTAL_H
#define PORTAL_H

#include "tile.h"

/**
 * @brief A Portal tile that moves the player to the next level.
 */
class Portal : public TileWrapper {
public:
    explicit Portal(std::unique_ptr<Tile> baseTile, int targetLevel, int targetX = 0, int targetY = 0)
        : TileWrapper(std::move(baseTile))
        , m_targetLevel(targetLevel)
        , m_targetX(targetX)
        , m_targetY(targetY)
    {}

    int getTargetLevel() const { return m_targetLevel; }
    int getTargetX() const { return m_targetX; }
    int getTargetY() const { return m_targetY; }

    std::string serialize() const noexcept override {
        // Example: "Portal to Level 2 at [3,4]"
        return "Portal -> L" + std::to_string(m_targetLevel) +
               " [" + std::to_string(m_targetX) + "," + std::to_string(m_targetY) + "] " +
               TileWrapper::serialize();
    }

private:
    int m_targetLevel;
    int m_targetX;
    int m_targetY;
};

#endif // PORTAL_H
