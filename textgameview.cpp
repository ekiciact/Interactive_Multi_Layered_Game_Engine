#include "textgameview.h"
#include <QKeyEvent>
#include <QDebug>
#include <QScrollBar>

TextGameView::TextGameView(GameModel *model, QWidget *parent)
    : QWidget(parent), model(model)
{
    textEdit = new QTextEdit(this);
    textEdit->setReadOnly(true);
    textEdit->setFontFamily("Courier");
    textEdit->setFontPointSize(10);
    textEdit->setLineWrapMode(QTextEdit::NoWrap);

    statusTextEdit = new QTextEdit(this);
    statusTextEdit->setReadOnly(true);
    statusTextEdit->setFixedWidth(200); // Adjust as needed

    setupLayout();

    connect(model, &GameModel::modelUpdated, this, &TextGameView::updateView);
    connect(model, &GameModel::gameOver, this, &TextGameView::handleGameOver);
    connect(model, &GameModel::modelReset, this, &TextGameView::handleModelReset); // Connect to modelReset signal

    renderTextWorld();
    updateStatus();

    // Set focus policy to accept keyboard input
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
}

void TextGameView::handleModelReset()
{
    renderTextWorld();
    updateStatus();
}

TextGameView::~TextGameView()
{
}

void TextGameView::setupLayout()
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(textEdit, 3); // 3/4 of the space
    mainLayout->addWidget(statusTextEdit, 1); // 1/4 of the space
    setLayout(mainLayout);
}

void TextGameView::updateView()
{
    renderTextWorld();
    updateStatus();
}

void TextGameView::handleGameOver()
{
    textEdit->append("\nGame Over!");
    textEdit->verticalScrollBar()->setValue(textEdit->verticalScrollBar()->maximum());
}

void TextGameView::renderTextWorld()
{
    int rows = model->getWorld()->getRows();
    int cols = model->getWorld()->getCols();
    QString text;

    for (int y = 0; y < rows; ++y) {
        QString line;
        for (int x = 0; x < cols; ++x) {
            QChar ch = '.';
            Tile *tile = model->getTileAt(x, y);
            if (tile->getValue() == std::numeric_limits<float>::infinity()) {
                ch = '#'; // Wall
            }

            if (model->getProtagonist()->getXPos() == x &&
                model->getProtagonist()->getYPos() == y) {
                ch = 'P'; // Protagonist
            } else {
                bool entityFound = false;
                for (Enemy *enemy : model->getEnemies()) {
                    if (!enemy->getDefeated() &&
                        enemy->getXPos() == x && enemy->getYPos() == y) {
                        if (dynamic_cast<PEnemy*>(enemy)) {
                            ch = 'X'; // PEnemy
                        } else {
                            ch = 'E'; // Enemy
                        }
                        entityFound = true;
                        break;
                    }
                }
                if (!entityFound) {
                    for (Tile *hp : model->getHealthPacks()) {
                        if (hp->getXPos() == x && hp->getYPos() == y) {
                            ch = 'H'; // Health Pack
                            entityFound = true;
                            break;
                        }
                    }
                }
                if (!entityFound) {
                    for (Tile *portal : model->getPortals()) {
                        if (portal->getXPos() == x && portal->getYPos() == y) {
                            ch = 'O'; // Portal
                            break;
                        }
                    }
                }
            }

            line.append(ch);
        }
        text.append(line + '\n');
    }
    textEdit->setPlainText(text);
    textEdit->verticalScrollBar()->setValue(textEdit->verticalScrollBar()->minimum());
}

void TextGameView::updateStatus()
{
    Protagonist *protagonist = model->getProtagonist();
    QString statusText = QString("Health: %1\nEnergy: %2\nLevel: %3")
                             .arg(protagonist->getHealth())
                             .arg(protagonist->getEnergy())
                             .arg(model->getCurrentLevel() + 1); // Changed to use getter method

    // Find nearby entities within a certain radius
    int radius = 3; // For example, 3 tiles
    int pX = protagonist->getXPos();
    int pY = protagonist->getYPos();
    QList<QString> nearbyEntities;

    for (Enemy *enemy : model->getEnemies()) {
        if (!enemy->getDefeated()) {
            int eX = enemy->getXPos();
            int eY = enemy->getYPos();
            int distX = qAbs(eX - pX);
            int distY = qAbs(eY - pY);
            if (distX <= radius && distY <= radius) {
                QString enemyType = "Enemy";
                if (dynamic_cast<PEnemy*>(enemy)) {
                    enemyType = "Poisonous Enemy";
                }
                nearbyEntities.append(QString("%1 at (%2,%3)")
                                          .arg(enemyType)
                                          .arg(eX)
                                          .arg(eY));
            }
        }
    }

    for (Tile *hp : model->getHealthPacks()) {
        int hX = hp->getXPos();
        int hY = hp->getYPos();
        int distX = qAbs(hX - pX);
        int distY = qAbs(hY - pY);
        if (distX <= radius && distY <= radius) {
            nearbyEntities.append(QString("Health Pack at (%1,%2)")
                                      .arg(hX)
                                      .arg(hY));
        }
    }

    for (Tile *portal : model->getPortals()) {
        int ptX = portal->getXPos();
        int ptY = portal->getYPos();
        int distX = qAbs(ptX - pX);
        int distY = qAbs(ptY - pY);
        if (distX <= radius && distY <= radius) {
            nearbyEntities.append(QString("Portal at (%1,%2)")
                                      .arg(ptX)
                                      .arg(ptY));
        }
    }

    if (!nearbyEntities.isEmpty()) {
        statusText.append("\n\nNearby Entities:");
        for (const QString &entityInfo : nearbyEntities) {
            statusText.append("\n- " + entityInfo);
        }
    }

    statusTextEdit->setPlainText(statusText);
}

void TextGameView::keyPressEvent(QKeyEvent *event)
{
    int dx = 0;
    int dy = 0;
    switch (event->key()) {
    case Qt::Key_Left:
    case Qt::Key_A:
        dx = -1;
        break;
    case Qt::Key_Right:
    case Qt::Key_D:
        dx = 1;
        break;
    case Qt::Key_Up:
    case Qt::Key_W:
        dy = -1;
        break;
    case Qt::Key_Down:
    case Qt::Key_S:
        dy = 1;
        break;
    case Qt::Key_Space:
        model->startAutoPlay();
        return;
    default:
        QWidget::keyPressEvent(event);
        return;
    }

    model->moveProtagonist(dx, dy);
}
