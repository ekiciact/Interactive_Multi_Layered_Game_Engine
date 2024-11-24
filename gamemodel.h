#ifndef GAMEMODEL_H
#define GAMEMODEL_H

#include <QObject>
#include <QList>
#include <QTimer>
#include <vector>
#include "world.h"
#include "pathfinder_class.h"

// Forward declaration
class Level;

class Node : public Tile
{
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
        : Tile(other.getXPos(), other.getYPos(), other.getValue()), f(other.f), g(other.g), h(other.h), visited(other.visited), closed(other.closed), prev(other.prev)
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

    // Required for priority_queue comparison
    bool operator>(const Node &other) const
    {
        return f > other.f;
    }
};

class GameModel : public QObject
{
    Q_OBJECT

public:
    explicit GameModel(QObject *parent = nullptr);
    ~GameModel();

    Tile* getTileAt(int x, int y);
    World* getWorld() const;
    Protagonist* getProtagonist() const;
    QList<Enemy*> getEnemies() const;
    QList<Tile*> getHealthPacks() const;
    QList<Tile*> getPortals() const;

    void moveProtagonist(int dx, int dy);
    void startAutoPlay();
    void stopAutoPlay();

    bool saveGame(const QString &fileName);
    bool loadGame(const QString &fileName);

    void newGame();
    void restartGame();

    int getCurrentLevel() const { return currentLevel; } // Getter for currentLevel

signals:
    void modelUpdated();
    void gameOver();
    void modelReset(); // New signal to indicate the model has been reset

private slots:
    void autoPlayStep();

private:
    void initializeWorld();
    void cleanupWorld();

    void checkForEncounters();
    void checkForHealthPacks();
    void checkForPortal();
    void handlePEnemyPoison(PEnemy *pEnemy);

    // Methods for auto-play logic
    Enemy* findNextTargetEnemy();
    void planPathToEnemy(Enemy* targetEnemy);
    void planPathToHealthPack();
    void planPathToPortal();
    void planPathToEnemyWithHealthPacks(Enemy* targetEnemy);
    void decideNextAction();
    void resetNodes();

    enum class TargetType { None, Enemy, HealthPack, Portal };
    TargetType currentTarget;

    World *world;
    Protagonist *protagonist;
    QList<Enemy*> enemies;
    QList<Tile*> healthPacks;
    QList<Tile*> portals;    // List of portals
    QVector<Tile*> tiles;
    std::vector<Node> nodes;

    int rows;
    int cols;

    QTimer *autoPlayTimer;
    std::vector<int> autoPath;
    int autoPathIndex;

    // Comparator for nodes
    Comparator<Node> nodeComparator;

    int currentLevel;
    QVector<QString> levelFiles;
};

#endif // GAMEMODEL_H
