#include "gamestatemanager.h"
#include "world.h"
#include "protagonist.h"
#include "enemy.h"
#include "penemy.h"
#include "xenemy.h"
#include "healthpack.h"
#include "portal.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QRandomGenerator>
#include <limits>
#include <memory>
#include <random>

bool GameStateManager::newGame(GameModel *model, QMap<int, std::shared_ptr<CachedLevel>> &levelCache)
{
    int lvl = model->getCurrentLevel();

    if (levelCache.contains(lvl)) {
        loadLevelFromCache(model, levelCache, lvl);
        emit model->modelReset();
        return true;
    }

    World w;
    try {
        w.createWorld(model->getLevelFiles()[model->getCurrentLevel()], 20, 25);
    } catch (...) {
        qWarning() << "Failed to create world";
        return false;
    }

    auto tileVec = w.getTiles();
    std::vector<std::unique_ptr<TileWrapper>> tileWrappers;
    tileWrappers.reserve(tileVec.size());
    int rows = w.getRows();
    int cols = w.getCols();

    for (auto &t : tileVec) {
        tileWrappers.push_back(std::make_unique<TileWrapper>(std::move(t)));
    }

    auto protagonist = std::make_unique<ProtagonistWrapper>(w.getProtagonist());

    auto enemyVec = w.getEnemies();
    std::vector<std::unique_ptr<EnemyWrapper>> enemyWrappers;
    enemyWrappers.reserve(enemyVec.size());
    for (auto &e : enemyVec) {
        PEnemy *pE = dynamic_cast<PEnemy*>(e.get());
        if (pE) {
            auto pEUnique = std::unique_ptr<PEnemy>(static_cast<PEnemy*>(e.release()));
            auto penemyWrapper = std::make_unique<PEnemyWrapper>(std::move(pEUnique));
            enemyWrappers.push_back(std::move(penemyWrapper));
        } else {
            enemyWrappers.push_back(std::make_unique<EnemyWrapper>(std::move(e)));
        }
    }

    convertRandomEnemiesToXEnemies(model, enemyWrappers, cols, rows);

    auto hpVec = w.getHealthPacks();
    std::vector<std::unique_ptr<HealthPack>> hpWrappers;
    hpWrappers.reserve(hpVec.size());
    for (auto &hp : hpVec) {
        hpWrappers.push_back(std::make_unique<HealthPack>(std::move(hp)));
    }

    // portal stuff
    int totalLevels = model->getLevelFiles().size();
    bool hasNext = (lvl < totalLevels - 1);     // Not the last level
    bool hasPrevious = (lvl > 0);

    std::vector<std::unique_ptr<Portal>> portalWrappers; // local to store portals


    QPoint randCoord = pickRandomValidTile(
        tileWrappers,
        rows,
        cols
        );
    auto forwardTile = std::make_unique<Tile>(randCoord.x(), randCoord.y(), 0.0f);
    portalWrappers.push_back(std::make_unique<Portal>(std::move(forwardTile), lvl+1,0,0));

    if (hasPrevious) {
        auto prevCached = levelCache.value(lvl - 1, nullptr);
        QPoint prevForwardCoord(-1, -1);
        if (prevCached) {
            prevForwardCoord = prevCached->forwardPortalCoord;
        }
        auto backwardTile = std::make_unique<Tile>(0, 0, 0.0f);
        portalWrappers.push_back(std::make_unique<Portal>(std::move(backwardTile), lvl-1,prevForwardCoord.x(),prevForwardCoord.y()));

    }
    model->setTiles(std::move(tileWrappers), rows, cols);
    model->setProtagonist(std::move(protagonist));
    model->setEnemies(std::move(enemyWrappers));
    model->setHealthPacks(std::move(hpWrappers));
    model->setPortals(std::move(portalWrappers));

    cacheCurrentLevel(model, levelCache, lvl, randCoord);

    emit model->modelReset();
    return true;
}
QPoint GameStateManager::pickRandomValidTile(
    const std::vector<std::unique_ptr<TileWrapper>> &tiles,
    int rows,
    int cols
    ) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distX(0, cols - 1);
    std::uniform_int_distribution<> distY(0, rows - 1);

    while (true) {
        int x = distX(gen);
        int y = distY(gen);

        // Skip (0,0)
        if (x == 0 && y == 0) {
            continue;
        }

        // Find the tile in 'tiles'
        for (auto &tw : tiles) {
            if (tw->getXPos() == x && tw->getYPos() == y) {
                if (tw->getValue() != std::numeric_limits<float>::infinity()) {
                    // Valid tile
                    return QPoint(x, y);
                }
                break;
            }
        }
    }
    // Technically never returns here
    return QPoint(-1, -1);
}

bool GameStateManager::restartGame(GameModel *model, QMap<int, std::shared_ptr<CachedLevel>> &levelCache)
{
    levelCache.clear();
    model->setCurrentLevel(0);
    return newGame(model, levelCache);
}

bool GameStateManager::saveGameToFile(GameModel *model, const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out << "Level " << model->getCurrentLevel() << "\n";
    out << "LevelFile " << model->getLevelFiles()[model->getCurrentLevel()] << "\n";

    auto *p = model->getProtagonist();
    out << "Protagonist " << p->getXPos() << " " << p->getYPos()
        << " " << p->getHealth() << " " << p->getEnergy() << "\n";

    auto &enemies = model->getEnemies();
    out << "Enemies " << enemies.size() << "\n";
    for (auto &e : enemies) {
        PEnemy *pE = dynamic_cast<PEnemy*>(e->getRaw());
        XEnemyWrapper *xE = dynamic_cast<XEnemyWrapper*>(e.get());
        out << e->getXPos() << " " << e->getYPos() << " "
            << e->getStrength() << " " << (e->isDefeated()?1:0);
        if (pE) {
            out << " " << pE->getPoisonLevel();
        }
        out << "\n";
    }

    auto &hps = model->getHealthPacks();
    out << "HealthPacks " << hps.size() << "\n";
    for (auto &hp : hps) {
        out << hp->getXPos() << " " << hp->getYPos() << " " << hp->getValue() << "\n";
    }

    auto &ports = model->getPortals();
    out << "Portals " << ports.size() << "\n";
    for (auto &pt : ports) {
        out << pt->getXPos()        << " "
            << pt->getYPos()        << " "
            << pt->getTargetLevel() << " "
            << pt->getTargetX()     << " "
            << pt->getTargetY()     << "\n";
    }

    return true;
}

bool GameStateManager::loadGameFromFile(GameModel *model, QMap<int, std::shared_ptr<CachedLevel>> &levelCache, const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    QTextStream in(&file);

    QString line = in.readLine();
    if (!line.startsWith("Level ")) return false;
    int lvl = line.mid(6).toInt();
    model->setCurrentLevel(lvl);

    line = in.readLine();
    if (!line.startsWith("LevelFile ")) return false;
    QString levelFile = line.mid(10);

    World w;
    try {
        w.createWorld(levelFile, 0, 0);
    } catch (...) {
        return false;
    }

    auto tileData = w.getTiles();
    int rows = w.getRows();
    int cols = w.getCols();

    std::vector<std::unique_ptr<TileWrapper>> tileWrappers;
    tileWrappers.reserve(tileData.size());
    for (auto &t : tileData) {
        tileWrappers.push_back(std::make_unique<TileWrapper>(std::move(t)));
    }

    auto protagonist = std::make_unique<ProtagonistWrapper>(w.getProtagonist());

    line = in.readLine();
    if (!line.startsWith("Protagonist ")) return false;
    auto tokens = line.split(" ");
    if (tokens.size() != 5) return false;
    int px = tokens[1].toInt();
    int py = tokens[2].toInt();
    float ph = tokens[3].toFloat();
    float pe = tokens[4].toFloat();
    protagonist->setPos(px, py);
    protagonist->setHealth(ph);
    protagonist->setEnergy(pe);

    line = in.readLine();
    if (!line.startsWith("Enemies ")) return false;
    int numEnemies = line.mid(8).toInt();
    std::vector<std::unique_ptr<EnemyWrapper>> enemies;
    enemies.reserve(numEnemies);
    for (int i=0; i<numEnemies; i++) {
        line = in.readLine();
        tokens = line.split(" ");
        if (tokens.size()<4) return false;
        int ex = tokens[0].toInt();
        int ey = tokens[1].toInt();
        float val = tokens[2].toFloat();
        bool defeated = tokens[3].toInt();
        bool isP = false, isX = false;
        float poisonLevel = 0;
        bool resurrectedX = false;
        if (tokens.size()>4) {
            if (tokens[4].startsWith("X")) {
                // XEnemy
                isX=true;
                resurrectedX=(tokens[4]=="X1");
            } else {
                // PEnemy
                poisonLevel = tokens[4].toFloat();
                isP=true;
            }
        }

        if (isP) {
            auto pE = std::make_unique<PEnemy>(ex, ey, val);
            pE->setPoisonLevel(poisonLevel);
            pE->setDefeated(defeated);
            enemies.push_back(std::make_unique<PEnemyWrapper>(std::move(pE)));
        } else if (isX) {
            auto e = std::make_unique<Enemy>(ex, ey, val);
            e->setDefeated(defeated);
            auto xE = std::make_unique<XEnemyWrapper>(std::move(e), cols, rows);
            // resurrectedX and defeated are handled, but we have no direct setter. Just accept state as is.
            enemies.push_back(std::move(xE));
        } else {
            auto e = std::make_unique<Enemy>(ex, ey, val);
            e->setDefeated(defeated);
            enemies.push_back(std::make_unique<EnemyWrapper>(std::move(e)));
        }
    }

    line = in.readLine();
    if (!line.startsWith("HealthPacks ")) return false;
    int numHP = line.mid(12).toInt();
    std::vector<std::unique_ptr<HealthPack>> hps;
    hps.reserve(numHP);
    for (int i=0; i<numHP; i++) {
        line = in.readLine();
        tokens = line.split(" ");
        if (tokens.size()!=3) return false;
        int hx = tokens[0].toInt();
        int hy = tokens[1].toInt();
        float hv = tokens[2].toFloat();
        auto hpTile = std::make_unique<Tile>(hx, hy, hv);
        hps.push_back(std::make_unique<HealthPack>(std::move(hpTile)));
    }
    QPoint forwardPortalCoord(-1, -1);
    line = in.readLine();
    if (!line.startsWith("Portals ")) return false;
    int numPortals = line.mid(8).toInt();
    std::vector<std::unique_ptr<Portal>> ports;
    ports.reserve(numPortals);
    for (int i=0; i<numPortals; i++) {
        line = in.readLine();
        tokens = line.split(" ");
        if (tokens.size()!=5) return false;
        int ptx         = tokens[0].toInt();
        int pty         = tokens[1].toInt();
        int targetLvl   = tokens[2].toInt();
        int targetX     = tokens[3].toInt();
        int targetY     = tokens[4].toInt();
        auto pTile = std::make_unique<Tile>(ptx, pty, 0.0f);
        ports.push_back(std::make_unique<Portal>(std::move(pTile),
                                                 targetLvl,
                                                 targetX,
                                                 targetY));
        if ((ptx != 0 || pty != 0) && forwardPortalCoord == QPoint(-1, -1)) {
            forwardPortalCoord = QPoint(ptx, pty);
        }
    }

    model->setTiles(std::move(tileWrappers), rows, cols);
    model->setProtagonist(std::move(protagonist));
    model->setEnemies(std::move(enemies));
    model->setHealthPacks(std::move(hps));
    model->setPortals(std::move(ports));

    cacheCurrentLevel(model, levelCache, lvl, forwardPortalCoord);

    emit model->modelReset();
    emit model->modelUpdated();
    return true;
}

void GameStateManager::loadLevelFromCache(GameModel *model, QMap<int, std::shared_ptr<CachedLevel>> &levelCache, int level)
{
    auto it = levelCache.find(level);
    if (it == levelCache.end()) {
        // not cached
        newGame(model, levelCache);
        return;
    }

    auto cached = it.value(); // std::shared_ptr<CachedLevel>
    model->setTiles(cloneTiles(cached->tiles), cached->rows, cached->cols);

    {
        auto origP = cached->protagonist->getRaw();
        auto newProtag = std::make_unique<Protagonist>();
        newProtag->setHealth(origP->getHealth());
        newProtag->setEnergy(origP->getEnergy());
        model->setProtagonist(std::make_unique<ProtagonistWrapper>(std::move(newProtag)));
    }
    model->setEnemies(cloneEnemies(model, cached->enemies));
    model->setHealthPacks(cloneHealthPacks(cached->healthPacks));
    model->setPortals(clonePortals(cached->portals));
}

void GameStateManager::cacheCurrentLevel(GameModel *model, QMap<int, std::shared_ptr<CachedLevel>> &levelCache, int level, const QPoint &forwardPortalCoord)
{
    auto c = std::make_shared<CachedLevel>();
    c->rows = model->getRows();
    c->cols = model->getCols();
    c->forwardPortalCoord = forwardPortalCoord;
    c->tiles = cloneTiles(model->getTiles());
    {
        auto origP = model->getProtagonist()->getRaw();
        auto newProtag = std::make_unique<Protagonist>();
        newProtag->setPos(0, 0);
        newProtag->setHealth(origP->getHealth());
        newProtag->setEnergy(origP->getEnergy());
        c->protagonist = std::make_unique<ProtagonistWrapper>(std::move(newProtag));
    }
    c->enemies = cloneEnemies(model, model->getEnemies());
    c->healthPacks = cloneHealthPacks(model->getHealthPacks());
    c->portals = clonePortals(model->getPortals());

    levelCache[level] = c;
}

std::vector<std::unique_ptr<TileWrapper>> GameStateManager::cloneTiles(const std::vector<std::unique_ptr<TileWrapper>> &source)
{
    std::vector<std::unique_ptr<TileWrapper>> result;
    result.reserve(source.size());
    for (auto &t : source) {
        auto tilePtr = std::make_unique<Tile>(t->getXPos(), t->getYPos(), t->getValue());
        result.push_back(std::make_unique<TileWrapper>(std::move(tilePtr)));
    }
    return result;
}

std::vector<std::unique_ptr<EnemyWrapper>> GameStateManager::cloneEnemies(GameModel *model, const std::vector<std::unique_ptr<EnemyWrapper>> &source)
{
    std::vector<std::unique_ptr<EnemyWrapper>> result;
    result.reserve(source.size());
    for (auto &e : source) {
        Enemy *rawE = e->getRaw();
        if (auto pE = dynamic_cast<PEnemy*>(rawE)) {
            auto pEc = std::make_unique<PEnemy>(pE->getXPos(), pE->getYPos(), pE->getValue());
            pEc->setPoisonLevel(pE->getPoisonLevel());
            pEc->setDefeated(pE->getDefeated());
            result.push_back(std::make_unique<PEnemyWrapper>(std::move(pEc)));
        } else if (auto xE = dynamic_cast<XEnemyWrapper*>(e.get())) {
            Enemy *base = e->getRaw();
            auto Ec = std::make_unique<Enemy>(base->getXPos(), base->getYPos(), base->getValue());
            Ec->setDefeated(base->getDefeated());
            auto xEc = std::make_unique<XEnemyWrapper>(std::move(Ec), model->getCols(), model->getRows());
            result.push_back(std::move(xEc));
        } else {
            auto Ec = std::make_unique<Enemy>(e->getXPos(), e->getYPos(), e->getStrength());
            Ec->setDefeated(e->isDefeated());
            result.push_back(std::make_unique<EnemyWrapper>(std::move(Ec)));
        }
    }
    return result;
}

std::vector<std::unique_ptr<HealthPack>> GameStateManager::cloneHealthPacks(const std::vector<std::unique_ptr<HealthPack>> &source)
{
    std::vector<std::unique_ptr<HealthPack>> result;
    result.reserve(source.size());
    for (auto &hp : source) {
        auto hc = std::make_unique<Tile>(hp->getXPos(), hp->getYPos(), hp->getValue());
        result.push_back(std::make_unique<HealthPack>(std::move(hc)));
    }
    return result;
}

std::vector<std::unique_ptr<Portal>> GameStateManager::clonePortals(const std::vector<std::unique_ptr<Portal>> &source)
{
    std::vector<std::unique_ptr<Portal>> result;
    result.reserve(source.size());
    for (auto &pt : source) {
        auto pc = std::make_unique<Tile>(pt->getXPos(), pt->getYPos(), pt->getValue());
        result.push_back(std::make_unique<Portal>(std::move(pc),
                                                  pt->getTargetLevel(),
                                                  pt->getTargetX(),
                                                  pt->getTargetY()));
    }
    return result;
}

void GameStateManager::convertRandomEnemiesToXEnemies(GameModel *model, std::vector<std::unique_ptr<EnemyWrapper>> &enemies, int cols, int rows)
{
    if (enemies.empty()) return;
    size_t count = enemies.size()/4;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0,(int)enemies.size()-1);
    for (size_t i=0;i<count;i++){
        int idx = dist(gen);
        if (dynamic_cast<PEnemyWrapper*>(enemies[idx].get()))
            continue;
        if (dynamic_cast<XEnemyWrapper*>(enemies[idx].get()))
            continue;
        Enemy *rawE = enemies[idx]->getRaw();
        float str = rawE->getValue();
        int x=rawE->getXPos();
        int y=rawE->getYPos();
        bool defeated = rawE->getDefeated();
        auto newE = std::make_unique<Enemy>(x,y,str);
        newE->setDefeated(defeated);
        auto xE = std::make_unique<XEnemyWrapper>(std::move(newE), cols, rows);
        enemies[idx]=std::move(xE);
    }
}
