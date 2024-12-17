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
    autoPlayTimer(new QTimer(this))
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
}

void GameController::setupConnections()
{
    connect(model, &GameModel::gameOver, this, [this](){
        QMessageBox::information(this, "Game Over", "Game Over!");
        stopAutoPlay();
    });

    connect(autoPlayTimer, &QTimer::timeout, this, &GameController::handleAutoPlayStep);

    connect(textView, &TextGameView::commandEntered, this, &GameController::handleTextCommand);

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
    stopAutoPlay();
    autoPlayStrategy->start(model);
    autoPlayStrategy->planPathToTile(x,y);
    startAutoPlay();
}

void GameController::attackNearestEnemy()
{
    // Stop any ongoing autoplay and clear old paths
    stopAutoPlay();
    autoPlayStrategy->start(model);

    // Compute path to nearest enemy (once)
    bool success = dynamic_cast<DefaultAutoPlayStrategy*>(autoPlayStrategy.get())->computePathToEnemy();
    if (success) {
        oneShotMovement = true;
        startAutoPlay(); // Follow this path once
        textView->appendMessage("Moving towards enemy...");
    } else {
        textView->appendMessage("No enemies found.");
    }
}

void GameController::takeNearestHealthPack()
{
    stopAutoPlay();
    autoPlayStrategy->start(model);

    bool success = dynamic_cast<DefaultAutoPlayStrategy*>(autoPlayStrategy.get())->computePathToHealthPack();
    if (success) {
        oneShotMovement = true;
        startAutoPlay(); // Follow this path once
        textView->appendMessage("Moving towards health pack...");
    } else {
        textView->appendMessage("No health packs found.");
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
        stopAutoPlay();
        return;
    }

    auto *p = model->getProtagonist();
    if (p->getHealth() <= 0 || p->getEnergy() <= 0) {
        emit model->gameOver();
        return;
    }

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
        // No steps left in the current path
        if (oneShotMovement) {
            // If this was a single action (mouse click or one-shot command), just stop
            stopAutoPlay();
            return;
        } else {
            // Full auto-play mode: try the next action
            autoPlayStrategy->decideNextAction();
            move = autoPlayStrategy->nextStep();
            if (move.dx == 0 && move.dy == 0) {
                // Still no steps: stop autoplay
                stopAutoPlay();
                return;
            }
        }
    }

    // Perform the move
    moveProtagonist(move.dx, move.dy);

    // Check after move if protagonist died
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
    if (!gameStateManager.newGame(model, levelCache)) {
        qWarning() << "Failed to start new game.";
    }
}

void GameController::restartGame()
{
    stopAutoPlay();
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
            if (auto xE = dynamic_cast<XEnemyWrapper*>(e.get())) {
                // XEnemy logic:
                if (xE->getTimesHit() == 0) {
                    // First encounter: Teleport, no damage
                    // Just call hit once
                    xE->hit();
                    emit model->modelUpdated();
                } else if (xE->getTimesHit() == 1) {
                    // Second encounter:
                    // damage as normal enemy
                    float healthCost = e->getStrength();
                    float newHealth = p->getHealth() - healthCost;
                    if (newHealth > 0) {
                        // Protagonist survives second encounter damage
                        p->setHealth(newHealth);
                        // Now defeat the XEnemy with a second hit
                        xE->hit(); // This sets it defeated
                        emit model->modelUpdated();
                    } else {
                        // Protagonist dies
                        p->setHealth(0);
                        emit model->gameOver();
                        stopAutoPlay();
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
    auto &ports = const_cast<std::vector<std::unique_ptr<Portal>>&>(model->getPortals());
    for (auto &portal : ports) {
        if (portal->getXPos() == p->getXPos() && portal->getYPos() == p->getYPos()) {
            int lvl = model->getCurrentLevel();
            lvl++;
            if (lvl < model->getLevelFiles().size()) {
                stopAutoPlay();
                model->setCurrentLevel(lvl);
                gameStateManager.newGame(model, levelCache);
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

void GameController::onTileSelected(int x, int y)
{
    // Stop current autoplay and start fresh
    stopAutoPlay();
    autoPlayStrategy->start(model);

    // Compute path to the clicked tile
    bool success = dynamic_cast<DefaultAutoPlayStrategy*>(autoPlayStrategy.get())->computePathToTile(x,y);
    if (success) {
        oneShotMovement = true; // Follow just this path
        startAutoPlay();
    } else {
        qDebug() << "No path found to the selected tile.";
    }
}
