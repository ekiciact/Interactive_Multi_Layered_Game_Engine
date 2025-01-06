#include "textgameview.h"
#include <QKeyEvent>
#include <QScrollBar>
#include <QDebug>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <limits>

TextGameView::TextGameView(GameModel *model, QWidget *parent)
    : QWidget(parent), model(model), colorIndex(0)
{
    textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);
    textEdit->setFontFamily("Courier");
    textEdit->setFontPointSize(10);
    textEdit->setLineWrapMode(QTextEdit::NoWrap);

    statusTextEdit = new QTextEdit(this);
    statusTextEdit->setReadOnly(true);
    statusTextEdit->setFixedWidth(200);

    helpTextEdit = new QTextEdit(this);
    helpTextEdit->setReadOnly(true);
    helpTextEdit->setFixedWidth(200);

    commandLine = new QLineEdit(this);
    connect(commandLine, &QLineEdit::returnPressed, this, &TextGameView::onCommandReturnPressed);

    setupLayout();

    connect(model, &GameModel::modelUpdated, this, &TextGameView::updateView);
    connect(model, &GameModel::gameOver, this, &TextGameView::handleGameOver);
    connect(model, &GameModel::modelReset, this, &TextGameView::handleModelReset);

    // Define a cycle of colors for the protagonist (like RGB LED cycling):
    // Add more colors or change them.
    colorCycle << "rgb(255,255,0)"
               << "rgb(255,0,255)"
               << "rgb(0,255,255)"
               << "rgb(128,0,128)"
               << "rgb(255,165,0)"
               << "rgb(128,128,0)"
               << "rgb(0,128,128)"
               << "rgb(255,192,203)"
               << "rgb(139,69,19)"
               << "rgb(173,216,230)"
               << "rgb(240,230,140)"
               << "rgb(255,222,173)";


    // Setup a timer to cycle protagonist color periodically
    colorTimer = new QTimer(this);
    connect(colorTimer, &QTimer::timeout, this, &TextGameView::cycleProtagonistColor);
    colorTimer->start(250); // Change color every 0.25 second

    renderTextWorld();
    updateStatus();
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
}

TextGameView::~TextGameView()
{}

void TextGameView::setupLayout()
{
    // Create a horizontal layout for the main game view and right panel
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(textEdit, 3);

    // Create a vertical layout for the right panel (status and help)
    QVBoxLayout *rightPanelLayout = new QVBoxLayout;

    // Add status and help to right panel with equal heights
    rightPanelLayout->addWidget(statusTextEdit);
    rightPanelLayout->addWidget(helpTextEdit);

    // Add right panel to main layout
    mainLayout->addLayout(rightPanelLayout, 1);

    // Create the final vertical layout with command line at bottom
    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->addLayout(mainLayout);
    vLayout->addWidget(commandLine);

    setLayout(vLayout);
}

void TextGameView::appendMessage(const QString &msg)
{
    textEdit->append(msg);
    textEdit->verticalScrollBar()->setValue(textEdit->verticalScrollBar()->maximum());
}

void TextGameView::appendHelpMessage(const QString &msg)
{
    helpTextEdit->clear(); // Clear previous help text
    helpTextEdit->setPlainText(msg);
}

void TextGameView::handleModelReset()
{
    renderTextWorld();
    updateStatus();
}

void TextGameView::updateView()
{
    renderTextWorld();
    updateStatus();
}

void TextGameView::handleGameOver()
{
    appendMessage("\nGame Over!");
}

void TextGameView::renderTextWorld()
{
    int rows = model->getRows();
    int cols = model->getCols();
    QString htmlText;
    auto *p = model->getProtagonist();

    // Get current protagonist color
    QString currentColor = colorCycle[colorIndex % colorCycle.size()];

    htmlText.append("<pre>");

    for (int y=0; y<rows; ++y) {
        QString line;
        for (int x=0; x<cols; ++x) {
            QString styledChar;
            QChar ch='.';
            TileWrapper *tile = nullptr;
            for (auto &tw:model->getTiles()) {
                if (tw->getXPos()==x && tw->getYPos()==y) {
                    tile=tw.get();
                    break;
                }
            }

            // Check for infinite tile
            if (tile && tile->getValue()==std::numeric_limits<float>::infinity()) {
                ch='#';
            }

            // Check if protagonist is here
            if (p->getXPos()==x && p->getYPos()==y) {
                // Protagonist: use the cycling color
                styledChar = QString("<span style='font-weight:bold; color:%1;'>P</span>")
                                 .arg(currentColor);
            } else {
                bool entityFound = false;

                // Check for enemies
                // If enemy is alive:
                //   XEnemy = 'X', PEnemy = 'P', normal Enemy = 'E'
                // If enemy is defeated:
                //   'D' (red)
                bool enemyPlaced = false;
                for (auto &e:model->getEnemies()) {
                    if (e->getXPos()==x && e->getYPos()==y) {
                        if (!e->isDefeated()) {
                            if (dynamic_cast<PEnemyWrapper*>(e.get())) {
                                styledChar = "P"; // PEnemy (capital P)
                            } else if (dynamic_cast<XEnemyWrapper*>(e.get())) {
                                styledChar = "X"; // XEnemy
                            } else {
                                styledChar = "E"; // Normal enemy
                            }
                            enemyPlaced = true;
                        } else {
                            // Defeated enemy
                            styledChar = "<span style='color:red;'>D</span>";
                            enemyPlaced = true;
                        }
                        entityFound = true;
                        break;
                    }
                }

                // If no enemy found or placed, check health packs
                if (!entityFound) {
                    for (auto &hp:model->getHealthPacks()) {
                        if (hp->getXPos()==x && hp->getYPos()==y) {
                            // Health Pack: 'H' green color
                            styledChar = "<span style='color:green;'>H</span>";
                            entityFound = true;
                            break;
                        }
                    }
                }

                // Check portals if still nothing found
                if (!entityFound) {
                    for (auto &port:model->getPortals()) {
                        if (port->getXPos()==x && port->getYPos()==y) {
                            // Portal: 'O' blue color
                            styledChar = "<span style='color:blue;'>O</span>";
                            entityFound = true;
                            break;
                        }
                    }
                }

                // If still no entity found, use default
                if (!entityFound) {
                    if (ch == '#') {
                        styledChar = "#";
                    } else {
                        styledChar = ".";
                    }
                }
            }

            line.append(styledChar);
        }
        htmlText.append(line + "\n");
    }
    htmlText.append("</pre>");

    textEdit->setHtml(htmlText);
    textEdit->verticalScrollBar()->setValue(textEdit->verticalScrollBar()->minimum());
}

void TextGameView::updateStatus()
{
    auto *p = model->getProtagonist();
    QString statusText = QString("Health: %1\nEnergy: %2\nLevel: %3")
                             .arg(p->getHealth())
                             .arg(p->getEnergy())
                             .arg(model->getCurrentLevel()+1);

    int radius=3;
    int pX=p->getXPos();int pY=p->getYPos();
    QList<QString> nearby;
    for (auto &e:model->getEnemies()) {
        if(!e->isDefeated()) {
            int distX=qAbs(e->getXPos()-pX);
            int distY=qAbs(e->getYPos()-pY);
            if (distX<=radius&&distY<=radius) {
                QString et="Enemy";
                if (dynamic_cast<PEnemyWrapper*>(e.get())) et="Poisonous Enemy";
                if (dynamic_cast<XEnemyWrapper*>(e.get())) et="XEnemy";
                nearby.append(QString("%1 at (%2,%3)").arg(et).arg(e->getXPos()).arg(e->getYPos()));
            }
        }
    }
    for (auto &hp:model->getHealthPacks()) {
        int distX=qAbs(hp->getXPos()-pX);
        int distY=qAbs(hp->getYPos()-pY);
        if(distX<=radius&&distY<=radius)
            nearby.append(QString("Health Pack at (%1,%2)").arg(hp->getXPos()).arg(hp->getYPos()));
    }
    for (auto &port:model->getPortals()) {
        int distX=qAbs(port->getXPos()-pX);
        int distY=qAbs(port->getYPos()-pY);
        if(distX<=radius&&distY<=radius)
            nearby.append(QString("Portal at (%1,%2)").arg(port->getXPos()).arg(port->getYPos()));
    }

    if(!nearby.isEmpty()) {
        statusText.append("\n\nNearby Entities:");
        for(auto &n:nearby) statusText.append("\n- "+n);
    }

    statusTextEdit->setPlainText(statusText);
}

void TextGameView::onCommandReturnPressed()
{
    QString cmd = commandLine->text();
    commandLine->clear();
    if (cmd.isEmpty()) return;
    emit commandEntered(cmd);
}

void TextGameView::cycleProtagonistColor()
{
    // Cycle to the next color
    colorIndex = (colorIndex + 1) % colorCycle.size();
    // Re-render the view to show the new protagonist color
    updateView();
}
