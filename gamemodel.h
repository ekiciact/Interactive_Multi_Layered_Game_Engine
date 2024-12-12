#ifndef GAMEMODEL_H
#define GAMEMODEL_H

#include <QObject>
#include <QVector>
#include <QList>
#include <memory>

#include "world.h"
#include "protagonist.h"
#include "enemy.h"
#include "penemy.h"
#include "xenemy.h"
#include "healthpack.h"
#include "portal.h"
#include "tile.h"
#include <vector>

/**
 * The GameModel primarily holds the game's state (protagonist, enemies, tiles, health packs, portals).

 *
 * GameModel just stores and provides access to data.
 *
 * Signals:
 * - modelUpdated(): Emitted when the model's data changes and the view should update.
 * - gameOver(): Emitted when the game is over.
 * - modelReset(): Emitted when the model state is reset (like starting a new game or loading a saved game).
 */

class GameModel : public QObject {
    Q_OBJECT

public:
    explicit GameModel(QObject *parent = nullptr);
    ~GameModel();

    // Accessors
    int getRows() const { return rows; }
    int getCols() const { return cols; }

    ProtagonistWrapper* getProtagonist() const { return protagonist.get(); }
    const std::vector<std::unique_ptr<EnemyWrapper>>& getEnemies() const { return enemies; }
    const std::vector<std::unique_ptr<HealthPack>>& getHealthPacks() const { return healthPacks; }
    const std::vector<std::unique_ptr<Portal>>& getPortals() const { return portals; }
    const std::vector<std::unique_ptr<TileWrapper>>& getTiles() const { return tiles; }

    // Mutators
    void setProtagonist(std::unique_ptr<ProtagonistWrapper> p);
    void setTiles(std::vector<std::unique_ptr<TileWrapper>> t, int r, int c);
    void setEnemies(std::vector<std::unique_ptr<EnemyWrapper>> e);
    void setHealthPacks(std::vector<std::unique_ptr<HealthPack>> hp);
    void setPortals(std::vector<std::unique_ptr<Portal>> p);

    void setCurrentLevel(int level) { currentLevel = level; }
    int getCurrentLevel() const { return currentLevel; }

    // For loading/saving and new/restart games
    const QVector<QString>& getLevelFiles() const { return levelFiles; }

signals:
    void modelUpdated();
    void gameOver();
    void modelReset();

private:
    int rows;
    int cols;

    std::unique_ptr<ProtagonistWrapper> protagonist;
    std::vector<std::unique_ptr<EnemyWrapper>> enemies;
    std::vector<std::unique_ptr<HealthPack>> healthPacks;
    std::vector<std::unique_ptr<Portal>> portals;
    std::vector<std::unique_ptr<TileWrapper>> tiles;

    int currentLevel;
    QVector<QString> levelFiles; // Levels

    friend class GameController; // Allow GameController access if needed
};

#endif // GAMEMODEL_H
