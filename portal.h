#ifndef PORTAL_H
#define PORTAL_H

#include "tile.h"

/**
 * @brief A Portal tile that moves the player to the next level.
 */
class Portal : public TileWrapper {
public:
    explicit Portal(std::unique_ptr<Tile> baseTile)
        : TileWrapper(std::move(baseTile))
    {}

    std::string serialize() const noexcept override {
        return "Portal: " + TileWrapper::serialize();
    }
};

#endif // PORTAL_H
