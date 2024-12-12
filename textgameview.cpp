#include "textgameview.h"
#include <QKeyEvent>
#include <QScrollBar>
#include <QDebug>
#include <QHBoxLayout>
#include <QVBoxLayout>

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
    statusTextEdit->setFixedWidth(200);

    commandLine = new QLineEdit(this);
    connect(commandLine, &QLineEdit::returnPressed, this, &TextGameView::onCommandReturnPressed);

    setupLayout(); // now this function call has a definition below.

    connect(model, &GameModel::modelUpdated, this, &TextGameView::updateView);
    connect(model, &GameModel::gameOver, this, &TextGameView::handleGameOver);
    connect(model, &GameModel::modelReset, this, &TextGameView::handleModelReset);

    renderTextWorld();
    updateStatus();
    setFocusPolicy(Qt::StrongFocus);
    setFocus();
}

TextGameView::~TextGameView()
{}

void TextGameView::setupLayout()
{
    // Create a main horizontal layout for text and status side by side
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->addWidget(textEdit, 3);
    mainLayout->addWidget(statusTextEdit, 1);

    // Create a vertical layout that holds the main layout plus the command line at the bottom
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
    QString text;
    auto *p = model->getProtagonist();

    for (int y=0; y<rows; ++y) {
        QString line;
        for (int x=0; x<cols; ++x) {
            QChar ch='.';
            TileWrapper *tile=nullptr;
            for (auto &tw:model->getTiles()) {
                if (tw->getXPos()==x && tw->getYPos()==y) {
                    tile=tw.get();break;
                }
            }
            if (tile && tile->getValue()==std::numeric_limits<float>::infinity()) {
                ch='#';
            }

            if (p->getXPos()==x && p->getYPos()==y) {
                ch='P';
            } else {
                bool entityFound=false;
                for (auto &e:model->getEnemies()) {
                    if(!e->isDefeated() && e->getXPos()==x && e->getYPos()==y) {
                        if (dynamic_cast<PEnemyWrapper*>(e.get())) ch='X'; // poisonous enemy
                        else if (dynamic_cast<XEnemyWrapper*>(e.get())) ch='Z'; // XEnemy
                        else ch='E';
                        entityFound=true;break;
                    }
                }
                if(!entityFound) {
                    for (auto &hp:model->getHealthPacks()) {
                        if (hp->getXPos()==x && hp->getYPos()==y) {
                            ch='H';entityFound=true;break;
                        }
                    }
                }
                if(!entityFound) {
                    for (auto &port:model->getPortals()) {
                        if (port->getXPos()==x && port->getYPos()==y) {
                            ch='O';break;
                        }
                    }
                }
            }

            line.append(ch);
        }
        text.append(line+'\n');
    }
    textEdit->setPlainText(text);
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
