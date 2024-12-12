#include "gamecontroller.h"
#include "world.h"
#include "healthpack.h"
#include "portal.h"
#include "protagonist.h"
#include "enemy.h"
#include "penemy.h"
#include "xenemy.h"

#include <QAction>
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>
#include <limits>
#include <cmath>
#include <random>
#include <memory>

GameController::GameController(QWidget *parent)
    : QMainWindow(parent),
    model(new GameModel(this)),
    autoPlayTimer(new QTimer(this)),
    autoPathIndex(0),
    currentTarget(TargetType::None),
    nodeComparator([](const Node &a, const Node &b) { return a.f > b.f; })
{
    setupModel();
    setupViews();
    setupConnections();
    createActions();
    createMenus();
    setupCommands();
}

GameController::~GameController()
{
}

void GameController::show()
{
    QMainWindow::show();
}

void GameController::setupModel()
{
    model->setCurrentLevel(0);
    newGame();
}

void GameController::setupViews()
{
    graphicView = new GameView(model, this);
    textView = new TextGameView(model, this);

    stackedWidget = new QStackedWidget(this);
    stackedWidget->addWidget(graphicView);
    stackedWidget->addWidget(textView);
    setCentralWidget(stackedWidget);
}

void GameController::setupConnections()
{
    connect(model, &GameModel::gameOver, this, [this](){
        QMessageBox::information(this, "Game Over", "Game Over!");
        stopAutoPlay();
    });

    connect(autoPlayTimer, &QTimer::timeout, this, &GameController::handleAutoPlayStep);

    // Connect from textView command signal
    connect(textView, &TextGameView::commandEntered, this, &GameController::handleTextCommand);

    // Connect from graphicView key signals already done in previous step, no changes needed here.
    connect(graphicView, &GameView::moveRequest, this, [this](int dx, int dy){
        moveProtagonist(dx, dy);
    });
    connect(graphicView, &GameView::autoPlayRequest, this, &GameController::startAutoPlay);
    connect(graphicView, &GameView::tileSelected, this, &GameController::onTileSelected);
}

void GameController::createActions()
{
    switchViewAction = new QAction(tr("&Switch View"), this);
    connect(switchViewAction, &QAction::triggered, this, &GameController::switchView);

    autoPlayAction = new QAction(tr("&Auto Play"), this);
    connect(autoPlayAction, &QAction::triggered, this, &GameController::startAutoPlay);

    saveGameAction = new QAction(tr("&Save Game"), this);
    connect(saveGameAction, &QAction::triggered, this, &GameController::saveGame);

    loadGameAction = new QAction(tr("&Load Game"), this);
    connect(loadGameAction, &QAction::triggered, this, &GameController::loadGame);

    newGameAction = new QAction(tr("&New Game"), this);
    connect(newGameAction, &QAction::triggered, this, &GameController::newGame);

    restartGameAction = new QAction(tr("&Restart Game"), this);
    connect(restartGameAction, &QAction::triggered, this, &GameController::restartGame);
}

void GameController::createMenus()
{
    gameMenu = menuBar()->addMenu(tr("&Game"));
    gameMenu->addAction(newGameAction);
    gameMenu->addAction(restartGameAction);
    gameMenu->addAction(autoPlayAction);
    gameMenu->addAction(saveGameAction);
    gameMenu->addAction(loadGameAction);

    viewMenu = menuBar()->addMenu(tr("&View"));
    viewMenu->addAction(switchViewAction);
}

void GameController::setupCommands()
{
    // Add commands
    commandParser.addCommand("up", [this](QStringList){ moveUp(); });
    commandParser.addCommand("down", [this](QStringList){ moveDown(); });
    commandParser.addCommand("left", [this](QStringList){ moveLeft(); });
    commandParser.addCommand("right", [this](QStringList){ moveRight(); });
    commandParser.addCommand("goto", [this](QStringList args){
        if (args.size() == 2) {
            int x = args[0].toInt();
            int y = args[1].toInt();
            gotoXY(x,y);
        }
    });
    commandParser.addCommand("attack nearest enemy", [this](QStringList){ attackNearestEnemy(); });
    commandParser.addCommand("take nearest health pack", [this](QStringList){ takeNearestHealthPack(); });
    commandParser.addCommand("help", [this](QStringList){ printHelp(); });
}

void GameController::handleTextCommand(QString command)
{
    if (!commandParser.parseCommand(command)) {
        textView->appendMessage("Unknown or ambiguous command. Type 'help' for a list.");
    }
}

void GameController::printHelp()
{
    textView->appendMessage(commandParser.helpText());
}

void GameController::moveUp() { moveProtagonist(0, -1); }
void GameController::moveDown() { moveProtagonist(0, 1); }
void GameController::moveLeft() { moveProtagonist(-1, 0); }
void GameController::moveRight() { moveProtagonist(1, 0); }

void GameController::gotoXY(int x, int y)
{
    // set a path to that tile using A*
    // and follow it in autopilot mode
    stopAutoPlay();
    resetNodes();
    auto *p = model->getProtagonist();
    int cols = model->getCols();

    if (x<0||x>=model->getCols()||y<0||y>=model->getRows()) {
        textView->appendMessage("Coordinates out of range.");
        return;
    }

    Node &startNode = nodes[p->getYPos()*cols + p->getXPos()];
    Node &endNode = nodes[y*cols + x];

    auto costFunc = [this](const Node &a, const Node &b) {
        for (auto &e : model->getEnemies()) {
            if (!e->isDefeated() && e->getXPos()==b.getXPos() && e->getYPos()==b.getYPos()) {
                return std::numeric_limits<float>::infinity();
            }
        }
        float energyCost = 1.0f/(b.getValue()+1.0f)*0.1f;
        return energyCost;
    };

    auto heuristicFunc = [](const Node &a, const Node &b) {
        return std::sqrt(std::pow(a.getXPos()-b.getXPos(),2)+std::pow(a.getYPos()-b.getYPos(),2));
    };

    PathFinder<Node, Node> pathfinder(nodes, &startNode, &endNode, nodeComparator, (unsigned int)cols, costFunc, heuristicFunc, 1.0f);
    autoPath = pathfinder.A_star();
    autoPathIndex = 0;
    if (autoPath.empty()) {
        textView->appendMessage("No path found.");
    } else {
        startAutoPlay();
    }
}

void GameController::attackNearestEnemy()
{
    EnemyWrapper* e = findNextTargetEnemy();
    if (e) {
        // Attack logic: just move protagonist onto enemy tile if possible
        // If on same tile, we have attacked. If it's XEnemy, call hit()
        auto *p = model->getProtagonist();
        if (p->getXPos()==e->getXPos() && p->getYPos()==e->getYPos()) {
            // Already on enemy tile, count as hit
            if (auto xE = dynamic_cast<XEnemyWrapper*>(e)) {
                xE->hit();
                if (xE->isDefeated())
                    textView->appendMessage("XEnemy defeated (again)!");
                else
                    textView->appendMessage("XEnemy resurrected stronger!");
            } else {
                e->setDefeated(true);
                textView->appendMessage("Enemy defeated!");
            }
            emit model->modelUpdated();
        } else {
            // Try pathfinding to enemy tile
            planPathToEnemy(e);
            if (!autoPath.empty()) {
                startAutoPlay();
                textView->appendMessage("Moving towards enemy...");
            } else {
                textView->appendMessage("No path to enemy.");
            }
        }
    } else {
        textView->appendMessage("No enemies found.");
    }
}

void GameController::takeNearestHealthPack()
{
    // Just plan path to nearest healthpack
    planPathToHealthPack();
    if (!autoPath.empty()) {
        startAutoPlay();
        textView->appendMessage("Moving towards health pack...");
    } else {
        textView->appendMessage("No health pack found.");
    }
}

void GameController::switchView()
{
    int index = stackedWidget->currentIndex();
    stackedWidget->setCurrentIndex((index + 1) % 2);
}

void GameController::startAutoPlay()
{
    if (autoPlayActive) {
        // If auto-play is already active, calling startAutoPlay again means stop it.
        stopAutoPlay();
        return;
    }

    auto *p = model->getProtagonist();
    if (p->getHealth() <= 0 || p->getEnergy() <= 0) {
        emit model->gameOver();
        return;
    }

    // If autoPath is empty and we are not doing a one-shot movement,
    // we can decide next action if it's full auto-play mode
    if (!oneShotMovement && autoPath.empty()) {
        decideNextAction();
    }

    autoPlayActive = true;
    autoPlayTimer->start(500);
}

void GameController::stopAutoPlay()
{
    autoPlayTimer->stop();
    currentTarget = TargetType::None;
    autoPlayActive = false;
    oneShotMovement = false; // reset one-shot movement mode
}


void GameController::handleAutoPlayStep()
{
    auto *p = model->getProtagonist();
    if (p->getHealth() <= 0 || p->getEnergy() <= 0) {
        stopAutoPlay();
        emit model->gameOver();
        return;
    }

    if (autoPathIndex < (int)autoPath.size()) {
        int move = autoPath[autoPathIndex++];
        int dx = 0, dy = 0;
        switch(move) {
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

        if (autoPathIndex >= (int)autoPath.size()) {
            // Path ended
            if (oneShotMovement) {
                // If it was a single move to a clicked location,
                // stop right after reaching the destination.
                stopAutoPlay();
            } else {
                // If full auto-play mode, continue deciding next actions
                decideNextAction();
            }
        }
    } else {
        // No path
        if (oneShotMovement) {
            // If we intended to move only once and there's no path,
            // just stop to avoid indefinite attempts
            stopAutoPlay();
        } else {
            decideNextAction();
        }
    }
}

void GameController::saveGame()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Game"), "", tr("Game Files (*.game)"));
    if (!fileName.isEmpty()) {
        if (saveGameToFile(fileName)) {
            QMessageBox::information(this, tr("Save Game"), tr("Game saved successfully."));
        } else {
            QMessageBox::warning(this, tr("Save Game"), tr("Failed to save the game."));
        }
    }
}

void GameController::loadGame()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Load Game"), "", tr("Game Files (*.game)"));
    if (!fileName.isEmpty()) {
        if (loadGameFromFile(fileName)) {
            QMessageBox::information(this, tr("Load Game"), tr("Game loaded successfully."));
        } else {
            QMessageBox::warning(this, tr("Load Game"), tr("Failed to load the game."));
        }
    }
}

void GameController::newGame()
{
    stopAutoPlay();
    int lvl = model->getCurrentLevel();
    if (levelCache.contains(lvl)) {
        loadLevelFromCache(lvl);
        emit model->modelReset();
        return;
    }

    World w;
    try {
        w.createWorld(model->getLevelFiles()[model->getCurrentLevel()], 20, 25);
    } catch (...) {
        qWarning() << "Failed to create world";
        return;
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

    // Convert some enemies to XEnemies
    convertRandomEnemiesToXEnemies(enemyWrappers, cols, rows);

    auto hpVec = w.getHealthPacks();
    std::vector<std::unique_ptr<HealthPack>> hpWrappers;
    hpWrappers.reserve(hpVec.size());
    for (auto &hp : hpVec) {
        hpWrappers.push_back(std::make_unique<HealthPack>(std::move(hp)));
    }

    std::vector<std::unique_ptr<Portal>> portalWrappers;
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distX(0, cols - 1);
        std::uniform_int_distribution<> distY(0, rows - 1);
        bool portalPlaced = false;
        while (!portalPlaced) {
            int x = distX(gen);
            int y = distY(gen);
            TileWrapper* tile = nullptr;
            for (auto &tw : tileWrappers) {
                if (tw->getXPos() == x && tw->getYPos() == y) {
                    tile = tw.get();
                    break;
                }
            }
            if (tile && tile->getValue() != std::numeric_limits<float>::infinity() && !(x == 0 && y == 0)) {
                auto pTile = std::make_unique<Tile>(x, y, 0.0f);
                portalWrappers.push_back(std::make_unique<Portal>(std::move(pTile)));
                portalPlaced = true;
            }
        }
    }

    model->setTiles(std::move(tileWrappers), rows, cols);
    model->setProtagonist(std::move(protagonist));
    model->setEnemies(std::move(enemyWrappers));
    model->setHealthPacks(std::move(hpWrappers));
    model->setPortals(std::move(portalWrappers));

    cacheCurrentLevel(lvl);

    emit model->modelReset();
}

void GameController::restartGame()
{
    stopAutoPlay();
    int lvl = model->getCurrentLevel();
    model->setCurrentLevel(lvl);
    newGame();
}

void GameController::moveProtagonist(int dx, int dy)
{
    auto *p = model->getProtagonist();
    int newX = p->getXPos() + dx;
    int newY = p->getYPos() + dy;

    int rows = model->getRows();
    int cols = model->getCols();
    const auto &tiles = model->getTiles();

    if (newX >= 0 && newX < cols && newY >= 0 && newY < rows) {
        TileWrapper *targetTile = nullptr;
        for (auto &tw : tiles) {
            if (tw->getXPos() == newX && tw->getYPos() == newY) {
                targetTile = tw.get();
                break;
            }
        }

        if (targetTile && targetTile->getValue() != std::numeric_limits<float>::infinity()) {
            float energyCost = 1.0f / (targetTile->getValue() + 1.0f) * 0.1f;
            float newEnergy = p->getEnergy() - energyCost;
            if (newEnergy >= 0) {
                p->setEnergy(newEnergy);
                p->setPos(newX, newY);
                checkForHealthPacks();
                checkForEncounters();
                checkForPortal();
                emit model->modelUpdated();

                if (p->getHealth() <= 0 || p->getEnergy() <= 0) {
                    emit model->gameOver();
                    stopAutoPlay();
                }
            } else {
                emit model->gameOver();
                stopAutoPlay();
            }
        }
    }
}

void GameController::checkForEncounters()
{
    auto *p = model->getProtagonist();
    for (auto &e : model->getEnemies()) {
        if (!e->isDefeated() && e->getXPos() == p->getXPos() && e->getYPos() == p->getYPos()) {
            float healthCost = e->getStrength();
            float newHealth = p->getHealth() - healthCost;
            if (newHealth > 0) {
                p->setHealth(newHealth);
                // Check if XEnemy
                if (auto xE = dynamic_cast<XEnemyWrapper*>(e.get())) {
                    xE->hit();
                } else {
                    e->setDefeated(true);
                }

                if (auto pE = dynamic_cast<PEnemy*>(e->getRaw())) {
                    pE->poison();
                    handlePEnemyPoison(pE);
                }

                emit model->modelUpdated();
            } else {
                p->setHealth(0);
                emit model->gameOver();
                stopAutoPlay();
            }
            break;
        }
    }
}

void GameController::checkForHealthPacks()
{
    auto *p = model->getProtagonist();
    auto &hpList = const_cast<std::vector<std::unique_ptr<HealthPack>>&>(model->getHealthPacks());
    for (auto it = hpList.begin(); it != hpList.end(); ++it) {
        if ((*it)->getXPos() == p->getXPos() && (*it)->getYPos() == p->getYPos()) {
            float newHealth = p->getHealth() + (*it)->getHealAmount();
            if (newHealth > 100.0f) newHealth = 100.0f;
            p->setHealth(newHealth);
            hpList.erase(it);
            emit model->modelUpdated();
            break;
        }
    }
}

void GameController::checkForPortal()
{
    auto *p = model->getProtagonist();
    auto &ports = const_cast<std::vector<std::unique_ptr<Portal>>&>(model->getPortals());
    for (auto &portal : ports) {
        if (portal->getXPos() == p->getXPos() && portal->getYPos() == p->getYPos()) {
            int lvl = model->getCurrentLevel();
            lvl++;
            if (lvl < model->getLevelFiles().size()) {
                stopAutoPlay();
                model->setCurrentLevel(lvl);
                newGame();
                emit model->modelUpdated();
            } else {
                emit model->gameOver();
            }
            break;
        }
    }
}

void GameController::handlePEnemyPoison(PEnemy *pEnemy)
{
    auto *prot = model->getProtagonist();
    int radius = (int)(pEnemy->getPoisonLevel() / 10);
    int centerX = pEnemy->getXPos();
    int centerY = pEnemy->getYPos();

    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            int x = centerX + dx;
            int y = centerY + dy;
            if (x == prot->getXPos() && y == prot->getYPos()) {
                float newHealth = prot->getHealth() - 5.0f;
                prot->setHealth(newHealth);
                if (newHealth <= 0) {
                    emit model->gameOver();
                    stopAutoPlay();
                }
            }
        }
    }
}

EnemyWrapper* GameController::findNextTargetEnemy()
{
    EnemyWrapper *targetEnemy = nullptr;
    float minDistance = std::numeric_limits<float>::max();
    auto *p = model->getProtagonist();

    for (auto &e : model->getEnemies()) {
        if (!e->isDefeated()) {
            float dist = std::sqrt(std::pow(e->getXPos() - p->getXPos(), 2) +
                                   std::pow(e->getYPos() - p->getYPos(), 2));
            if (dist < minDistance) {
                minDistance = dist;
                targetEnemy = e.get();
            }
        }
    }

    return targetEnemy;
}

void GameController::decideNextAction()
{
    auto *p = model->getProtagonist();
    EnemyWrapper* targetEnemy = findNextTargetEnemy();

    if (targetEnemy) {
        float enemyDamage = targetEnemy->getStrength();
        if (p->getHealth() <= enemyDamage) {
            planPathToHealthPack();
            currentTarget = TargetType::HealthPack;
        } else if (p->getHealth() < 70.0f) {
            planPathToEnemyWithHealthPacks(targetEnemy);
            currentTarget = TargetType::Enemy;
        } else {
            planPathToEnemy(targetEnemy);
            currentTarget = TargetType::Enemy;
        }
        autoPathIndex = 0;
    } else {
        planPathToPortal();
        currentTarget = TargetType::Portal;
        autoPathIndex = 0;
    }
}

void GameController::resetNodes()
{
    nodes.clear();
    const auto &tiles = model->getTiles();
    nodes.reserve(tiles.size());

    for (auto &tw : tiles) {
        Node n(tw->getXPos(), tw->getYPos(), tw->getValue());
        nodes.push_back(n);
    }
    for (auto &node : nodes) {
        node.f = node.g = node.h = 0;
        node.visited = node.closed = false;
        node.prev = nullptr;
    }
}

// Path planning methods (unchanged logic)

void GameController::planPathToEnemy(EnemyWrapper* targetEnemy)
{
    resetNodes();
    auto *p = model->getProtagonist();
    int cols = model->getCols();
    Node &startNode = nodes[p->getYPos()*cols + p->getXPos()];
    Node &endNode = nodes[targetEnemy->getYPos()*cols + targetEnemy->getXPos()];

    auto costFunc = [this, targetEnemy](const Node &a, const Node &b) {
        for (auto &e : model->getEnemies()) {
            if (!e->isDefeated() && e.get() != targetEnemy &&
                e->getXPos()==b.getXPos() && e->getYPos()==b.getYPos()) {
                return std::numeric_limits<float>::infinity();
            }
        }
        float energyCost = 1.0f / (b.getValue() + 1.0f)*0.1f;
        return energyCost;
    };
    auto heuristicFunc = [](const Node &a, const Node &b) {
        return std::sqrt((a.getXPos()-b.getXPos())*(a.getXPos()-b.getXPos()) + (a.getYPos()-b.getYPos())*(a.getYPos()-b.getYPos()));
    };

    PathFinder<Node, Node> pathfinder(nodes, &startNode, &endNode, nodeComparator, (unsigned int)cols, costFunc, heuristicFunc, 1.0f);
    autoPath = pathfinder.A_star();
    autoPathIndex = 0;
}

void GameController::planPathToHealthPack()
{
    resetNodes();
    auto *p = model->getProtagonist();
    HealthPack *nearestHP = nullptr;
    float minDist = std::numeric_limits<float>::max();
    for (auto &hp : model->getHealthPacks()) {
        float dist = std::sqrt((hp->getXPos()-p->getXPos())*(hp->getXPos()-p->getXPos()) + (hp->getYPos()-p->getYPos())*(hp->getYPos()-p->getYPos()));
        if (dist < minDist) {
            minDist = dist;
            nearestHP = hp.get();
        }
    }
    if (!nearestHP) {
        autoPath.clear();
        autoPathIndex = 0;
        return;
    }

    int cols = model->getCols();
    Node &startNode = nodes[p->getYPos()*cols + p->getXPos()];
    Node &endNode = nodes[nearestHP->getYPos()*cols + nearestHP->getXPos()];

    auto costFunc = [this](const Node &a, const Node &b) {
        for (auto &e : model->getEnemies()) {
            if(!e->isDefeated() && e->getXPos()==b.getXPos() && e->getYPos()==b.getYPos()) {
                return std::numeric_limits<float>::infinity();
            }
        }
        float energyCost = 1.0f/(b.getValue()+1.0f)*0.1f;
        return energyCost;
    };
    auto heuristicFunc = [](const Node &a, const Node &b) {
        return std::sqrt((a.getXPos()-b.getXPos())*(a.getXPos()-b.getXPos()) + (a.getYPos()-b.getYPos())*(a.getYPos()-b.getYPos()));
    };

    PathFinder<Node, Node> pathfinder(nodes, &startNode, &endNode, nodeComparator,(unsigned int)cols,costFunc,heuristicFunc,1.0f);
    autoPath = pathfinder.A_star();
    autoPathIndex = 0;
}

void GameController::planPathToPortal()
{
    resetNodes();
    if (model->getPortals().empty()) {
        autoPath.clear();
        autoPathIndex = 0;
        return;
    }

    auto *p = model->getProtagonist();
    Portal *portal = model->getPortals().front().get();

    int cols = model->getCols();
    Node &startNode = nodes[p->getYPos()*cols + p->getXPos()];
    Node &endNode = nodes[portal->getYPos()*cols + portal->getXPos()];

    auto costFunc = [this](const Node &a, const Node &b) {
        for (auto &e : model->getEnemies()) {
            if (!e->isDefeated() && e->getXPos()==b.getXPos() && e->getYPos()==b.getYPos()) {
                return std::numeric_limits<float>::infinity();
            }
        }
        float energyCost = 1.0f/(b.getValue()+1.0f)*0.1f;
        return energyCost;
    };
    auto heuristicFunc = [](const Node &a, const Node &b) {
        return std::sqrt((a.getXPos()-b.getXPos())*(a.getXPos()-b.getXPos()) + (a.getYPos()-b.getYPos())*(a.getYPos()-b.getYPos()));
    };

    PathFinder<Node, Node> pathfinder(nodes, &startNode, &endNode,nodeComparator,(unsigned int)cols,costFunc,heuristicFunc,1.0f);
    autoPath = pathfinder.A_star();
    autoPathIndex = 0;
}

void GameController::planPathToEnemyWithHealthPacks(EnemyWrapper* targetEnemy)
{
    resetNodes();
    auto *p = model->getProtagonist();
    int cols = model->getCols();
    Node &startNode = nodes[p->getYPos()*cols + p->getXPos()];
    Node &endNode = nodes[targetEnemy->getYPos()*cols + targetEnemy->getXPos()];

    auto costFunc = [this, targetEnemy](const Node &a, const Node &b) {
        for (auto &e : model->getEnemies()) {
            if (!e->isDefeated() && e.get() != targetEnemy &&
                e->getXPos()==b.getXPos() && e->getYPos()==b.getYPos()) {
                return std::numeric_limits<float>::infinity();
            }
        }

        float energyCost = 1.0f/(b.getValue()+1.0f)*0.1f;
        for (auto &hp : model->getHealthPacks()) {
            if (hp->getXPos()==b.getXPos() && hp->getYPos()==b.getYPos()) {
                energyCost *= 0.5f;
                break;
            }
        }
        return energyCost;
    };
    auto heuristicFunc = [](const Node &a, const Node &b) {
        return std::sqrt((a.getXPos()-b.getXPos())*(a.getXPos()-b.getXPos())+(a.getYPos()-b.getYPos())*(a.getYPos()-b.getYPos()));
    };

    PathFinder<Node, Node> pathfinder(nodes,&startNode,&endNode,nodeComparator,(unsigned int)cols,costFunc,heuristicFunc,1.0f);
    autoPath = pathfinder.A_star();
    autoPathIndex = 0;
}

bool GameController::saveGameToFile(const QString &fileName)
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
        } else if (xE) {
            out << " X" << (xE->hasResurrected()?1:0);
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
        out << pt->getXPos() << " " << pt->getYPos() << "\n";
    }

    return true;
}

bool GameController::loadGameFromFile(const QString &fileName)
{
    stopAutoPlay();
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
            if (tokens[4]=="X0"||tokens[4]=="X1") {
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
            if (resurrectedX && defeated) {
                // simulate that it was resurrected once
                // We have no direct setter for resurrected in code provided,
                // but we can re-implement logic if needed. For simplicity, let's just accept this state.
            }
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

    line = in.readLine();
    if (!line.startsWith("Portals ")) return false;
    int numPortals = line.mid(8).toInt();
    std::vector<std::unique_ptr<Portal>> ports;
    ports.reserve(numPortals);
    for (int i=0; i<numPortals; i++) {
        line = in.readLine();
        tokens = line.split(" ");
        if (tokens.size()!=2) return false;
        int ptx = tokens[0].toInt();
        int pty = tokens[1].toInt();
        auto pTile = std::make_unique<Tile>(ptx, pty, 0.0f);
        ports.push_back(std::make_unique<Portal>(std::move(pTile)));
    }

    model->setTiles(std::move(tileWrappers), rows, cols);
    model->setProtagonist(std::move(protagonist));
    model->setEnemies(std::move(enemies));
    model->setHealthPacks(std::move(hps));
    model->setPortals(std::move(ports));

    cacheCurrentLevel(lvl);

    emit model->modelReset();
    emit model->modelUpdated();
    return true;
}

void GameController::loadLevelFromCache(int level)
{
    auto it = levelCache.find(level);
    if (it == levelCache.end()) {
        // not cached
        newGame();
        return;
    }

    auto cached = it.value(); // std::shared_ptr<CachedLevel>
    model->setTiles(cloneTiles(cached->tiles), cached->rows, cached->cols);
    {
        auto origP = cached->protagonist->getRaw();
        auto newProtag = std::make_unique<Protagonist>();
        newProtag->setPos(origP->getXPos(), origP->getYPos());
        newProtag->setHealth(origP->getHealth());
        newProtag->setEnergy(origP->getEnergy());
        model->setProtagonist(std::make_unique<ProtagonistWrapper>(std::move(newProtag)));
    }
    model->setEnemies(cloneEnemies(cached->enemies));
    model->setHealthPacks(cloneHealthPacks(cached->healthPacks));
    model->setPortals(clonePortals(cached->portals));
}

void GameController::cacheCurrentLevel(int level)
{
    auto c = std::make_shared<CachedLevel>();
    c->rows = model->getRows();
    c->cols = model->getCols();
    c->tiles = cloneTiles(model->getTiles());
    {
        auto origP = model->getProtagonist()->getRaw();
        auto newProtag = std::make_unique<Protagonist>();
        newProtag->setPos(origP->getXPos(), origP->getYPos());
        newProtag->setHealth(origP->getHealth());
        newProtag->setEnergy(origP->getEnergy());
        c->protagonist = std::make_unique<ProtagonistWrapper>(std::move(newProtag));
    }
    c->enemies = cloneEnemies(model->getEnemies());
    c->healthPacks = cloneHealthPacks(model->getHealthPacks());
    c->portals = clonePortals(model->getPortals());

    levelCache[level] = c;
}

std::vector<std::unique_ptr<TileWrapper>> GameController::cloneTiles(const std::vector<std::unique_ptr<TileWrapper>> &source)
{
    std::vector<std::unique_ptr<TileWrapper>> result;
    result.reserve(source.size());
    for (auto &t : source) {
        auto tilePtr = std::make_unique<Tile>(t->getXPos(), t->getYPos(), t->getValue());
        result.push_back(std::make_unique<TileWrapper>(std::move(tilePtr)));
    }
    return result;
}

std::vector<std::unique_ptr<EnemyWrapper>> GameController::cloneEnemies(const std::vector<std::unique_ptr<EnemyWrapper>> &source)
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
            // We must reconstruct from base Enemy
            Enemy *base = e->getRaw();
            auto Ec = std::make_unique<Enemy>(base->getXPos(), base->getYPos(), base->getValue());
            Ec->setDefeated(base->getDefeated());
            // Recreate XEnemyWrapper
            auto xEc = std::make_unique<XEnemyWrapper>(std::move(Ec), model->getCols(), model->getRows());
            // We can't set resurrected easily but that's fine for demo.
            result.push_back(std::move(xEc));
        } else {
            auto Ec = std::make_unique<Enemy>(e->getXPos(), e->getYPos(), e->getStrength());
            Ec->setDefeated(e->isDefeated());
            result.push_back(std::make_unique<EnemyWrapper>(std::move(Ec)));
        }
    }
    return result;
}

std::vector<std::unique_ptr<HealthPack>> GameController::cloneHealthPacks(const std::vector<std::unique_ptr<HealthPack>> &source)
{
    std::vector<std::unique_ptr<HealthPack>> result;
    result.reserve(source.size());
    for (auto &hp : source) {
        auto hc = std::make_unique<Tile>(hp->getXPos(), hp->getYPos(), hp->getValue());
        result.push_back(std::make_unique<HealthPack>(std::move(hc)));
    }
    return result;
}

std::vector<std::unique_ptr<Portal>> GameController::clonePortals(const std::vector<std::unique_ptr<Portal>> &source)
{
    std::vector<std::unique_ptr<Portal>> result;
    result.reserve(source.size());
    for (auto &pt : source) {
        auto pc = std::make_unique<Tile>(pt->getXPos(), pt->getYPos(), pt->getValue());
        result.push_back(std::make_unique<Portal>(std::move(pc)));
    }
    return result;
}

void GameController::onTileSelected(int x, int y)
{
    stopAutoPlay(); // stop any previous action
    oneShotMovement = true; // Indicate this is a single move to a point


    // Validate target
    if (x < 0 || x >= model->getCols() || y < 0 || y >= model->getRows()) {
        qDebug() << "Invalid tile selected!";
        return;
    }

    // Plan a path to the clicked tile using A* (similar to gotoXY logic)
    resetNodes();
    auto *p = model->getProtagonist();
    int cols = model->getCols();
    Node &startNode = nodes[p->getYPos()*cols + p->getXPos()];
    Node &endNode = nodes[y*cols + x];

    auto costFunc = [this](const Node &a, const Node &b) {
        // Check for enemies blocking the path
        for (auto &e : model->getEnemies()) {
            if (!e->isDefeated() &&
                e->getXPos() == b.getXPos() && e->getYPos() == b.getYPos()) {
                return std::numeric_limits<float>::infinity();
            }
        }
        float energyCost = 1.0f/(b.getValue()+1.0f)*0.1f;
        return energyCost;
    };

    auto heuristicFunc = [](const Node &a, const Node &b) {
        return std::sqrt(std::pow(a.getXPos()-b.getXPos(),2) +
                         std::pow(a.getYPos()-b.getYPos(),2));
    };

    PathFinder<Node, Node> pathfinder(nodes, &startNode, &endNode, nodeComparator,
                                      (unsigned int)cols, costFunc, heuristicFunc, 1.0f);
    autoPath = pathfinder.A_star();
    autoPathIndex = 0;

    if (autoPath.empty()) {
        qDebug() << "No path found to the selected tile.";
    } else {
        // Start auto-play along this path
        startAutoPlay();
    }
}


void GameController::convertRandomEnemiesToXEnemies(std::vector<std::unique_ptr<EnemyWrapper>> &enemies, int cols, int rows)
{
    if (enemies.empty()) return;
    size_t count = enemies.size()/4;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0,(int)enemies.size()-1);
    for (size_t i=0;i<count;i++){
        int idx = dist(gen);
        // Check if PEnemyWrapper or XEnemyWrapper already
        // Now we can dynamic_cast because polymorphic:
        // EnemyWrapper has virtual destructor => polymorphic
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


