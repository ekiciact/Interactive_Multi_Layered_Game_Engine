#include "gamemodel.h"
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QtMath>
#include <functional>
#include <random> // Include for random number generation

GameModel::GameModel(QObject *parent)
    : QObject(parent), autoPathIndex(0), currentTarget(TargetType::None),
    nodeComparator([](const Node &a, const Node &b) { return a.f > b.f; }),
    currentLevel(0)
{
    // Define level files
    levelFiles = {":/images/level1.png", ":/images/level2.png", ":/images/level3.png"};
    initializeWorld();

    autoPlayTimer = new QTimer(this);
    connect(autoPlayTimer, &QTimer::timeout, this, &GameModel::autoPlayStep);
}

GameModel::~GameModel()
{
    cleanupWorld();
}

void GameModel::initializeWorld()
{
    if (currentLevel >= levelFiles.size()) {
        // All levels completed
        emit gameOver();
        return;
    }

    world = new World();
    try {
        world->createWorld(levelFiles[currentLevel], 20, 25);
    } catch (const std::exception &e) {
        qWarning() << "Failed to create world:" << e.what();
        return;
    }

    protagonist = world->getProtagonist().release();

    for (auto &enemyPtr : world->getEnemies()) {
        Enemy *enemy = enemyPtr.release();
        if (PEnemy *pEnemy = dynamic_cast<PEnemy*>(enemy)) {
            connect(pEnemy, &PEnemy::poisonLevelUpdated,
                    [this, pEnemy](int level) {
                        Q_UNUSED(level);
                        handlePEnemyPoison(pEnemy);
                    });
        }
        enemies.append(enemy);
    }

    for (auto &hpPtr : world->getHealthPacks()) {
        healthPacks.append(hpPtr.release());
    }

    // Get tiles from world
    auto tileVector = world->getTiles();
    rows = world->getRows();
    cols = world->getCols();

    tiles.clear();
    tiles.reserve(tileVector.size());
    for (auto &tilePtr : tileVector) {
        Tile *tile = tilePtr.release();
        tiles.push_back(tile);
    }

    // Initialize nodes for pathfinding
    nodes.clear();
    nodes.reserve(rows * cols);
    for (Tile *tile : tiles) {
        Node node(tile->getXPos(), tile->getYPos(), tile->getValue());
        nodes.push_back(node);
    }

    // Place a portal on the map (without modifying the world class)
    // Rrandomly select a tile that's not a wall or occupied
    bool portalPlaced = false;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> distX(0, cols - 1);
    std::uniform_int_distribution<> distY(0, rows - 1);

    while (!portalPlaced) {
        int x = distX(gen);
        int y = distY(gen);
        Tile *tile = getTileAt(x, y);
        if (tile && tile->getValue() != std::numeric_limits<float>::infinity()) {
            bool occupied = false;
            for (Enemy *enemy : enemies) {
                if (enemy->getXPos() == x && enemy->getYPos() == y) {
                    occupied = true;
                    break;
                }
            }
            if (!occupied && !(x == 0 && y == 0)) {
                Tile *portal = new Tile(x, y, 0.0f); // Use Tile to represent a portal
                portals.append(portal);
                portalPlaced = true;
            }
        }
    }

    emit modelReset(); // Notify views that the model has been reset
}

void GameModel::cleanupWorld()
{
    delete world;
    delete protagonist;
    qDeleteAll(enemies);
    qDeleteAll(healthPacks);
    qDeleteAll(portals);

    for (Tile *tile : tiles) {
        delete tile;
    }
    tiles.clear();

    nodes.clear();
}

void GameModel::newGame()
{
    stopAutoPlay(); // Stop the timer before re-initializing
    currentLevel = 0;
    cleanupWorld();
    initializeWorld();
    emit modelUpdated();
}

void GameModel::restartGame()
{
    stopAutoPlay(); // Stop the timer before re-initializing
    cleanupWorld();
    initializeWorld();
    emit modelUpdated();
}

Tile* GameModel::getTileAt(int x, int y)
{
    if (x >= 0 && x < cols && y >= 0 && y < rows) {
        return tiles[y * cols + x];
    }
    return nullptr;
}

World* GameModel::getWorld() const
{
    return world;
}

Protagonist* GameModel::getProtagonist() const
{
    return protagonist;
}

QList<Enemy*> GameModel::getEnemies() const
{
    return enemies;
}

QList<Tile*> GameModel::getHealthPacks() const
{
    return healthPacks;
}

QList<Tile*> GameModel::getPortals() const
{
    return portals;
}

void GameModel::moveProtagonist(int dx, int dy)
{
    int newX = protagonist->getXPos() + dx;
    int newY = protagonist->getYPos() + dy;

    // Check boundaries and tile values
    if (newX >= 0 && newX < cols &&
        newY >= 0 && newY < rows) {
        Tile *tile = getTileAt(newX, newY);
        if (tile && tile->getValue() != std::numeric_limits<float>::infinity()) {
            // Deduct energy based on tile value
            float energyCost = 1.0f / (tile->getValue() + 1.0f) * 0.1f; // Inverted calculation
            float newEnergy = protagonist->getEnergy() - energyCost;
            if (newEnergy >= 0) {
                protagonist->setEnergy(newEnergy);
                protagonist->setPos(newX, newY);
                checkForHealthPacks(); // Check for health packs before encounters
                checkForEncounters();
                checkForPortal(); // Check for portal
                emit modelUpdated();

                if (protagonist->getHealth() <= 0 ||
                    protagonist->getEnergy() <= 0) {
                    emit gameOver();
                    stopAutoPlay();
                }
            } else {
                emit gameOver();
                stopAutoPlay();
            }
        }
    }
}

void GameModel::checkForEncounters()
{
    for (Enemy *enemy : enemies) {
        if (!enemy->getDefeated() &&
            enemy->getXPos() == protagonist->getXPos() &&
            enemy->getYPos() == protagonist->getYPos()) {
            // Engage in combat
            float healthCost = enemy->getValue(); // Enemy damage
            float newHealth = protagonist->getHealth() - healthCost;
            if (newHealth > 0) {
                protagonist->setHealth(newHealth);
                enemy->setDefeated(true);

                if (PEnemy *pEnemy = dynamic_cast<PEnemy*>(enemy)) {
                    // Start poison effect
                    pEnemy->poison();
                }

                emit modelUpdated();
            } else {
                protagonist->setHealth(0);
                emit gameOver();
                stopAutoPlay();
            }
            break;
        }
    }
}

void GameModel::checkForHealthPacks()
{
    for (Tile *hp : healthPacks) {
        if (hp->getXPos() == protagonist->getXPos() &&
            hp->getYPos() == protagonist->getYPos()) {
            // Increase health
            float healthBoost = hp->getValue(); // Health pack value
            float newHealth = protagonist->getHealth() + healthBoost;
            if (newHealth > 100.0f)
                newHealth = 100.0f;
            protagonist->setHealth(newHealth);
            healthPacks.removeOne(hp);
            delete hp;
            emit modelUpdated();
            break;
        }
    }
}

void GameModel::checkForPortal()
{
    for (Tile *portal : portals) {
        if (portal->getXPos() == protagonist->getXPos() &&
            portal->getYPos() == protagonist->getYPos()) {
            // Proceed to next level
            currentLevel++;
            if (currentLevel < levelFiles.size()) {
                stopAutoPlay(); // Stop the timer before re-initializing
                cleanupWorld();
                initializeWorld();
                emit modelUpdated();
            } else {
                // Game completed
                emit gameOver();
            }
            break;
        }
    }
}

void GameModel::handlePEnemyPoison(PEnemy *pEnemy)
{
    // Implement the poisoning effect on surrounding tiles or protagonist
    int radius = static_cast<int>(pEnemy->getPoisonLevel() / 10);
    int centerX = pEnemy->getXPos();
    int centerY = pEnemy->getYPos();

    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            int x = centerX + dx;
            int y = centerY + dy;
            if (x >= 0 && x < cols &&
                y >= 0 && y < rows) {
                if (protagonist->getXPos() == x &&
                    protagonist->getYPos() == y) {
                    // Protagonist is in poison area
                    float newHealth = protagonist->getHealth() - 5.0f;
                    protagonist->setHealth(newHealth);
                    if (newHealth <= 0) {
                        emit gameOver();
                        stopAutoPlay();
                    }
                }
            }
        }
    }
}

Enemy* GameModel::findNextTargetEnemy()
{
    Enemy *targetEnemy = nullptr;
    float minDistance = std::numeric_limits<float>::max();

    for (Enemy *enemy : enemies) {
        if (!enemy->getDefeated()) {
            float dist = qSqrt(qPow(enemy->getXPos() - protagonist->getXPos(), 2) +
                               qPow(enemy->getYPos() - protagonist->getYPos(), 2));
            if (dist < minDistance) {
                minDistance = dist;
                targetEnemy = enemy;
            }
        }
    }

    return targetEnemy;
}

void GameModel::planPathToEnemy(Enemy* targetEnemy)
{
    Node& startNode = nodes[protagonist->getYPos() * cols + protagonist->getXPos()];
    Node& endNode = nodes[targetEnemy->getYPos() * cols + targetEnemy->getXPos()];

    // Reset nodes
    resetNodes();

    // Define cost and heuristic functions
    auto costFunc = [this, targetEnemy](const Node &a, const Node &b) {
        Q_UNUSED(a);

        // Check if the node is occupied by an enemy other than the target
        for (Enemy* enemy : enemies) {
            if (!enemy->getDefeated() &&
                enemy != targetEnemy &&
                enemy->getXPos() == b.getXPos() &&
                enemy->getYPos() == b.getYPos()) {
                return std::numeric_limits<float>::infinity();
            }
        }

        // Calculate energy cost
        float energyCost = 1.0f / (b.getValue() + 1.0f) * 0.1f;

        return energyCost;
    };

    auto heuristicFunc = [](const Node &a, const Node &b) {
        return qSqrt(qPow(a.getXPos() - b.getXPos(), 2) +
                     qPow(a.getYPos() - b.getYPos(), 2));
    };

    PathFinder<Node, Node> pathfinder(nodes, &startNode, &endNode,
                                      nodeComparator, static_cast<unsigned int>(cols), costFunc, heuristicFunc, 1.0f);

    autoPath = pathfinder.A_star();
    autoPathIndex = 0;
}

void GameModel::planPathToHealthPack()
{
    Node& startNode = nodes[protagonist->getYPos() * cols + protagonist->getXPos()];

    // Find the nearest health pack
    Tile* nearestHealthPack = nullptr;
    float minDistance = std::numeric_limits<float>::max();

    for (Tile* hp : healthPacks) {
        float dist = qSqrt(qPow(hp->getXPos() - protagonist->getXPos(), 2) +
                           qPow(hp->getYPos() - protagonist->getYPos(), 2));
        if (dist < minDistance) {
            minDistance = dist;
            nearestHealthPack = hp;
        }
    }

    if (!nearestHealthPack) {
        // No health packs available
        autoPath.clear();
        autoPathIndex = 0;
        return;
    }

    Node& endNode = nodes[nearestHealthPack->getYPos() * cols + nearestHealthPack->getXPos()];

    // Reset nodes
    resetNodes();

    // Define cost and heuristic functions
    auto costFunc = [this](const Node &a, const Node &b) {
        Q_UNUSED(a);

        // Check if the node is occupied by an enemy
        for (Enemy* enemy : enemies) {
            if (!enemy->getDefeated() &&
                enemy->getXPos() == b.getXPos() &&
                enemy->getYPos() == b.getYPos()) {
                return std::numeric_limits<float>::infinity();
            }
        }

        // Calculate energy cost
        float energyCost = 1.0f / (b.getValue() + 1.0f) * 0.1f;

        return energyCost;
    };

    auto heuristicFunc = [](const Node &a, const Node &b) {
        return qSqrt(qPow(a.getXPos() - b.getXPos(), 2) +
                     qPow(a.getYPos() - b.getYPos(), 2));
    };

    PathFinder<Node, Node> pathfinder(nodes, &startNode, &endNode,
                                      nodeComparator, static_cast<unsigned int>(cols), costFunc, heuristicFunc, 1.0f);

    autoPath = pathfinder.A_star();
    autoPathIndex = 0;
}

void GameModel::planPathToPortal()
{
    if (!portals.isEmpty()) {
        Tile *portal = portals.first();
        Node& startNode = nodes[protagonist->getYPos() * cols + protagonist->getXPos()];
        Node& endNode = nodes[portal->getYPos() * cols + portal->getXPos()];

        // Reset nodes
        resetNodes();

        // Define cost and heuristic functions
        auto costFunc = [this](const Node &a, const Node &b) {
            Q_UNUSED(a);

            // Check if the node is occupied by an enemy
            for (Enemy* enemy : enemies) {
                if (!enemy->getDefeated() &&
                    enemy->getXPos() == b.getXPos() &&
                    enemy->getYPos() == b.getYPos()) {
                    return std::numeric_limits<float>::infinity();
                }
            }

            // Calculate energy cost
            float energyCost = 1.0f / (b.getValue() + 1.0f) * 0.1f;
            return energyCost;
        };

        auto heuristicFunc = [](const Node &a, const Node &b) {
            return qSqrt(qPow(a.getXPos() - b.getXPos(), 2) +
                         qPow(a.getYPos() - b.getYPos(), 2));
        };

        PathFinder<Node, Node> pathfinder(nodes, &startNode, &endNode,
                                          nodeComparator, static_cast<unsigned int>(cols), costFunc, heuristicFunc, 1.0f);

        autoPath = pathfinder.A_star();
        autoPathIndex = 0;
    } else {
        // No portal, cannot proceed
        autoPath.clear();
        autoPathIndex = 0;
    }
}

void GameModel::decideNextAction()
{
    Enemy* targetEnemy = findNextTargetEnemy();
    if (targetEnemy) {
        float enemyDamage = targetEnemy->getValue();
        if (protagonist->getHealth() <= enemyDamage) {
            planPathToHealthPack();
            currentTarget = TargetType::HealthPack;
        } else if (protagonist->getHealth() < 70.0f) {
            planPathToEnemyWithHealthPacks(targetEnemy);
            currentTarget = TargetType::Enemy;
        } else {
            planPathToEnemy(targetEnemy);
            currentTarget = TargetType::Enemy;
        }
        autoPathIndex = 0;
    } else {
        // No enemies left, plan path to portal
        planPathToPortal();
        currentTarget = TargetType::Portal;
        autoPathIndex = 0;
    }
}

void GameModel::startAutoPlay()
{
    // Check if autoPlayTimer is already running
    if (autoPlayTimer->isActive()) {
        return;
    }

    // Check if the protagonist is alive and has energy
    if (protagonist->getHealth() <= 0 || protagonist->getEnergy() <= 0) {
        emit gameOver();
        return;
    }

    decideNextAction();

    autoPlayTimer->start(500); // Start the autoPlayTimer
}

void GameModel::stopAutoPlay()
{
    autoPlayTimer->stop();
    currentTarget = TargetType::None;
}

void GameModel::autoPlayStep()
{
    if (protagonist->getHealth() <= 0 || protagonist->getEnergy() <= 0) {
        // Protagonist is dead or out of energy
        stopAutoPlay();
        emit gameOver();
        return;
    }

    if (autoPathIndex < static_cast<int>(autoPath.size())) {
        int move = autoPath[autoPathIndex++];
        int dx = 0, dy = 0;
        switch (move) {
        case 0: dy = -1; break;
        case 1: dx = 1; dy = -1; break;
        case 2: dx = 1; break;
        case 3: dx = 1; dy = 1; break;
        case 4: dy = 1; break;
        case 5: dx = -1; dy = 1; break;
        case 6: dx = -1; break;
        case 7: dx = -1; dy = -1; break;
        }
        moveProtagonist(dx, dy);

        // After moving, check if we reached the end of the path
        if (autoPathIndex >= static_cast<int>(autoPath.size())) {
            // Path completed
            decideNextAction();
        }

    } else {
        // No path, need to decide what to do
        decideNextAction();
    }
}

void GameModel::resetNodes()
{
    for (Node &node : nodes) {
        node.f = node.g = node.h = 0;
        node.visited = node.closed = false;
        node.prev = nullptr;
    }
}

void GameModel::planPathToEnemyWithHealthPacks(Enemy* targetEnemy)
{
    Node& startNode = nodes[protagonist->getYPos() * cols + protagonist->getXPos()];
    Node& endNode = nodes[targetEnemy->getYPos() * cols + targetEnemy->getXPos()];

    // Reset nodes
    resetNodes();

    // Adjust cost function to consider energy cost and health packs
    auto costFunc = [this, targetEnemy](const Node &a, const Node &b) {
        Q_UNUSED(a);

        // Check if the node is occupied by an enemy other than the target
        for (Enemy* enemy : enemies) {
            if (!enemy->getDefeated() &&
                enemy != targetEnemy &&
                enemy->getXPos() == b.getXPos() &&
                enemy->getYPos() == b.getYPos()) {
                return std::numeric_limits<float>::infinity();
            }
        }

        // Calculate energy cost
        float energyCost = 1.0f / (b.getValue() + 1.0f) * 0.1f;

        // Check if the node corresponds to a tile with a health pack
        for (Tile* hp : healthPacks) {
            if (hp->getXPos() == b.getXPos() && hp->getYPos() == b.getYPos()) {
                // Reduce cost to favor paths through health packs
                energyCost *= 0.5f;
                break;
            }
        }

        return energyCost;
    };

    auto heuristicFunc = [](const Node &a, const Node &b) {
        return qSqrt(qPow(a.getXPos() - b.getXPos(), 2) +
                     qPow(a.getYPos() - b.getYPos(), 2));
    };

    PathFinder<Node, Node> pathfinder(nodes, &startNode, &endNode,
                                      nodeComparator, static_cast<unsigned int>(cols), costFunc, heuristicFunc, 1.0f);

    autoPath = pathfinder.A_star();
    autoPathIndex = 0;
}

bool GameModel::saveGame(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Cannot open file for writing:" << file.errorString();
        return false;
    }

    QTextStream out(&file);

    // Save current level
    out << "Level " << currentLevel << "\n";
    out << "LevelFile " << levelFiles[currentLevel] << "\n";

    // Save protagonist state
    out << "Protagonist " << protagonist->getXPos() << " " << protagonist->getYPos()
        << " " << protagonist->getHealth() << " " << protagonist->getEnergy() << "\n";

    // Save enemies
    out << "Enemies " << enemies.size() << "\n";
    for (Enemy *enemy : enemies) {
        out << enemy->getXPos() << " " << enemy->getYPos() << " "
            << enemy->getValue() << " " << enemy->getDefeated();
        if (PEnemy *pEnemy = dynamic_cast<PEnemy*>(enemy)) {
            out << " " << pEnemy->getPoisonLevel();
        }
        out << "\n";
    }

    // Save health packs
    out << "HealthPacks " << healthPacks.size() << "\n";
    for (Tile *hp : healthPacks) {
        out << hp->getXPos() << " " << hp->getYPos() << " " << hp->getValue() << "\n";
    }

    // Save portals
    out << "Portals " << portals.size() << "\n";
    for (Tile *portal : portals) {
        out << portal->getXPos() << " " << portal->getYPos() << "\n";
    }

    file.close();
    qDebug() << "Game saved to" << fileName;
    return true;
}

bool GameModel::loadGame(const QString &fileName)
{
    stopAutoPlay(); // Stop the timer before loading the game

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open file for reading:" << file.errorString();
        return false;
    }

    QTextStream in(&file);

    cleanupWorld();

    QString line;
    line = in.readLine();
    if (!line.startsWith("Level ")) {
        qWarning() << "Invalid save file format";
        return false;
    }
    currentLevel = line.mid(6).toInt();

    // Read level file name
    line = in.readLine();
    if (!line.startsWith("LevelFile ")) {
        qWarning() << "Invalid save file format";
        return false;
    }
    QString levelFile = line.mid(10);

    // Now create the world from the level file
    world = new World();
    try {
        world->createWorld(levelFile, 0, 0); // Pass 0 enemies and health packs, we'll load them from the save file
    } catch (const std::exception &e) {
        qWarning() << "Failed to create world:" << e.what();
        return false;
    }

    // Create protagonist
    protagonist = world->getProtagonist().release();

    // Get tiles from world
    auto tileVector = world->getTiles();
    rows = world->getRows();
    cols = world->getCols();

    tiles.clear();
    tiles.reserve(tileVector.size());
    for (auto &tilePtr : tileVector) {
        Tile *tile = tilePtr.release();
        tiles.push_back(tile);
    }

    // Initialize nodes for pathfinding
    nodes.clear();
    nodes.reserve(rows * cols);
    for (Tile *tile : tiles) {
        Node node(tile->getXPos(), tile->getYPos(), tile->getValue());
        nodes.push_back(node);
    }

    // Load protagonist state
    line = in.readLine();
    if (!line.startsWith("Protagonist ")) {
        qWarning() << "Invalid save file format";
        return false;
    }
    QStringList tokens = line.split(" ");
    if (tokens.size() != 5) {
        qWarning() << "Invalid protagonist data";
        return false;
    }
    int x = tokens[1].toInt();
    int y = tokens[2].toInt();
    float health = tokens[3].toFloat();
    float energy = tokens[4].toFloat();
    protagonist->setPos(x, y);
    protagonist->setHealth(health);
    protagonist->setEnergy(energy);

    // Load enemies
    line = in.readLine();
    if (!line.startsWith("Enemies ")) {
        qWarning() << "Invalid save file format";
        return false;
    }
    int numEnemies = line.mid(8).toInt();
    enemies.clear();
    for (int i = 0; i < numEnemies; ++i) {
        line = in.readLine();
        tokens = line.split(" ");
        if (tokens.size() < 4) {
            qWarning() << "Invalid enemy data";
            return false;
        }
        int ex = tokens[0].toInt();
        int ey = tokens[1].toInt();
        float value = tokens[2].toFloat();
        bool defeated = tokens[3].toInt();
        Enemy *enemy = nullptr;
        if (tokens.size() == 5) {
            // PEnemy
            float poisonLevel = tokens[4].toFloat();
            PEnemy *pEnemy = new PEnemy(ex, ey, value);
            pEnemy->setPoisonLevel(poisonLevel);
            enemy = pEnemy;

            connect(pEnemy, &PEnemy::poisonLevelUpdated,
                    [this, pEnemy](int level) {
                        Q_UNUSED(level);
                        handlePEnemyPoison(pEnemy);
                    });
        } else {
            enemy = new Enemy(ex, ey, value);
        }
        enemy->setDefeated(defeated);
        enemies.append(enemy);
    }

    // Load health packs
    line = in.readLine();
    if (!line.startsWith("HealthPacks ")) {
        qWarning() << "Invalid save file format";
        return false;
    }
    int numHealthPacks = line.mid(12).toInt();
    healthPacks.clear();
    for (int i = 0; i < numHealthPacks; ++i) {
        line = in.readLine();
        tokens = line.split(" ");
        if (tokens.size() != 3) {
            qWarning() << "Invalid health pack data";
            return false;
        }
        int hx = tokens[0].toInt();
        int hy = tokens[1].toInt();
        float value = tokens[2].toFloat();
        Tile *hp = new Tile(hx, hy, value);
        healthPacks.append(hp);
    }

    // Load portals
    line = in.readLine();
    if (!line.startsWith("Portals ")) {
        qWarning() << "Invalid save file format";
        return false;
    }
    int numPortals = line.mid(8).toInt();
    portals.clear();
    for (int i = 0; i < numPortals; ++i) {
        line = in.readLine();
        tokens = line.split(" ");
        if (tokens.size() != 2) {
            qWarning() << "Invalid portal data";
            return false;
        }
        int px = tokens[0].toInt();
        int py = tokens[1].toInt();
        Tile *portal = new Tile(px, py, 0.0f);
        portals.append(portal);
    }

    file.close();
    emit modelReset(); // Notify views that the model has been reset
    emit modelUpdated();
    qDebug() << "Game loaded from" << fileName;
    return true;
}
