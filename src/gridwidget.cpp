#include "gridwidget.h"
#include <QHBoxLayout>
#include <QtMath>
#include <QApplication>
#include <QTimer>
#include <chrono>
#include <climits>
#include <QMessageBox>

// Implement DraggableCounter methods
void DraggableCounter::mousePressEvent(QMouseEvent *event) {
    m_dragPosition = event->globalPosition().toPoint() - geometry().topLeft();
    setCursor(Qt::ClosedHandCursor);
}

void DraggableCounter::mouseMoveEvent(QMouseEvent *event) {
    move(event->globalPosition().toPoint() - m_dragPosition);
}

void DraggableCounter::mouseReleaseEvent(QMouseEvent *) {
    setCursor(Qt::ArrowCursor);
}

// Implement GridWidget methods
GridWidget::GridWidget(datastorage &gameData, QWidget *parent) 
    : QWidget(parent), m_gameData(gameData), rng(std::random_device{}()) {
    dogImage.load("doghead.png");
    beeImage.load("bee.png");
    
    // Initialize game state
    survivalTimer = 0;
    gameActive = true;
    
    // Initialize countdown counter
    countdownCounter = new DraggableCounter(this);
    countdownCounter->setStyleSheet("QLabel { background: red; color: white; padding: 10px; border: 2px solid darkred; font-size: 16px; font-weight: bold; }");
    countdownCounter->setAlignment(Qt::AlignCenter);
    countdownCounter->move(width()/2 - 100, 10);
    countdownCounter->setFixedSize(300, 60);
    countdownCounter->hide();
    
    initializeGameObjects();
    
    // Main game timer
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &GridWidget::updateBees);
    timer->start(1000);
    
    // Blocks/HP counter
    counter = new DraggableCounter(this);
    counter->setStyleSheet("QLabel { background: white; padding: 5px; border: 1px solid gray; }");
    counter->setAlignment(Qt::AlignCenter);
    counter->move(width() - 150, 10);
    updateCounter();
    
    // Calculate spacing and set window size
    const int INITIAL_WINDOW_SIZE = 1600;
    const float ASPECT_RATIO = 1.0f;
    const int CELL_WIDTH = (INITIAL_WINDOW_SIZE - 2*MARGIN) / (GRID_COLS - 1);
    const int CELL_HEIGHT = (INITIAL_WINDOW_SIZE*ASPECT_RATIO - 2*MARGIN) / (GRID_ROWS - 1);
    SPACING = qMin(CELL_WIDTH, static_cast<int>(CELL_HEIGHT/ASPECT_RATIO));
    setFixedSize(
        MARGIN*2 + (GRID_COLS-1)*SPACING,
        MARGIN*2 + (GRID_ROWS-1)*SPACING
    );
    setWindowTitle("Save The Dogs");
    
    // Start 10-second countdown
    startCountdown();
}

void GridWidget::startCountdown() {
    beeSpawnCountdown = 10;
    totalBeesToSpawn = (rand() % 2) + 4; // 4-5 waves
    currentSpawnWave = 0;
    beesSpawnedThisWave = 0;
    
    countdownCounter->setText(QString("Countdown: %1 Seconds").arg(beeSpawnCountdown));
    countdownCounter->show();
    
    QTimer *countdownTimer = new QTimer(this);
    connect(countdownTimer, &QTimer::timeout, this, [this, countdownTimer]() {
        beeSpawnCountdown--;
        countdownCounter->setText(QString("Countdown: %1 Seconds").arg(beeSpawnCountdown));
        
        if (beeSpawnCountdown <= 0) {
            countdownTimer->stop();
            countdownTimer->deleteLater();
            countdownCounter->hide();
            startBeeWaves();
        }
    });
    countdownTimer->start(1000);
}

void GridWidget::startBeeWaves() {
    QTimer *waveTimer = new QTimer(this);
    connect(waveTimer, &QTimer::timeout, this, [this, waveTimer]() {
        if (currentSpawnWave < totalBeesToSpawn) {
            spawnBeeWave();
            currentSpawnWave++;
        } else {
            waveTimer->stop();
            waveTimer->deleteLater();
        }
    });
    waveTimer->start(1000);
}

void GridWidget::spawnBeeWave() {
    int beesThisWave = 5;
    beesSpawnedThisWave = 0;
    
    QTimer *spawnTimer = new QTimer(this);
    connect(spawnTimer, &QTimer::timeout, this, [this, spawnTimer, beesThisWave]() {
        if (beesSpawnedThisWave < beesThisWave) {
            spawnSingleBee();
            beesSpawnedThisWave++;
        } else {
            spawnTimer->stop();
            spawnTimer->deleteLater();
        }
    });
    spawnTimer->start(200);
}

void GridWidget::spawnSingleBee() {
    Bee bee;
    bee.position = QPoint(width() + 100 + (rand() % 50), MARGIN + (rand() % (height() - 64)));
    bee.direction = 0;
    bee.moving = true;
    bee.stunned = false;
    bee.stunnedTime = 0;
    bee.health = (rand() % 11) + 15; // 15-25 HP
    bee.maxHealth = bee.health;
    bee.touchingLine = false;
    bees.push_back(bee);
    update();
}

void GridWidget::updateCounter() {
    counter->setText(QString("Blocks Left: %1\nHP Left: %2")
                      .arg(m_gameData.boughtblocks)
                      .arg(m_gameData.current_hp));
    counter->adjustSize();
}

void GridWidget::initializeGameObjects() {
    bees.clear();
    drawnLines.clear();
    selectedPoints.clear();
    
    // Random dog position
    dogPos = QPoint(
        MARGIN + (rand() % (width()/2 - 128)),
        MARGIN + (rand() % (height() - 128))
    );
}

void GridWidget::updateBees() {
    if (beeSpawnCountdown > 0 || !gameActive) {
        return;
    }
    
    // Increment survival timer
    survivalTimer++;
    
    // Check win conditions
    checkWinConditions();
    
    if (!gameActive) return;
    
    // Check game over condition
    if (m_gameData.current_hp <= 0) {
        gameActive = false;
        QMessageBox::information(this, "Game Over", "The dog has been stung too many times! Game Over!");
        this->close();
        return;
    }
    
    // Health overflow check
    const unsigned long long HEALTH_THRESHOLD = ULLONG_MAX * 3 / 4;
    if (m_gameData.current_hp > HEALTH_THRESHOLD) {
        gameActive = false;
        QMessageBox::information(this, "Game Over", "Invalid health value detected! Game Over!");
        this->close();
        return;
    }
    
    // Update line health
    updateLineHealth();
    
    for(size_t i = 0; i < bees.size(); i++) {
        if (bees[i].stunned) {
            bees[i].stunnedTime++;
            if (bees[i].stunnedTime >= 60) {
                bees[i].stunned = false;
                bees[i].stunnedTime = 0;
                bees[i].moving = true;
                bees[i].touchingLine = false;
            }
            continue;
        }
        
        if (!bees[i].moving) continue;
        
        // Move toward dog if past midline
        if (bees[i].position.x() <= width() / 2) {
            int dx = dogPos.x() - bees[i].position.x();
            int dy = dogPos.y() - bees[i].position.y();
            double distance = sqrt(dx*dx + dy*dy);
            
            if (distance > 0) {
                bees[i].position.rx() += (dx / distance) * SPACING;
                bees[i].position.ry() += (dy / distance) * SPACING;
            }
        } else {
            // Move left
            bees[i].position.rx() -= SPACING * 2;
        }
        
        // Wall bouncing
        if (bees[i].position.x() < 0) {
            bees[i].position.rx() = 0;
            bees[i].position.ry() += (rand() % 3 - 1) * SPACING;
        }
        
        if (bees[i].position.y() < 0) bees[i].position.ry() = 0;
        if (bees[i].position.y() > height() - 64) bees[i].position.ry() = height() - 64;
        
        // Check dog collision
        if (QRect(dogPos, QSize(128,128)).intersects(QRect(bees[i].position, QSize(64,64)))) {
            m_gameData.current_hp -= (rand() % 2) + 2; // 2-3 damage
            bees[i].position.rx() += SPACING * 5; // Bounce back
            updateCounter();
            
            if (m_gameData.current_hp <= 0) {
                gameActive = false;
                QMessageBox::information(this, "Game Over", "The dog has been stung too many times! Game Over!");
                this->close();
                return;
            }
        }
        
        // Check line collisions
        checkLineCollisions(i);
        
        // Remove dead bees
        if (bees[i].health <= 0) {
            bees.erase(bees.begin() + i);
            i--;
            continue;
        }
        
        // Remove off-screen bees
        if (bees[i].position.x() < -100) {
            bees.erase(bees.begin() + i);
            i--;
        }
    }
    update();
}

void GridWidget::checkWinConditions() {
    // Survive for 10 seconds
    if (survivalTimer >= 600) {
        int xpReward = (rand() % 371) + 30; // 30-400 XP
        playerWins(xpReward);
        return;
    }
    
    // All bees dead and all waves spawned
    if (bees.empty() && currentSpawnWave >= totalBeesToSpawn) {
        int xpReward = (rand() % 371) + 30; // 30-400 XP
        playerWins(xpReward);
        return;
    }
}

void GridWidget::playerWins(int xpReward) {
    gameActive = false;
    m_gameData.auraxp += xpReward;
    
    QMessageBox::information(this, "Victory!", 
        QString("You won! The dog is safe!\n\n"
                "You earned %1 Aura XP!\n\n"
                "Total Aura XP: %2")
                .arg(xpReward)
                .arg(m_gameData.auraxp));
    
    this->close();
}

void GridWidget::updateLineHealth() {
    for (auto& line : drawnLines) {
        if (line.health > 0) {
            bool beeTouching = false;
            for (const auto& bee : bees) {
                if (bee.touchingLine) {
                    QRect beeRect(bee.position.x(), bee.position.y(), 64, 64);
                    QLineF topLine(beeRect.topLeft(), beeRect.topRight());
                    QLineF bottomLine(beeRect.bottomLeft(), beeRect.bottomRight());
                    QLineF leftLine(beeRect.topLeft(), beeRect.bottomLeft());
                    QLineF rightLine(beeRect.topRight(), beeRect.bottomRight());
                    
                    QPointF intersectionPoint;
                    if (line.line.intersects(topLine, &intersectionPoint) == QLineF::BoundedIntersection ||
                        line.line.intersects(bottomLine, &intersectionPoint) == QLineF::BoundedIntersection ||
                        line.line.intersects(leftLine, &intersectionPoint) == QLineF::BoundedIntersection ||
                        line.line.intersects(rightLine, &intersectionPoint) == QLineF::BoundedIntersection) {
                        beeTouching = true;
                        break;
                    }
                }
            }
            
            if (beeTouching) {
                line.health -= (rand() % 5) + 3;
                if (line.health <= 0) {
                    line.health = 0;
                    freeBeesFromLine(line);
                }
            }
        }
    }
}

void GridWidget::freeBeesFromLine(const Line &destroyedLine) {
    for (auto& bee : bees) {
        if (bee.stunned) {
            QRect beeRect(bee.position.x(), bee.position.y(), 64, 64);
            QLineF topLine(beeRect.topLeft(), beeRect.topRight());
            QLineF bottomLine(beeRect.bottomLeft(), beeRect.bottomRight());
            QLineF leftLine(beeRect.topLeft(), beeRect.bottomLeft());
            QLineF rightLine(beeRect.topRight(), beeRect.bottomRight());
            
            QPointF intersectionPoint;
            if (destroyedLine.line.intersects(topLine, &intersectionPoint) == QLineF::BoundedIntersection ||
                destroyedLine.line.intersects(bottomLine, &intersectionPoint) == QLineF::BoundedIntersection ||
                destroyedLine.line.intersects(leftLine, &intersectionPoint) == QLineF::BoundedIntersection ||
                destroyedLine.line.intersects(rightLine, &intersectionPoint) == QLineF::BoundedIntersection) {
                
                bee.stunned = false;
                bee.moving = true;
                bee.stunnedTime = 0;
                bee.touchingLine = false;
            }
        }
    }
}

void GridWidget::checkLineCollisions(size_t beeIndex) {
    bool isTouchingLine = false;
    
    for (auto& line : drawnLines) {
        if (line.health > 0) {
            QRect beeRect(bees[beeIndex].position.x(), bees[beeIndex].position.y(), 64, 64);
            
            QLineF topLine(beeRect.topLeft(), beeRect.topRight());
            QLineF bottomLine(beeRect.bottomLeft(), beeRect.bottomRight());
            QLineF leftLine(beeRect.topLeft(), beeRect.bottomLeft());
            QLineF rightLine(beeRect.topRight(), beeRect.bottomRight());
            
            QPointF intersectionPoint;
            if (line.line.intersects(topLine, &intersectionPoint) == QLineF::BoundedIntersection ||
                line.line.intersects(bottomLine, &intersectionPoint) == QLineF::BoundedIntersection ||
                line.line.intersects(leftLine, &intersectionPoint) == QLineF::BoundedIntersection ||
                line.line.intersects(rightLine, &intersectionPoint) == QLineF::BoundedIntersection) {
                
                isTouchingLine = true;
                bees[beeIndex].moving = false;
                bees[beeIndex].stunned = true;
                
                // 6-16 damage per second
                bees[beeIndex].health -= (rand() % 11) + 6;
                
                break;
            }
        }
    }
    
    bees[beeIndex].touchingLine = isTouchingLine;
}

void GridWidget::paintEvent(QPaintEvent *e) {
    updateCounter();
    Q_UNUSED(e);
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing);

    // Background
    p.fillRect(rect(), BG_COLOR);

    // Grid points
    p.setPen(GRID_COLOR);
    p.setBrush(GRID_COLOR);
    for (int y = 0; y < GRID_ROWS; ++y) {
        for (int x = 0; x < GRID_COLS; ++x) {
            int px = MARGIN + x * SPACING;
            int py = MARGIN + y * SPACING;
            p.drawEllipse(QPoint(px, py), POINT_RADIUS, POINT_RADIUS);
        }
    }

    // Lines with health bars
    p.setPen(QPen(LINE_COLOR, LINE_WIDTH));
    const int max_length = m_gameData.boughtblocks + 3;
    for (const Line &line : drawnLines) {
        if (line.health > 0) {
            QLineF qline(line.p1, line.p2);
            if (qline.length() > max_length) {
                qline.setLength(max_length);
            }
            p.drawLine(qline);
            
            QPoint midPoint = (line.p1 + line.p2) / 2;
            p.setPen(Qt::black);
            p.drawRect(midPoint.x() - 10, midPoint.y() - 15, 20, 5);
            p.fillRect(midPoint.x() - 10, midPoint.y() - 15, (line.health * 20) / 20, 5, Qt::green);
        }
    }

    // Selected points and temporary lines
    p.setPen(SELECT_COLOR);
    p.setBrush(SELECT_COLOR);
    if (!selectedPoints.empty()) {
        p.drawEllipse(selectedPoints[0], POINT_RADIUS+1, POINT_RADIUS+1);
        if (selectedPoints.size() == 2) {
            p.drawEllipse(selectedPoints[1], POINT_RADIUS+1, POINT_RADIUS+1);
            p.setPen(QPen(SELECT_COLOR, LINE_WIDTH));
            p.drawLine(selectedPoints[0], selectedPoints[1]);
        }
    }

    // Dog and bees
    p.drawPixmap(dogPos.x(), dogPos.y(), 128, 128, dogImage);
    
    for(const auto& bee : bees) {
        p.drawPixmap(bee.position.x(), bee.position.y(), 64, 64, beeImage);
        
        // Bee health bar
        p.setPen(Qt::black);
        p.drawRect(bee.position.x(), bee.position.y() - 10, 64, 5);
        int healthWidth = (bee.health * 64) / bee.maxHealth;
        p.fillRect(bee.position.x(), bee.position.y() - 10, healthWidth, 5, Qt::red);
    }
}

void GridWidget::mousePressEvent(QMouseEvent *e) {
    if (e->button() == Qt::LeftButton) {
        QPoint clickedP = getGridPoint(e->pos());
        if (clickedP.x() != -1) {
            if (selectedPoints.size() < 2) {
                selectedPoints.push_back(clickedP);
                if (selectedPoints.size() == 2) {
                    QLineF tempLine(selectedPoints[0], selectedPoints[1]);
                    const int max_length = m_gameData.boughtblocks + 3;
                    if (tempLine.length() > max_length) {
                        QMessageBox::warning(this, "Error", "Error: You don't have enough blocks.");
                        selectedPoints.clear();
                    } else {
                        Line newLine;
                        newLine.p1 = selectedPoints[0];
                        newLine.p2 = selectedPoints[1];
                        newLine.health = 20;
                        newLine.line = tempLine;
                        drawnLines.push_back(newLine);
                        m_gameData.boughtblocks -= qFloor(tempLine.length() / SPACING);
                        updateCounter();
                    }
                }
            } else {
                selectedPoints.clear();
                selectedPoints.push_back(clickedP);
            }
            update();
        }
    }
}

QPoint GridWidget::getGridPoint(const QPoint &mouse) {
    int ox = mouse.x() - MARGIN;
    int oy = mouse.y() - MARGIN;
    if (ox < 0 || oy < 0 || ox > (GRID_COLS-1)*SPACING || oy > (GRID_ROWS-1)*SPACING) {
        return QPoint(-1, -1);
    }
    int x = qRound(static_cast<double>(ox)/SPACING);
    int y = qRound(static_cast<double>(oy)/SPACING);
    if (x >=0 && x < GRID_COLS && y >=0 && y < GRID_ROWS) {
        return QPoint(MARGIN + x*SPACING, MARGIN + y*SPACING);
    }
    return QPoint(-1, -1);
}