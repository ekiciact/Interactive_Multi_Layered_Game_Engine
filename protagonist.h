#ifndef PROTAGONIST_WRAPPER_H
#define PROTAGONIST_WRAPPER_H

#include "world.h"
#include <memory>
#include <string>

class ProtagonistWrapper {
public:
    explicit ProtagonistWrapper(std::unique_ptr<Protagonist> p)
        : protagonist(std::move(p))
    {}

    int getXPos() const noexcept { return protagonist->getXPos(); }
    int getYPos() const noexcept { return protagonist->getYPos(); }
    void setPos(int x, int y) noexcept { protagonist->setPos(x, y); }

    float getHealth() const noexcept { return protagonist->getHealth(); }
    void setHealth(float h) noexcept { protagonist->setHealth(h); }

    float getEnergy() const noexcept { return protagonist->getEnergy(); }
    void setEnergy(float e) noexcept { protagonist->setEnergy(e); }

    std::string serialize() const noexcept {
        return "Protagonist: " + protagonist->serialize();
    }

    Protagonist* getRaw() const noexcept { return protagonist.get(); } // Added getter

private:
    std::unique_ptr<Protagonist> protagonist;
};

#endif // PROTAGONIST_WRAPPER_H
