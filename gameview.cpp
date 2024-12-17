#include "gameview.h"
#include <QKeyEvent>
#include <QGraphicsRectItem>
#include <QGraphicsPixmapItem>
#include <QVariantAnimation>
#include <QWheelEvent>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QDebug>
#include <QVBoxLayout>
#include <limits>
#include <QLabel>
#include <QMouseEvent>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QDebug>

// Here we add tile images and overlay image.
// Top overlay image (e.g. :/images/overlay.png)

GameView::GameView(GameModel *model, QWidget *parent)
    : QWidget(parent), model(model), protagonistItem(nullptr), overlayItem(nullptr)
{
    scene = new QGraphicsScene(this);
    graphicsView = new QGraphicsView(scene, this);

    setFocusPolicy(Qt::StrongFocus);
    graphicsView->setFocusPolicy(Qt::NoFocus);

    graphicsView->setMouseTracking(true);
    graphicsView->installEventFilter(this);

    statusTextEdit = new QTextEdit(this);
    statusTextEdit->setReadOnly(true);
    statusTextEdit->setFixedWidth(200);

    healthBar = new QProgressBar(this);
    healthBar->setRange(0,100);
    energyBar = new QProgressBar(this);
    energyBar->setRange(0,100);

    setupScene();
    setupLayout();

    connect(model, &GameModel::modelUpdated, this, &GameView::updateView);
    connect(model, &GameModel::gameOver, this, &GameView::handleGameOver);
    connect(model, &GameModel::modelReset, this, &GameView::handleModelReset);
}

GameView::~GameView()
{
    delete scene;
}

void GameView::handleModelReset()
{
    scene->clear();
    tileItems.clear();
    enemyItems.clear();
    healthPackItems.clear();
    portalItems.clear();
    protagonistItem = nullptr;
    overlayItem = nullptr;
    setupScene();
    updateStatus();
}

void GameView::setupLayout()
{
    QVBoxLayout *vLay = new QVBoxLayout;
    vLay->addWidget(graphicsView, 5);
    vLay->addWidget(new QLabel("Health:", this));
    vLay->addWidget(healthBar);
    vLay->addWidget(new QLabel("Energy:", this));
    vLay->addWidget(energyBar);
    vLay->addWidget(statusTextEdit, 2);

    setLayout(vLay);
}

void GameView::setupScene()
{
    drawTiles();
    drawEntities();

    // Add overlay image
    QPixmap overlayMap(":/images/overlay.png");
    if (!overlayMap.isNull()) {
        overlayItem = scene->addPixmap(overlayMap.scaled(scene->sceneRect().size().toSize()));
        overlayItem->setZValue(10); // top layer
    }
}

void GameView::drawTiles()
{
    int rows = model->getRows();
    int cols = model->getCols();
    const auto &tiles = model->getTiles();
    for (auto &tile : tiles) {
        QRectF rect(tile->getXPos()*32, tile->getYPos()*32,32,32);

        // Instead of colored rect, load tile image. If no image, fallback to color
        // For simplicity, use a single tile image
        QGraphicsRectItem *item = scene->addRect(rect);

        if (tile->getValue() == std::numeric_limits<float>::infinity()) {
            item->setBrush(Qt::black);
        } else {
            // Just use a gray color scale or a grass image
            // QPixmap tileImg(":/images/grass.png");
            // if (!tileImg.isNull()) {
            //   QBrush brush(tileImg.scaled(32,32));
            //   item->setBrush(brush);
            // } else {
            int colorValue = static_cast<int>(tile->getValue()*255);
            item->setBrush(QColor(colorValue,colorValue,colorValue));
            //}
        }
        tileItems.insert(tile.get(), item);
    }

    scene->setSceneRect(0,0, cols*32, rows*32);
}

void GameView::drawEntities()
{
    ProtagonistWrapper *protagonist = model->getProtagonist();
    protagonistItem = scene->addPixmap(QPixmap(":/images/protagonist.png").scaled(32,32));
    protagonistItem->setPos(protagonist->getXPos()*32, protagonist->getYPos()*32);

    for (auto &enemy : model->getEnemies()) {
        QString imagePath = ":/images/enemy.png";
        if (dynamic_cast<PEnemy*>(enemy->getRaw())) {
            imagePath = ":/images/penemy.png";
        } else if (dynamic_cast<XEnemyWrapper*>(enemy.get())) {
            imagePath = ":/images/xenemy.png";
        }
        QPixmap img(imagePath);
        if (enemy->isDefeated()) {
            img = QPixmap(":/images/enemy_defeated.png").scaled(32,32);
        } else {
            img = img.scaled(32,32);
        }

        QGraphicsPixmapItem *item = scene->addPixmap(img);
        item->setPos(enemy->getXPos()*32, enemy->getYPos()*32);
        enemyItems.insert(enemy.get(), item);
    }

    for (auto &hp : model->getHealthPacks()) {
        QGraphicsPixmapItem *item = scene->addPixmap(QPixmap(":/images/healthpack.png").scaled(32,32));
        item->setPos(hp->getXPos()*32, hp->getYPos()*32);
        healthPackItems.insert(hp.get(), item);
    }

    for (auto &portal : model->getPortals()) {
        QGraphicsPixmapItem *item = scene->addPixmap(QPixmap(":/images/portal.png").scaled(32,32));
        item->setPos(portal->getXPos()*32, portal->getYPos()*32);
        portalItems.insert(portal.get(), item);
    }
}

void GameView::updateView()
{
    ProtagonistWrapper *protagonist = model->getProtagonist();
    if (protagonistItem) {
        protagonistItem->setPos(protagonist->getXPos()*32, protagonist->getYPos()*32);
    }

    // Update enemy positions:
    for (auto &enemy : model->getEnemies()) {
        QGraphicsPixmapItem *item = enemyItems.value(enemy.get(), nullptr);
        if (item) {
            // Reposition enemy based on current coords
            item->setPos(enemy->getXPos()*32, enemy->getYPos()*32);

            if (enemy->isDefeated()) {
                item->setPixmap(QPixmap(":/images/enemy_defeated.png").scaled(32,32));
            } else {
                if (dynamic_cast<XEnemyWrapper*>(enemy.get())) {
                    // Optionally set a different image:
                    // item->setPixmap(QPixmap(":/images/xenemy.png").scaled(32,32));
                }
            }
        }
    }

    // Health packs may be removed
    for (auto it = healthPackItems.begin(); it != healthPackItems.end();) {
        bool stillExists = false;
        for (auto &hp : model->getHealthPacks()) {
            if (hp.get()==it.key()) {
                stillExists=true;
                break;
            }
        }
        if (!stillExists) {
            scene->removeItem(it.value());
            delete it.value();
            it = healthPackItems.erase(it);
        } else {
            ++it;
        }
    }

    updateStatus();
}

void GameView::updateStatus()
{
    ProtagonistWrapper *p = model->getProtagonist();
    healthBar->setValue((int)p->getHealth());
    energyBar->setValue((int)p->getEnergy());

    QString statusText = QString("Health: %1\nEnergy: %2\nLevel: %3")
                             .arg(p->getHealth())
                             .arg(p->getEnergy())
                             .arg(model->getCurrentLevel()+1);

    // Find nearby entities
    int radius = 3;
    int pX = p->getXPos();
    int pY = p->getYPos();
    QList<QString> nearbyEntities;
    for (auto &enemy : model->getEnemies()) {
        if (!enemy->isDefeated()) {
            int distX = qAbs(enemy->getXPos()-pX);
            int distY = qAbs(enemy->getYPos()-pY);
            if (distX<=radius && distY<=radius) {
                QString enemyType = "Enemy";
                if (dynamic_cast<PEnemy*>(enemy->getRaw())) enemyType="Poisonous Enemy";
                if (dynamic_cast<XEnemyWrapper*>(enemy.get())) enemyType="XEnemy";
                nearbyEntities.append(QString("%1 at (%2,%3)").arg(enemyType).arg(enemy->getXPos()).arg(enemy->getYPos()));
            }
        }
    }

    for (auto &hp : model->getHealthPacks()) {
        int distX = qAbs(hp->getXPos()-pX);
        int distY = qAbs(hp->getYPos()-pY);
        if (distX<=radius && distY<=radius) {
            nearbyEntities.append(QString("Health Pack at (%1,%2)").arg(hp->getXPos()).arg(hp->getYPos()));
        }
    }

    for (auto &portal : model->getPortals()) {
        int distX = qAbs(portal->getXPos()-pX);
        int distY = qAbs(portal->getYPos()-pY);
        if (distX<=radius && distY<=radius) {
            nearbyEntities.append(QString("Portal at (%1,%2)").arg(portal->getXPos()).arg(portal->getYPos()));
        }
    }

    if (!nearbyEntities.isEmpty()) {
        statusText.append("\n\nNearby Entities:");
        for (const QString &info : nearbyEntities)
            statusText.append("\n- "+info);
    }

    statusTextEdit->setPlainText(statusText);
}

void GameView::animateProtagonist(const QString &)
{
    // simple scale animation
    if (!protagonistItem) return;
    QVariantAnimation *animation = new QVariantAnimation(this);
    animation->setDuration(100);
    animation->setStartValue(1.0);
    animation->setKeyValueAt(0.5,1.2);
    animation->setEndValue(1.0);
    connect(animation, &QVariantAnimation::valueChanged, [this](const QVariant &val){
        if (protagonistItem) protagonistItem->setScale(val.toReal());
    });
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void GameView::handleGameOver()
{
    qDebug()<<"Game Over triggered in view";
}

void GameView::keyPressEvent(QKeyEvent *event)
{
    graphicsViewKeyPressEvent(event);
}

bool GameView::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == graphicsView) {
        if (event->type()==QEvent::Wheel) {
            graphicsViewWheelEvent(static_cast<QWheelEvent*>(event));
            return true;
        } else if (event->type()==QEvent::KeyPress) {
            graphicsViewKeyPressEvent(static_cast<QKeyEvent*>(event));
            return true;
        }
    }
    return QWidget::eventFilter(obj,event);
}

void GameView::graphicsViewKeyPressEvent(QKeyEvent *event)
{
    int dx=0,dy=0;
    switch (event->key()) {
    case Qt::Key_Left:
    case Qt::Key_A: dx=-1; break;
    case Qt::Key_Right:
    case Qt::Key_D: dx=1; break;
    case Qt::Key_Up:
    case Qt::Key_W: dy=-1; break;
    case Qt::Key_Down:
    case Qt::Key_S: dy=1; break;
    case Qt::Key_Space: emit autoPlayRequest(); return;
    default: QWidget::keyPressEvent(event); return;
    }
    emit moveRequest(dx,dy);
}

void GameView::graphicsViewWheelEvent(QWheelEvent *event)
{
    qreal x = event->position().x();
    qreal y = event->position().y();
    QPointF before = graphicsView->mapToScene(QPoint((int)x,(int)y));
    double scaleFactor = 1.15;
    if (event->angleDelta().y()>0) {
        graphicsView->scale(scaleFactor, scaleFactor);
    } else {
        graphicsView->scale(1.0/scaleFactor,1.0/scaleFactor);
    }
    QPointF after = graphicsView->mapToScene(QPoint((int)x,(int)y));
    QPointF offset = before - after;
    graphicsView->horizontalScrollBar()->setValue(graphicsView->horizontalScrollBar()->value()+offset.x());
    graphicsView->verticalScrollBar()->setValue(graphicsView->verticalScrollBar()->value()+offset.y());
    event->accept();
}

void GameView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QPointF scenePos = graphicsView->mapToScene(event->pos());
        int tileX = static_cast<int>(scenePos.x() / 32);
        int tileY = static_cast<int>(scenePos.y() / 32);

        if (tileX >= 0 && tileX < model->getCols() && tileY >= 0 && tileY < model->getRows()) {
            emit tileSelected(tileX, tileY);
        }
    }

    QWidget::mousePressEvent(event);
}
