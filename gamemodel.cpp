#include "gamemodel.h"
#include <limits>

GameModel::GameModel(QObject *parent)
    : QObject(parent), rows(0), cols(0), currentLevel(0)
{
    // Level files here
    levelFiles = {":/images/level1.png", ":/images/level2.png", ":/images/level3.png"};
}

GameModel::~GameModel() {
}

void GameModel::setProtagonist(std::unique_ptr<ProtagonistWrapper> p) {
    protagonist = std::move(p);
    emit modelUpdated();
}

void GameModel::setTiles(std::vector<std::unique_ptr<TileWrapper>> t, int r, int c) {
    tiles = std::move(t);
    rows = r;
    cols = c;
    emit modelUpdated();
}

void GameModel::setEnemies(std::vector<std::unique_ptr<EnemyWrapper>> e) {
    enemies = std::move(e);
    emit modelUpdated();
}

void GameModel::setHealthPacks(std::vector<std::unique_ptr<HealthPack>> hp) {
    healthPacks = std::move(hp);
    emit modelUpdated();
}

void GameModel::setPortals(std::vector<std::unique_ptr<Portal>> p) {
    portals = std::move(p);
    emit modelUpdated();
}
