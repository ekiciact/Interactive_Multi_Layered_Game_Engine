#include "gamecontroller.h"
#include <QAction>
#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>

GameController::GameController(QWidget *parent)
    : QMainWindow(parent)
{
    setupModel();
    setupViews();
    setupConnections();
    createActions();
    createMenus();
}

GameController::~GameController()
{
    delete model;
}

void GameController::show()
{
    QMainWindow::show();
}

void GameController::setupModel()
{
    model = new GameModel(this);
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
    // Connect any necessary signals and slots
    connect(model, &GameModel::gameOver, this, [](){
        QMessageBox::information(nullptr, "Game Over", "Game Over!");
    });
}

void GameController::createActions()
{
    switchViewAction = new QAction(tr("&Switch View"), this);
    connect(switchViewAction, &QAction::triggered,
            this, &GameController::switchView);

    autoPlayAction = new QAction(tr("&Auto Play"), this);
    connect(autoPlayAction, &QAction::triggered,
            this, &GameController::startAutoPlay);

    saveGameAction = new QAction(tr("&Save Game"), this);
    connect(saveGameAction, &QAction::triggered,
            this, &GameController::saveGame);

    loadGameAction = new QAction(tr("&Load Game"), this);
    connect(loadGameAction, &QAction::triggered,
            this, &GameController::loadGame);

    newGameAction = new QAction(tr("&New Game"), this);
    connect(newGameAction, &QAction::triggered,
            this, &GameController::newGame);

    restartGameAction = new QAction(tr("&Restart Game"), this);
    connect(restartGameAction, &QAction::triggered,
            this, &GameController::restartGame);
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

void GameController::switchView()
{
    int index = stackedWidget->currentIndex();
    stackedWidget->setCurrentIndex((index + 1) % 2);
}

void GameController::startAutoPlay()
{
    model->startAutoPlay();
}

void GameController::saveGame()
{
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    tr("Save Game"), "", tr("Game Files (*.game)"));
    if (!fileName.isEmpty()) {
        if (model->saveGame(fileName)) {
            QMessageBox::information(this, tr("Save Game"),
                                     tr("Game saved successfully."));
        } else {
            QMessageBox::warning(this, tr("Save Game"),
                                 tr("Failed to save the game."));
        }
    }
}

void GameController::loadGame()
{
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Load Game"), "", tr("Game Files (*.game)"));
    if (!fileName.isEmpty()) {
        if (model->loadGame(fileName)) {
            QMessageBox::information(this, tr("Load Game"),
                                     tr("Game loaded successfully."));
        } else {
            QMessageBox::warning(this, tr("Load Game"),
                                 tr("Failed to load the game."));
        }
    }
}

void GameController::newGame()
{
    model->newGame();
}

void GameController::restartGame()
{
    model->restartGame();
}
