#ifndef TILE_WRAPPER_H
#define TILE_WRAPPER_H

#include "world.h"
#include <memory>
#include <string>

/**
 * @brief Wrapper around a Tile pointer from the library.
 * We store the Tile in a std::unique_ptr to ensure automatic cleanup.
 */
class TileWrapper {
public:
    explicit TileWrapper(std::unique_ptr<Tile> baseTile)
        : tile(std::move(baseTile))
    {}

    virtual ~TileWrapper() = default;

    int getXPos() const noexcept { return tile->getXPos(); }
    int getYPos() const noexcept { return tile->getYPos(); }
    float getValue() const noexcept { return tile->getValue(); }

    void setXPos(int x) noexcept { tile->setXPos(x); }
    void setYPos(int y) noexcept { tile->setYPos(y); }

    virtual std::string serialize() const noexcept { return tile->serialize(); }

protected:
    std::unique_ptr<Tile> tile;
};

#endif // TILE_WRAPPER_H
