#ifndef NODE_H
#define NODE_H

#include "world.h"
#include <limits>

/**
 * @brief Node class for pathfinding, placed in its own header so both GameModel and GameController
 *        can access it.
 */
class Node : public Tile {
public:
    float f;
    float g;
    float h;
    bool visited;
    bool closed;
    Node* prev;

    Node(int xPosition, int yPosition, float tileValue)
        : Tile(xPosition, yPosition, tileValue), f(0), g(0), h(0), visited(false), closed(false), prev(nullptr)
    {}

    Node(const Node &other)
        : Tile(other.getXPos(), other.getYPos(), other.getValue()), f(other.f), g(other.g),
        h(other.h), visited(other.visited), closed(other.closed), prev(other.prev)
    {}

    Node& operator=(const Node &other)
    {
        if (this != &other)
        {
            xPos = other.getXPos();
            yPos = other.getYPos();
            value = other.getValue();
            f = other.f;
            g = other.g;
            h = other.h;
            visited = other.visited;
            closed = other.closed;
            prev = other.prev;
        }
        return *this;
    }

    bool operator>(const Node &other) const
    {
        return f > other.f;
    }
};

#endif // NODE_H
