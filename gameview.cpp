#include "gameview.h"
#include <QKeyEvent>
#include <QGraphicsRectItem>
#include <QGraphicsPixmapItem>
#include <QVariantAnimation>
#include <QWheelEvent>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QTextEdit>

GameView::GameView(GameModel *model, QWidget *parent)
    : QWidget(parent), model(model)
{
    scene = new QGraphicsScene(this);
    graphicsView = new QGraphicsView(scene, this);

    // Set focus policy to accept keyboard input
    graphicsView->setFocusPolicy(Qt::StrongFocus);
    graphicsView->setMouseTracking(true);

    // Install event filter
    graphicsView->installEventFilter(this);

    statusTextEdit = new QTextEdit(this);
    statusTextEdit->setReadOnly(true);
    statusTextEdit->setFixedWidth(200); // Adjust as needed

    setupScene();
    setupLayout();

    connect(model, &GameModel::modelUpdated, this, &GameView::updateView);
    connect(model, &GameModel::gameOver, this, &GameView::handleGameOver);
    connect(model, &GameModel::modelReset, this, &GameView::handleModelReset); // Connect to modelReset signal
}

GameView::~GameView()
{
    delete scene;
}

void GameView::handleModelReset()
{
    // Clear existing scene and data structures
    scene->clear();
    tileItems.clear();
    enemyItems.clear();
    healthPackItems.clear();
    portalItems.clear();
    protagonistItem = nullptr;

    // Rebuild the scene
    setupScene();
    updateStatus();
}

void GameView::setupLayout()
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(graphicsView, 3); // 3/4 of the space
    mainLayout->addWidget(statusTextEdit, 1); // 1/4 of the space
    setLayout(mainLayout);
}

void GameView::setupScene()
{
    drawTiles();
    drawEntities();
}

void GameView::drawTiles()
{
    int rows = model->getWorld()->getRows();
    int cols = model->getWorld()->getCols();
    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            Tile *tile = model->getTileAt(x, y);
            QRectF rect(x * 32, y * 32, 32, 32);
            QGraphicsRectItem *item = scene->addRect(rect);

            if (tile->getValue() == std::numeric_limits<float>::infinity()) {
                item->setBrush(Qt::black);
            } else {
                int colorValue = static_cast<int>(tile->getValue() * 255);
                item->setBrush(QColor(colorValue, colorValue, colorValue));
            }
            tileItems.insert(tile, item);
        }
    }

    // Set the scene rectangle
    scene->setSceneRect(0, 0, cols * 32, rows * 32);
}

void GameView::drawEntities()
{
    // Draw protagonist
    Protagonist *protagonist = model->getProtagonist();
    protagonistItem = scene->addPixmap(QPixmap(":/images/protagonist.png")
                                           .scaled(32, 32));
    protagonistItem->setPos(protagonist->getXPos() * 32,
                            protagonist->getYPos() * 32);

    // Draw enemies
    for (Enemy *enemy : model->getEnemies()) {
        QString imagePath = ":/images/enemy.png";
        if (dynamic_cast<PEnemy*>(enemy)) {
            imagePath = ":/images/penemy.png";
        }
        QGraphicsPixmapItem *item = scene->addPixmap(QPixmap(imagePath)
                                                         .scaled(32, 32));
        item->setPos(enemy->getXPos() * 32, enemy->getYPos() * 32);
        enemyItems.insert(enemy, item);
    }

    // Draw health packs
    for (Tile *hp : model->getHealthPacks()) {
        QGraphicsPixmapItem *item = scene->addPixmap(QPixmap(":/images/healthpack.png")
                                                         .scaled(32, 32));
        item->setPos(hp->getXPos() * 32, hp->getYPos() * 32);
        healthPackItems.insert(hp, item);
    }

    // Draw portals
    for (Tile *portal : model->getPortals()) {
        QGraphicsPixmapItem *item = scene->addPixmap(QPixmap(":/images/portal.png")
                                                         .scaled(32, 32));
        item->setPos(portal->getXPos() * 32, portal->getYPos() * 32);
        portalItems.insert(portal, item);
    }
}

void GameView::updateView()
{
    // Update protagonist position
    Protagonist *protagonist = model->getProtagonist();
    protagonistItem->setPos(protagonist->getXPos() * 32,
                            protagonist->getYPos() * 32);

    // Animate protagonist action
    animateProtagonist("move");

    // Update enemies
    for (Enemy *enemy : model->getEnemies()) {
        QGraphicsPixmapItem *item = enemyItems.value(enemy);
        if (enemy->getDefeated()) {
            item->setPixmap(QPixmap(":/images/enemy_defeated.png").scaled(32, 32));
        }
        if (PEnemy *pEnemy = dynamic_cast<PEnemy*>(enemy)) {
            // Animate poison effect
            // For example, change opacity based on poison level
            int alpha = static_cast<int>((pEnemy->getPoisonLevel() / 100.0f) * 255);
            item->setOpacity(alpha / 255.0);
        }
    }

    // Update health packs
    for (Tile *hp : healthPackItems.keys()) {
        if (!model->getHealthPacks().contains(hp)) {
            QGraphicsPixmapItem *item = healthPackItems.take(hp);
            scene->removeItem(item);
            delete item;
        }
    }

    // Update portals (if any changes)
    // For this example, portals remain static

    // Update status display
    updateStatus();
}

void GameView::updateStatus()
{
    Protagonist *protagonist = model->getProtagonist();
    QString statusText = QString("Health: %1\nEnergy: %2\nLevel: %3")
                             .arg(protagonist->getHealth())
                             .arg(protagonist->getEnergy())
                             .arg(model->getCurrentLevel() + 1); // Changed to use getter method

    // Find nearby entities within a certain radius
    int radius = 3; // For example, 3 tiles, it can be set more but more calculations
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

void GameView::animateProtagonist(const QString &action)
{
    Q_UNUSED(action);

    QVariantAnimation *animation = new QVariantAnimation(this);
    animation->setDuration(100);
    animation->setStartValue(1.0);
    animation->setKeyValueAt(0.5, 1.2);
    animation->setEndValue(1.0);

    connect(animation, &QVariantAnimation::valueChanged, [this](const QVariant &value){
        protagonistItem->setScale(value.toReal());
    });

    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void GameView::handleGameOver()
{
    qDebug() << "Game Over!";
    // Show game over message
}

void GameView::keyPressEvent(QKeyEvent *event)
{
    // Forward the key press event to the graphicsViewKeyPressEvent
    graphicsViewKeyPressEvent(event);
}

bool GameView::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == graphicsView) {
        if (event->type() == QEvent::Wheel) {
            graphicsViewWheelEvent(static_cast<QWheelEvent *>(event));
            return true; // Event handled
        }
        else if (event->type() == QEvent::KeyPress) {
            graphicsViewKeyPressEvent(static_cast<QKeyEvent *>(event));
            return true; // Event handled
        }
    }
    // Pass the event on to the parent class
    return QWidget::eventFilter(obj, event);
}

void GameView::graphicsViewKeyPressEvent(QKeyEvent *event)
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

void GameView::graphicsViewWheelEvent(QWheelEvent *event)
{
    // Get the mouse position in view coordinates
    qreal x = event->position().x();
    qreal y = event->position().y();

    // Map the position before scaling
    QPointF pointBeforeScale = graphicsView->mapToScene(x, y);

    // Scale the view
    const double scaleFactor = 1.15;
    if (event->angleDelta().y() > 0) { // Zoom in
        graphicsView->scale(scaleFactor, scaleFactor);
    } else { // Zoom out
        graphicsView->scale(1.0 / scaleFactor, 1.0 / scaleFactor);
    }

    // Map the position after scaling
    QPointF pointAfterScale = graphicsView->mapToScene(x, y);

    // Adjust the scrollbars to keep the mouse position stationary
    QPointF offset = pointBeforeScale - pointAfterScale;
    graphicsView->horizontalScrollBar()->setValue(graphicsView->horizontalScrollBar()->value() + offset.x());
    graphicsView->verticalScrollBar()->setValue(graphicsView->verticalScrollBar()->value() + offset.y());

    // Accept the event
    event->accept();
}
