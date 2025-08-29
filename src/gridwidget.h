#ifndef GRIDWIDGET_H
#define GRIDWIDGET_H

#include "defs.h"
#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QPainter>
#include <QPixmap>
#include <QMouseEvent>
#include <QMessageBox>
#include <vector>
#include <random>

// Constants
const int GRID_COLS = 48;
const int GRID_ROWS = 24;
const int POINT_RADIUS = 3;
const int LINE_WIDTH = 2;
const int MARGIN = 20;
const QColor GRID_COLOR = Qt::blue;
const QColor SELECT_COLOR = Qt::red;
const QColor LINE_COLOR = Qt::darkGreen;
const QColor BG_COLOR = Qt::white;

// Line structure
struct Line {
    QPoint p1;
    QPoint p2;
    QLineF line;
    int health;
};

struct Bee {
    QPoint position;
    int direction;
    bool moving;
    bool stunned;
    int stunnedTime;
    int health;
    int maxHealth;
    bool touchingLine;
};

class DraggableCounter : public QLabel {
public:
    using QLabel::QLabel;
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *) override;
private:
    QPoint m_dragPosition;
};

class GridWidget : public QWidget {
    Q_OBJECT
public:
    explicit GridWidget(datastorage &gameData, QWidget *parent = nullptr);
    
protected:
    void updateCounter();
    void initializeGameObjects();
    void updateBees();
    void paintEvent(QPaintEvent *e) override;
    void mousePressEvent(QMouseEvent *e) override;
    
private:
    QPixmap dogImage;
    QPixmap beeImage;
    QPoint dogPos;
    std::vector<QPoint> beePositions;
    std::vector<int> beeDirections;
    std::mt19937 rng;
    datastorage &m_gameData;
    DraggableCounter *counter;
    DraggableCounter *countdownCounter;
    std::vector<QPoint> selectedPoints;
    std::vector<Line> drawnLines;
    int SPACING;
    int beeSpawnCountdown;

    QPoint getGridPoint(const QPoint &mouse);
    void startCountdown();
    void spawnBees();
    std::vector<Bee> bees;
    int totalBeesToSpawn;
    int currentSpawnWave;
    int beesSpawnedThisWave;
    
    void startBeeWaves();
    void spawnBeeWave();
    void spawnSingleBee();
    void updateLineHealth();
    void checkLineCollisions(size_t beeIndex);
    void freeBeesFromLine(const Line &destroyedLine);
    
    // Win condition variables
    int survivalTimer;
    bool gameActive;
    void checkWinConditions();
    void playerWins(int xpReward);
};

#endif // GRIDWIDGET_H