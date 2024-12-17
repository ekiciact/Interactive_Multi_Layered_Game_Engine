#ifndef GAMESTATEMANAGER_H
#define GAMESTATEMANAGER_H

#include <memory>
#include <QMap>
#include <QString>
#include <vector>
#include "gamemodel.h"

class GameStateManager {
public:
    struct CachedLevel {
        std::vector<std::unique_ptr<TileWrapper>> tiles;
        std::unique_ptr<ProtagonistWrapper> protagonist;
        std::vector<std::unique_ptr<EnemyWrapper>> enemies;
        std::vector<std::unique_ptr<HealthPack>> healthPacks;
        std::vector<std::unique_ptr<Portal>> portals;
        int rows;
        int cols;
    };

    GameStateManager() = default;

    // Load a new game level from scratch
    bool newGame(GameModel *model, QMap<int, std::shared_ptr<CachedLevel>> &levelCache);

    // Restart current game level
    bool restartGame(GameModel *model, QMap<int, std::shared_ptr<CachedLevel>> &levelCache);

    // Save game to file
    bool saveGameToFile(GameModel *model, const QString &fileName);

    // Load game from file
    bool loadGameFromFile(GameModel *model, QMap<int, std::shared_ptr<CachedLevel>> &levelCache, const QString &fileName);

    // Load cached level
    void loadLevelFromCache(GameModel *model, QMap<int, std::shared_ptr<CachedLevel>> &levelCache, int level);

    // Cache current level
    void cacheCurrentLevel(GameModel *model, QMap<int, std::shared_ptr<CachedLevel>> &levelCache, int level);

private:
    // clone helper functions
    std::vector<std::unique_ptr<TileWrapper>> cloneTiles(const std::vector<std::unique_ptr<TileWrapper>> &source);
    std::vector<std::unique_ptr<EnemyWrapper>> cloneEnemies(GameModel *model, const std::vector<std::unique_ptr<EnemyWrapper>> &source);
    std::vector<std::unique_ptr<HealthPack>> cloneHealthPacks(const std::vector<std::unique_ptr<HealthPack>> &source);
    std::vector<std::unique_ptr<Portal>> clonePortals(const std::vector<std::unique_ptr<Portal>> &source);

    // Randomly convert some enemies to XEnemies
    void convertRandomEnemiesToXEnemies(GameModel *model, std::vector<std::unique_ptr<EnemyWrapper>> &enemies, int cols, int rows);
};

#endif // GAMESTATEMANAGER_H
