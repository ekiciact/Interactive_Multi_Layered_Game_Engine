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
    commandMoveTimer(new QTimer(this)),
    commandPathIndex(0)
{
    std::function<bool(const Node&, const Node&)> nodeComparator = [](const Node &a, const Node &b) {
        return a.f > b.f;
    };

    autoPlayStrategy = std::make_unique<DefaultAutoPlayStrategy>(nodeComparator);

    setupModel();
    setupViews();
    setupConnections();
    createActions();
    createMenus();
    setupCommands();

    // commandMoveTimer setup
    commandMoveTimer->setInterval(300); // Slightly faster or similar speed as autoplay
    connect(commandMoveTimer, &QTimer::timeout, this, &GameController::handleCommandMoveStep);
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
    newGame(); // calls gameStateManager internally
}

void GameController::setupViews()
{
    graphicView = new GameView(model, this);
    textView = new TextGameView(model, this);

    stackedWidget = new QStackedWidget(this);
    stackedWidget->addWidget(graphicView);
    stackedWidget->addWidget(textView);
    setCentralWidget(stackedWidget);

    graphicView->setUniversalOverlayImage(":/images/level_overlay.png");
}

void GameController::setupConnections()
{
    connect(model, &GameModel::gameOver, this, [this](){
        QMessageBox::information(this, "Game Over", "Game Over!");
        stopAutoPlay();
        commandMoveTimer->stop();
    });

    connect(autoPlayTimer, &QTimer::timeout, this, &GameController::handleAutoPlayStep);

    connect(textView, &TextGameView::commandEntered, this, &GameController::handleTextCommand);

    connect(graphicView, &GameView::moveRequest, this, [this](int dx, int dy){
        // Manual move using arrow keys (or UI buttons)
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

    toggleOverlayAction = new QAction(tr("&Toggle Overlay"), this);
    connect(toggleOverlayAction, &QAction::triggered, this, &GameController::toggleOverlay);
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
    viewMenu->addAction(toggleOverlayAction); // Add it to the menu
}

void GameController::setupCommands()
{
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

    commandParser.addCommand("attack nearest enemy", [this](QStringList){
        attackNearestEnemy();
    });

    commandParser.addCommand("take nearest health pack", [this](QStringList){
        takeNearestHealthPack();
    });

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
    stopAutoPlay();
    commandMoveTimer->stop();
    // Direct path movement to (x,y) with animation
    std::vector<int> path = computeDirectPath(model->getProtagonist()->getXPos(),
                                              model->getProtagonist()->getYPos(),
                                              x,y);
    if (!path.empty()) {
        startCommandPathMovement(path);
    } else {
        textView->appendMessage("No path found to the specified tile.");
    }
}

void GameController::attackNearestEnemy()
{
    stopAutoPlay();
    commandMoveTimer->stop();

    EnemyWrapper* e = findNearestUndefeatedEnemy();
    if (!e) {
        textView->appendMessage("No enemies found.");
        return;
    }

    auto *p = model->getProtagonist();
    std::vector<int> path = computeDirectPath(p->getXPos(), p->getYPos(),
                                              e->getXPos(), e->getYPos(),
                                              true /*avoid portal if enemies remain*/);
    if (path.empty()) {
        textView->appendMessage("No path found to the nearest enemy.");
        return;
    }

    textView->appendMessage("Moving towards enemy...");
    startCommandPathMovement(path);
}

void GameController::takeNearestHealthPack()
{
    stopAutoPlay();
    commandMoveTimer->stop();

    HealthPack* hp = findNearestHealthPack();
    if (!hp) {
        textView->appendMessage("No health packs found.");
        return;
    }

    auto *p = model->getProtagonist();
    std::vector<int> path = computeDirectPath(p->getXPos(), p->getYPos(),
                                              hp->getXPos(), hp->getYPos(),
                                              true /*avoid portal if enemies remain*/);
    if (path.empty()) {
        textView->appendMessage("No path found to the nearest health pack.");
        return;
    }

    textView->appendMessage("Moving towards health pack...");
    startCommandPathMovement(path);
}

void GameController::switchView()
{
    int index = stackedWidget->currentIndex();
    stackedWidget->setCurrentIndex((index + 1) % 2);
}

void GameController::startAutoPlay()
{
    // Stop any manual command movements
    commandMoveTimer->stop();

    if (autoPlayActive) {
        stopAutoPlay();
        return;
    }

    auto *p = model->getProtagonist();
    if (p->getHealth() <= 0 || p->getEnergy() <= 0) {
        emit model->gameOver();
        return;
    }

    autoPlayStrategy->start(model);
    autoPlayStrategy->decideNextAction();
    autoPlayActive = true;
    autoPlayTimer->start(500);
}

void GameController::stopAutoPlay()
{
    autoPlayTimer->stop();
    autoPlayStrategy->stop();
    autoPlayActive = false;
    oneShotMovement = false;
}

void GameController::handleAutoPlayStep()
{
    auto *p = model->getProtagonist();
    if (p->getHealth() <= 0 || p->getEnergy() <= 0) {
        stopAutoPlay();
        emit model->gameOver();
        return;
    }

    AutoPlayMove move = autoPlayStrategy->nextStep();

    if (move.dx == 0 && move.dy == 0) {
        // No steps in current path, decide next action
        autoPlayStrategy->decideNextAction();
        move = autoPlayStrategy->nextStep();
        if (move.dx == 0 && move.dy == 0) {
            // Still no steps: stop autoplay
            stopAutoPlay();
            return;
        }
    }

    // Perform the move
    moveProtagonist(move.dx, move.dy);

    // Check if protagonist died after move
    if (p->getHealth() <= 0 || p->getEnergy() <= 0) {
        stopAutoPlay();
        emit model->gameOver();
    }
}

void GameController::saveGame()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Game"), "", tr("Game Files (*.game)"));
    if (!fileName.isEmpty()) {
        if (gameStateManager.saveGameToFile(model, fileName)) {
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
        if (gameStateManager.loadGameFromFile(model, levelCache, fileName)) {
            QMessageBox::information(this, tr("Load Game"), tr("Game loaded successfully."));
        } else {
            QMessageBox::warning(this, tr("Load Game"), tr("Failed to load the game."));
        }
    }
}

void GameController::newGame()
{
    stopAutoPlay();
    commandMoveTimer->stop();
    if (!gameStateManager.newGame(model, levelCache)) {
        qWarning() << "Failed to start new game.";
    }
}

void GameController::restartGame()
{
    stopAutoPlay();
    commandMoveTimer->stop();
    if (!gameStateManager.restartGame(model, levelCache)) {
        qWarning() << "Failed to restart game.";
    }
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
                    commandMoveTimer->stop();
                }
            } else {
                emit model->gameOver();
                stopAutoPlay();
                commandMoveTimer->stop();
            }
        }
    }
}

void GameController::checkForEncounters()
{
    auto *p = model->getProtagonist();
    for (auto &e : model->getEnemies()) {
        if (!e->isDefeated() && e->getXPos() == p->getXPos() && e->getYPos() == p->getYPos()) {
            if (auto xE = dynamic_cast<XEnemyWrapper*>(e.get())) {
                // XEnemy logic:
                if (xE->getTimesHit() == 0) {
                    xE->hit();
                    emit model->modelUpdated();
                } else if (xE->getTimesHit() == 1) {
                    float healthCost = e->getStrength();
                    float newHealth = p->getHealth() - healthCost;
                    if (newHealth > 0) {
                        p->setHealth(newHealth);
                        xE->hit(); // second hit defeats XEnemy
                        emit model->modelUpdated();
                    } else {
                        p->setHealth(0);
                        emit model->gameOver();
                        stopAutoPlay();
                        commandMoveTimer->stop();
                    }
                }
            } else {
                // Normal or PEnemy logic:
                float healthCost = e->getStrength();
                float newHealth = p->getHealth() - healthCost;
                if (newHealth > 0) {
                    p->setHealth(newHealth);
                    e->setDefeated(true);
                    if (auto pE = dynamic_cast<PEnemy*>(e->getRaw())) {
                        pE->poison();
                        handlePEnemyPoison(pE);
                    }
                    emit model->modelUpdated();
                } else {
                    p->setHealth(0);
                    emit model->gameOver();
                    stopAutoPlay();
                    commandMoveTimer->stop();
                }
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
    bool anyEnemyAlive = false;
    for (auto &e : model->getEnemies()) {
        if (!e->isDefeated()) {
            anyEnemyAlive = true;
            break;
        }
    }

    auto &ports = const_cast<std::vector<std::unique_ptr<Portal>>&>(model->getPortals());
    for (auto &portal : ports) {
        if (portal->getXPos() == p->getXPos() && portal->getYPos() == p->getYPos()) {
            if (!anyEnemyAlive) {
                int lvl = model->getCurrentLevel();
                lvl++;
                if (lvl < (int)model->getLevelFiles().size()) {
                    stopAutoPlay();
                    commandMoveTimer->stop();
                    model->setCurrentLevel(lvl);
                    gameStateManager.newGame(model, levelCache);
                    emit model->modelUpdated();
                } else {
                    emit model->gameOver();
                }
            } else {
                // If enemies remain, do nothing, protagonist stays on portal tile
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
                    commandMoveTimer->stop();
                }
            }
        }
    }
}

void GameController::onTileSelected(int x, int y)
{
    stopAutoPlay();
    commandMoveTimer->stop();
    moveProtagonistDirectlyToTile(x,y);
}

void GameController::moveProtagonistDirectlyToTile(int x, int y)
{
    auto *p = model->getProtagonist();
    std::vector<int> path = computeDirectPath(p->getXPos(), p->getYPos(), x, y, true);
    if (!path.empty()) {
        startCommandPathMovement(path);
    } else {
        qDebug() << "No path found to the selected tile.";
    }
}

void GameController::toggleOverlay()
{
    if (!graphicView) return;
    bool visible = graphicView->isOverlayVisible();
    graphicView->setOverlayVisible(!visible);
}

std::vector<int> GameController::computeDirectPath(int startX, int startY, int endX, int endY, bool avoidPortalIfEnemies)
{
    std::vector<Node> nodes;
    const auto &tiles = model->getTiles();
    nodes.reserve(tiles.size());
    int cols = model->getCols();
    int rows = model->getRows();

    bool enemiesAlive = false;
    for (auto &e : model->getEnemies()) {
        if (!e->isDefeated()) {
            enemiesAlive = true;
            break;
        }
    }

    for (auto &tw : tiles) {
        Node n(tw->getXPos(), tw->getYPos(), tw->getValue());
        n.f = n.g = n.h = 0.0f;
        n.visited = n.closed = false;
        n.prev = nullptr;
        nodes.push_back(n);
    }

    if (endX < 0 || endX >= cols || endY < 0 || endY >= rows) {
        return {};
    }

    Node &startNode = nodes[startY*cols + startX];
    Node &endNode   = nodes[endY*cols + endX];

    auto costFunc = [this, enemiesAlive, avoidPortalIfEnemies](const Node &a, const Node &b) {
        for (auto &e : model->getEnemies()) {
            if (!e->isDefeated() && e->getXPos() == b.getXPos() && e->getYPos() == b.getYPos()) {
                return std::numeric_limits<float>::infinity();
            }
        }
        if (enemiesAlive && avoidPortalIfEnemies) {
            for (auto &port : model->getPortals()) {
                if (port->getXPos()==b.getXPos() && port->getYPos()==b.getYPos()) {
                    return std::numeric_limits<float>::infinity();
                }
            }
        }

        float val = b.getValue();
        return 1.0f/(val+1.0f)*0.1f;
    };

    auto heuristicFunc = [](const Node &a, const Node &b) {
        return std::sqrt((a.getXPos()-b.getXPos())*(a.getXPos()-b.getXPos())
                         + (a.getYPos()-b.getYPos())*(a.getYPos()-b.getYPos()));
    };

    std::function<bool(const Node&, const Node&)> nodeComparator = [](const Node &A, const Node &B){
        return A.f > B.f;
    };

    PathFinder<Node, Node> pathfinder(nodes, &startNode, &endNode, nodeComparator, (unsigned int)cols, costFunc, heuristicFunc, 1.0f);
    return pathfinder.A_star();
}

// New method to start the animated command-based movement
void GameController::startCommandPathMovement(const std::vector<int> &path)
{
    commandPath = path;
    commandPathIndex = 0;
    commandMoveTimer->start();
}

void GameController::handleCommandMoveStep()
{
    if (commandPathIndex >= (int)commandPath.size()) {
        // Done with movement
        commandMoveTimer->stop();
        return;
    }

    int move = commandPath[commandPathIndex++];
    int dx=0,dy=0;
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

    moveProtagonist(dx,dy);

    auto *p = model->getProtagonist();
    if (p->getHealth() <= 0 || p->getEnergy() <= 0) {
        // Movement ended due to death
        commandMoveTimer->stop();
    }
}

EnemyWrapper* GameController::findNearestUndefeatedEnemy()
{
    EnemyWrapper* targetEnemy = nullptr;
    float minDist = std::numeric_limits<float>::max();
    auto *p = model->getProtagonist();
    for (auto &e : model->getEnemies()) {
        if (!e->isDefeated()) {
            float dist = std::sqrt((e->getXPos() - p->getXPos())*(e->getXPos()-p->getXPos())
                                   + (e->getYPos() - p->getYPos())*(e->getYPos()-p->getYPos()));
            if (dist < minDist) {
                minDist = dist;
                targetEnemy = e.get();
            }
        }
    }
    return targetEnemy;
}

HealthPack* GameController::findNearestHealthPack()
{
    HealthPack* nearestHP = nullptr;
    float minDist = std::numeric_limits<float>::max();
    auto *p = model->getProtagonist();
    for (auto &hp : model->getHealthPacks()) {
        float dist = std::sqrt((hp->getXPos()-p->getXPos())*(hp->getXPos()-p->getXPos())
                               + (hp->getYPos()-p->getYPos())*(hp->getYPos()-p->getYPos()));
        if (dist < minDist) {
            minDist = dist;
            nearestHP = hp.get();
        }
    }
    return nearestHP;
}
