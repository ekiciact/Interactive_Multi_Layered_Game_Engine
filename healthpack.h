#ifndef HEALTHPACK_H
#define HEALTHPACK_H

#include "tile.h"

/**
 * @brief Represents a HealthPack in the game world. It derives from TileWrapper,
 *        but logically represents a health-restoring item.
 */
class HealthPack : public TileWrapper {
public:
    explicit HealthPack(std::unique_ptr<Tile> baseTile)
        : TileWrapper(std::move(baseTile))
    {}

    // The value of this tile represents how much health it restores.
    float getHealAmount() const noexcept { return getValue(); }

    // If you want to serialize:
    std::string serialize() const noexcept override {
        return "HealthPack: " + TileWrapper::serialize();
    }
};

#endif // HEALTHPACK_H
