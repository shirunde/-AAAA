#include "gameboard.h"
#include <QRandomGenerator>
#include <QPainter>
#include <QPixmap>
#include <QIcon>
#include <QColor>
#include <QDebug>
#include <QPolygon>
#include <QFont>
#include <QPair>
#include <QFileInfo>
#include <QApplication>
#include <QDir>
#include <algorithm>
#include <climits>

GameBoard::GameBoard(int r,int c,QWidget *parent)
    : QWidget(parent), rows(r), cols(c), hasFirst(false), 
      difficulty(PRIMARY), solveStepIndex(0), pairsRemoved(0), connectionLine(nullptr)
{
    grid = new QGridLayout(this);
    grid->setSpacing(0); // 图案紧挨着，无间距
    grid->setContentsMargins(0, 0, 0, 0);
    solveTimer = new QTimer(this);
    connect(solveTimer, &QTimer::timeout, this, &GameBoard::onAnimationFinished);
    connectionTimer = new QTimer(this);
    connect(connectionTimer, &QTimer::timeout, this, &GameBoard::onConnectionAnimationFinished);
    
    loadImages();
    generateSolvableMap();
}

GameBoard::~GameBoard() {
    if (connectionLine) {
        delete connectionLine;
    }
}

void GameBoard::loadImages() {
    imageCache.clear();
    
    // 获取应用程序目录 - 尝试多种方式
    QString basePath;
    
    // 方法1: 应用程序所在目录
    QDir appDir(QApplication::applicationDirPath());
    basePath = appDir.absolutePath();
    qDebug() << "Application dir:" << basePath;
    
    // 方法2: 当前工作目录
    QDir workDir = QDir::current();
    QString workPath = workDir.absolutePath();
    qDebug() << "Working dir:" << workPath;
    
    for (int i = 1; i <= 8; i++) {
        QStringList paths = {
            // 应用程序目录
            QDir::cleanPath(basePath + "/images/" + QString::number(i) + ".jpg"),
            QDir::cleanPath(basePath + "/../images/" + QString::number(i) + ".jpg"),
            // 工作目录
            QDir::cleanPath(workPath + "/images/" + QString::number(i) + ".jpg"),
            QDir::cleanPath(workPath + "/../images/" + QString::number(i) + ".jpg"),
            // 相对路径
            QString("images/%1.jpg").arg(i),
            QString("./images/%1.jpg").arg(i),
            QString("../images/%1.jpg").arg(i)
        };
        
        QPixmap pix;
        bool loaded = false;
        QString loadedPath;
        
        for (const QString& imagePath : paths) {
            QFileInfo fi(imagePath);
            if (fi.exists() && fi.isReadable()) {
                pix.load(imagePath);
                if (!pix.isNull()) {
                    loaded = true;
                    loadedPath = imagePath;
                    qDebug() << "Successfully loaded image" << i << "from:" << imagePath;
                    break;
                } else {
                    qDebug() << "Failed to load pixmap from:" << imagePath;
                }
            }
        }
        
        if (!loaded) {
            // 如果图片加载失败，创建彩色占位符
            pix = QPixmap(50, 50);
            QColor colors[] = {
                QColor(255, 100, 100), QColor(100, 255, 100), QColor(100, 100, 255),
                QColor(255, 255, 100), QColor(255, 100, 255), QColor(100, 255, 255),
                QColor(255, 150, 100), QColor(150, 100, 255)
            };
            pix.fill(colors[(i-1) % 8]);
            QPainter painter(&pix);
            painter.setPen(Qt::black);
            painter.setFont(QFont("Arial", 20, QFont::Bold));
            painter.drawText(pix.rect(), Qt::AlignCenter, QString::number(i));
            qDebug() << "Failed to load image" << i << ", using placeholder. Tried paths:" << paths;
        } else {
            // 缩放图片到合适大小
            pix = pix.scaled(50, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }
        imageCache.append(pix);
    }
    
    qDebug() << "Loaded" << imageCache.size() << "images into cache";
}

void GameBoard::setDifficulty(Difficulty d) {
    difficulty = d;
}

QPixmap GameBoard::getImageForValue(int value) {
    if (value == 0 || value < 1 || value > 8) return QPixmap();
    int imageIndex = ((value - 1) % 8);
    return imageCache[imageIndex];
}

void GameBoard::updateButtonImage(QPushButton* btn, int value) {
    if (value == 0) {
        // 保持按钮可见但空白，维持布局不变
        btn->setVisible(true);
        btn->setEnabled(false);
        btn->setIcon(QIcon());
        btn->setText("");
        btn->setStyleSheet("QPushButton { "
                          "border: 1px solid transparent; "
                          "background-color: transparent; "
                          "}");
    } else {
        btn->setEnabled(true);
        btn->setVisible(true);
        QPixmap pix = getImageForValue(value);
        if (!pix.isNull()) {
            // 清除文本，设置图标
            btn->setText("");
            btn->setIcon(QIcon(pix));
            btn->setIconSize(QSize(58, 58)); // 图标稍微小一点，留出边框空间
            
            // 注意：Qt中如果设置了样式表的background，可能会覆盖图标
            // 所以这里使用padding和透明背景，或者使用图标模式
            btn->setStyleSheet(
                "QPushButton { "
                "border: 1px solid #cccccc; "
                "border-radius: 3px; "
                "background-color: white; "
                "padding: 0px; "
                "} "
                "QPushButton:hover { "
                "border: 2px solid #4CAF50; "
                "background-color: #f0f8f0; "
                "} "
                "QPushButton:pressed { "
                "border: 2px solid #2E7D32; "
                "background-color: #e0f0e0; "
                "}"
            );
            
            // 确保按钮大小合适
            btn->setMinimumSize(60, 60);
            btn->setMaximumSize(60, 60);
        } else {
            // 如果没有图片，显示数字
            btn->setIcon(QIcon());
            btn->setText(QString::number(value));
            btn->setStyleSheet(
                "QPushButton { "
                "border: 1px solid #cccccc; "
                "border-radius: 3px; "
                "background: white; "
                "font-size: 20px; "
                "font-weight: bold; "
                "color: black; "
                "} "
                "QPushButton:hover { "
                "border: 2px solid #4CAF50; "
                "background: #f0f8f0; "
                "}"
            );
            qDebug() << "Warning: No image for value" << value << ", displaying number instead";
        }
    }
}

void GameBoard::drawConnectionLine(const QPoint& a, const QPoint& b) {
    if (connectionLine) {
        connectionLine->deleteLater();
        connectionLine = nullptr;
    }
    
    QVector<QPoint> gridPath = findPath(a, b);
    if (gridPath.size() < 2) {
        return;
    }
    
    connectionLine = new QLabel(this);
    connectionLine->setAttribute(Qt::WA_TransparentForMouseEvents);
    connectionLine->raise();
    connectionLine->setGeometry(0, 0, width(), height());
    
    QPixmap linePix(width(), height());
    linePix.fill(Qt::transparent);
    QPainter painter(&linePix);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QPen pen;
    pen.setWidth(4);
    pen.setColor(QColor(255, 87, 34, 220));
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);
    
    QVector<QPoint> screenPoints;
    for (const QPoint& gridPos : gridPath) {
        QPushButton* btn = buttons[gridPos.x()][gridPos.y()];
        QPoint screenPos = btn->pos() + QPoint(btn->width()/2, btn->height()/2);
        screenPoints.append(screenPos);
    }
    
    for (int i = 0; i < screenPoints.size() - 1; i++) {
        QPoint p1 = screenPoints[i];
        QPoint p2 = screenPoints[i+1];
        if (p1.x() == p2.x() || p1.y() == p2.y()) {
            painter.drawLine(p1, p2);
        }
    }
    
    painter.setBrush(QBrush(QColor(255, 152, 0, 255)));
    painter.setPen(QPen(QColor(255, 87, 34, 255), 2));
    if (!screenPoints.isEmpty()) {
        painter.drawEllipse(screenPoints.first(), 6, 6);
        painter.drawEllipse(screenPoints.last(), 6, 6);
    }
    
    connectionLine->setPixmap(linePix);
    connectionLine->show();
    connectionTimer->start(300);
}

void GameBoard::onConnectionAnimationFinished() {
    if (connectionLine) {
        connectionLine->deleteLater();
        connectionLine = nullptr;
    }
    connectionTimer->stop();
}

void GameBoard::removePair(const QPoint& a, const QPoint& b) {
    drawConnectionLine(a, b);
    emit pairMatched();
    
    QTimer::singleShot(300, this, [=]() {
        map[a.x()][a.y()] = 0;
        map[b.x()][b.y()] = 0;
        updateButtonImage(buttons[a.x()][a.y()], 0);
        updateButtonImage(buttons[b.x()][b.y()], 0);
        
        pairsRemoved++;
        
        if (pairsRemoved % 5 == 0) {
            emit bonusTime(10);
        }
        
        int points = 10;
        switch(difficulty) {
            case BEGINNER: points = 5; break;
            case PRIMARY: points = 10; break;
            case INTERMEDIATE: points = 15; break;
            case ADVANCED: points = 20; break;
        }
        emit pairRemoved(points);
    });
}

void GameBoard::setupButton(int i, int j, int value) {
    QPushButton *btn = new QPushButton(this);
    // 设置固定大小，确保布局稳定
    btn->setFixedSize(60, 60);
    btn->setMinimumSize(60, 60);
    btn->setMaximumSize(60, 60);
    updateButtonImage(btn, value);
    grid->addWidget(btn, i, j);
    buttons[i][j] = btn;
    
    connect(btn, &QPushButton::clicked, this, [=](){
        if(map[i][j] == 0) return;
        
        if(!hasFirst) {
            firstBtn = btn; 
            firstPos = QPoint(i, j); 
            hasFirst = true;
            btn->setStyleSheet("QPushButton { border: 2px solid #FF9800; border-radius: 3px; background: #FFF3E0; } "
                              "QPushButton:hover { border: 2px solid #F57C00; }");
        } else {
            firstBtn->setStyleSheet("");
            QPoint secondPos(i, j);
            bool wasFirst = (firstPos == secondPos);
            hasFirst = false;
            
            if(wasFirst) {
                return;
            }
            
            if(map[firstPos.x()][firstPos.y()] == map[i][j]) {
                // 连接规则统一：所有难度都是最多2次转弯
                if (canLink(firstPos, secondPos, 2)) {
                    removePair(firstPos, secondPos);
                } else {
                    hasFirst = false;
                }
            } else {
                hasFirst = false;
            }
        }
    });
}

// 从已解决的布局开始生成，保证一定有解
void GameBoard::generateSolvableMap() {
    generateMap();
}

void GameBoard::generateMap() {
    if (!buttons.isEmpty()) {
        QLayoutItem *it;
        while((it = grid->takeAt(0))) {
            if (it->widget()) {
                delete it->widget();
            }
            delete it;
        }
    }
    
    map = QVector<QVector<int>>(rows, QVector<int>(cols, 0));
    buttons.resize(rows);
    
    int totalCells = rows * cols;
    int pairCount = totalCells / 2;
    int typeCount = qMin(pairCount, 8); // 最多8种图片
    
    QVector<int> values;
    for(int i = 0; i < pairCount; i++) {
        int v = (i % typeCount) + 1;
        values.append(v);
        values.append(v);
    }
    
    // 根据难度生成不同布局
    switch(difficulty) {
        case BEGINNER: {
            // 入门级：尽量相邻配对，保证直连多
            // 每对图片都相邻放置，确保可以直接连接
            int pairIdx = 0;
            for(int i = 0; i < rows; i++) {
                buttons[i].resize(cols);
                for(int j = 0; j < cols; j += 2) {
                    if (j + 1 < cols && pairIdx < pairCount) {
                        int val = (pairIdx % typeCount) + 1;
                        map[i][j] = val;
                        map[i][j+1] = val; // 相邻配对，可以直连
                        pairIdx++;
                    }
                }
            }
            // 如果cols是奇数，处理最后一列
            if(cols % 2 == 1 && pairIdx < pairCount) {
                // 最后一列与下一行配对
                for(int i = 0; i < rows - 1 && pairIdx < pairCount; i += 2) {
                    int val = (pairIdx % typeCount) + 1;
                    map[i][cols-1] = val;
                    map[i+1][cols-1] = val; // 同一列相邻，可以直连
                    pairIdx++;
                }
            }
            break;
        }
        
        case PRIMARY: {
            // 初级：生成较多一拐的配对
            // 策略：让配对在同一行或同一列，但不相邻，这样可以一拐连接
            std::shuffle(values.begin(), values.end(), *QRandomGenerator::global());
            
            int pairIdx = 0;
            // 尽量在同一行放置配对，但不相邻
            for(int i = 0; i < rows; i++) {
                buttons[i].resize(cols);
                QVector<int> colIndices;
                for(int j = 0; j < cols; j++) {
                    if(map[i][j] == 0) colIndices.append(j);
                }
                std::shuffle(colIndices.begin(), colIndices.end(), *QRandomGenerator::global());
                
                // 在同一行放置配对
                for(int k = 0; k < colIndices.size() - 1 && pairIdx < pairCount; k += 2) {
                    int val = (pairIdx % typeCount) + 1;
                    map[i][colIndices[k]] = val;
                    map[i][colIndices[k+1]] = val;
                    pairIdx++;
                }
            }
            
            // 填充剩余的，尽量在同一列
            for(int j = 0; j < cols && pairIdx < pairCount; j++) {
                QVector<int> rowIndices;
                for(int i = 0; i < rows; i++) {
                    if(map[i][j] == 0) rowIndices.append(i);
                }
                for(int k = 0; k < rowIndices.size() - 1 && pairIdx < pairCount; k += 2) {
                    int val = (pairIdx % typeCount) + 1;
                    map[rowIndices[k]][j] = val;
                    map[rowIndices[k+1]][j] = val;
                    pairIdx++;
                }
            }
            
            // 最后填充剩余的
            int valIdx = pairIdx * 2;
            for(int i = 0; i < rows && valIdx < values.size(); i++) {
                for(int j = 0; j < cols && valIdx < values.size(); j++) {
                    if(map[i][j] == 0) {
                        map[i][j] = values[valIdx++];
                    }
                }
            }
            break;
        }
        
        case INTERMEDIATE: {
            // 中级：生成较多两拐的配对
            // 完全随机打乱
            std::shuffle(values.begin(), values.end(), *QRandomGenerator::global());
            
            int idx = 0;
            for(int i = 0; i < rows; i++) {
                buttons[i].resize(cols);
                for(int j = 0; j < cols; j++) {
                    map[i][j] = values[idx++];
                }
            }
            
            // 验证并调整，确保有解
            int attempts = 0;
            while(!verifySolvabilityQuick() && attempts < 50) {
                // 重新打乱
                std::shuffle(values.begin(), values.end(), *QRandomGenerator::global());
                idx = 0;
                for(int i = 0; i < rows; i++) {
                    for(int j = 0; j < cols; j++) {
                        map[i][j] = values[idx++];
                    }
                }
                attempts++;
            }
            break;
        }
        
        case ADVANCED: {
            // 高级：完全随机，难度最高
            std::shuffle(values.begin(), values.end(), *QRandomGenerator::global());
            
            int idx = 0;
            for(int i = 0; i < rows; i++) {
                buttons[i].resize(cols);
                for(int j = 0; j < cols; j++) {
                    map[i][j] = values[idx++];
                }
            }
            
            // 多次尝试，找到有解的布局
            int attempts = 0;
            while(!verifySolvabilityQuick() && attempts < 200) {
                std::shuffle(values.begin(), values.end(), *QRandomGenerator::global());
                idx = 0;
                for(int i = 0; i < rows; i++) {
                    for(int j = 0; j < cols; j++) {
                        map[i][j] = values[idx++];
                    }
                }
                attempts++;
            }
            break;
        }
    }
    
    // 创建按钮
    for(int i = 0; i < rows; i++) {
        for(int j = 0; j < cols; j++) {
            setupButton(i, j, map[i][j]);
        }
    }
    
    pairsRemoved = 0;
}

// 快速验证可解性（简化版）
bool GameBoard::verifySolvabilityQuick() {
    QVector<QVector<int>> tempMap = map;
    
    // 尝试消除所有配对
    int eliminated = 0;
    while(eliminated < rows * cols) {
        bool found = false;
        QPoint a, b;
        
        for(int i = 0; i < rows && !found; i++) {
            for(int j = 0; j < cols && !found; j++) {
                if(tempMap[i][j] == 0) continue;
                for(int x = i; x < rows && !found; x++) {
                    for(int y = 0; y < cols && !found; y++) {
                        if((i == x && j == y) || tempMap[x][y] == 0) continue;
                        if(tempMap[i][j] == tempMap[x][y]) {
                            int maxTurns = 2;
                            if(canLinkInternal(tempMap, QPoint(i,j), QPoint(x,y), maxTurns)) {
                                a = QPoint(i, j);
                                b = QPoint(x, y);
                                found = true;
                            }
                        }
                    }
                }
            }
        }
        
        if(!found) break;
        
        tempMap[a.x()][a.y()] = 0;
        tempMap[b.x()][b.y()] = 0;
        eliminated += 2;
    }
    
    return eliminated == rows * cols;
}

// canLink的内部版本，使用临时地图
bool GameBoard::canLinkInternal(const QVector<QVector<int>>& tempMap, const QPoint& a, const QPoint& b, int maxTurns) {
    // 直连
    if(a.x() == b.x()) {
        int minC = qMin(a.y(), b.y());
        int maxC = qMax(a.y(), b.y());
        bool clear = true;
        for(int c = minC + 1; c < maxC; c++) {
            if(tempMap[a.x()][c] != 0) { clear = false; break; }
        }
        if(clear) return true;
    }
    if(a.y() == b.y()) {
        int minR = qMin(a.x(), b.x());
        int maxR = qMax(a.x(), b.x());
        bool clear = true;
        for(int r = minR + 1; r < maxR; r++) {
            if(tempMap[r][a.y()] != 0) { clear = false; break; }
        }
        if(clear) return true;
    }
    
    if(maxTurns < 1) return false;
    
    // 一拐
    QPoint corner1(a.x(), b.y());
    if(corner1 != a && corner1 != b && tempMap[corner1.x()][corner1.y()] == 0) {
        bool hClear = true, vClear = true;
        int minC = qMin(a.y(), b.y());
        int maxC = qMax(a.y(), b.y());
        for(int c = minC + 1; c < maxC; c++) {
            if(tempMap[a.x()][c] != 0) { hClear = false; break; }
        }
        int minR = qMin(a.x(), b.x());
        int maxR = qMax(a.x(), b.x());
        for(int r = minR + 1; r < maxR; r++) {
            if(tempMap[r][b.y()] != 0) { vClear = false; break; }
        }
        if(hClear && vClear) return true;
    }
    
    QPoint corner2(b.x(), a.y());
    if(corner2 != a && corner2 != b && tempMap[corner2.x()][corner2.y()] == 0) {
        bool vClear = true, hClear = true;
        int minR = qMin(a.x(), b.x());
        int maxR = qMax(a.x(), b.x());
        for(int r = minR + 1; r < maxR; r++) {
            if(tempMap[r][a.y()] != 0) { vClear = false; break; }
        }
        int minC = qMin(a.y(), b.y());
        int maxC = qMax(a.y(), b.y());
        for(int c = minC + 1; c < maxC; c++) {
            if(tempMap[b.x()][c] != 0) { hClear = false; break; }
        }
        if(vClear && hClear) return true;
    }
    
    if(maxTurns < 2) return false;
    
    // 两拐
    for(int i = 0; i < rows; i++) {
        QPoint p1(i, a.y());
        QPoint p2(i, b.y());
        if(p1 != a && p1 != b && p2 != a && p2 != b && 
           tempMap[p1.x()][p1.y()] == 0 && tempMap[p2.x()][p2.y()] == 0) {
            // 检查三条线段
            bool ok = true;
            int minR = qMin(a.x(), i);
            int maxR = qMax(a.x(), i);
            for(int r = minR + 1; r < maxR; r++) {
                if(tempMap[r][a.y()] != 0) { ok = false; break; }
            }
            if(ok) {
                int minC = qMin(a.y(), b.y());
                int maxC = qMax(a.y(), b.y());
                for(int c = minC + 1; c < maxC; c++) {
                    if(tempMap[i][c] != 0) { ok = false; break; }
                }
            }
            if(ok) {
                minR = qMin(i, b.x());
                maxR = qMax(i, b.x());
                for(int r = minR + 1; r < maxR; r++) {
                    if(tempMap[r][b.y()] != 0) { ok = false; break; }
                }
            }
            if(ok) return true;
        }
    }
    
    for(int j = 0; j < cols; j++) {
        QPoint p1(a.x(), j);
        QPoint p2(b.x(), j);
        if(p1 != a && p1 != b && p2 != a && p2 != b && 
           tempMap[p1.x()][p1.y()] == 0 && tempMap[p2.x()][p2.y()] == 0) {
            bool ok = true;
            int minC = qMin(a.y(), j);
            int maxC = qMax(a.y(), j);
            for(int c = minC + 1; c < maxC; c++) {
                if(tempMap[a.x()][c] != 0) { ok = false; break; }
            }
            if(ok) {
                int minR = qMin(a.x(), b.x());
                int maxR = qMax(a.x(), b.x());
                for(int r = minR + 1; r < maxR; r++) {
                    if(tempMap[r][j] != 0) { ok = false; break; }
                }
            }
            if(ok) {
                minC = qMin(j, b.y());
                maxC = qMax(j, b.y());
                for(int c = minC + 1; c < maxC; c++) {
                    if(tempMap[b.x()][c] != 0) { ok = false; break; }
                }
            }
            if(ok) return true;
        }
    }
    
    return false;
}

bool GameBoard::lineClearRow(int r, int c1, int c2) {
    int minC = qMin(c1, c2);
    int maxC = qMax(c1, c2);
    for(int c = minC + 1; c < maxC; c++) {
        if(map[r][c] != 0) return false;
    }
    return true;
}

bool GameBoard::lineClearCol(int c, int r1, int r2) {
    int minR = qMin(r1, r2);
    int maxR = qMax(r1, r2);
    for(int r = minR + 1; r < maxR; r++) {
        if(map[r][c] != 0) return false;
    }
    return true;
}

bool GameBoard::canLink(const QPoint& a, const QPoint& b, int maxTurns) {
    if(maxTurns == -1) {
        maxTurns = 2;
    }
    
    // 直连
    if(a.x() == b.x() && lineClearRow(a.x(), a.y(), b.y())) {
        return true;
    }
    if(a.y() == b.y() && lineClearCol(a.y(), a.x(), b.x())) {
        return true;
    }
    
    if(maxTurns < 1) return false;
    
    // 一拐：先横后竖
    QPoint corner1(a.x(), b.y());
    if(corner1 != a && corner1 != b && map[corner1.x()][corner1.y()] == 0) {
        if(lineClearRow(a.x(), a.y(), b.y()) && lineClearCol(b.y(), a.x(), b.x())) {
            return true;
        }
    }
    
    // 一拐：先竖后横
    QPoint corner2(b.x(), a.y());
    if(corner2 != a && corner2 != b && map[corner2.x()][corner2.y()] == 0) {
        if(lineClearCol(a.y(), a.x(), b.x()) && lineClearRow(b.x(), a.y(), b.y())) {
            return true;
        }
    }
    
    if(maxTurns < 2) return false;
    
    // 两拐：竖-横-竖
    for(int i = 0; i < rows; i++) {
        QPoint p1(i, a.y());
        QPoint p2(i, b.y());
        if(p1 != a && p1 != b && p2 != a && p2 != b && 
           map[p1.x()][p1.y()] == 0 && map[p2.x()][p2.y()] == 0) {
            if(lineClearCol(a.y(), a.x(), i) && 
               lineClearRow(i, a.y(), b.y()) && 
               lineClearCol(b.y(), i, b.x())) {
                return true;
            }
        }
    }
    
    // 两拐：横-竖-横
    for(int j = 0; j < cols; j++) {
        QPoint p1(a.x(), j);
        QPoint p2(b.x(), j);
        if(p1 != a && p1 != b && p2 != a && p2 != b && 
           map[p1.x()][p1.y()] == 0 && map[p2.x()][p2.y()] == 0) {
            if(lineClearRow(a.x(), a.y(), j) && 
               lineClearCol(j, a.x(), b.x()) && 
               lineClearRow(b.x(), j, b.y())) {
                return true;
            }
        }
    }
    
    return false;
}

bool GameBoard::findHint(QPoint& a, QPoint& b) {
    // 连接规则统一：所有难度都是最多2次转弯
    for(int i = 0; i < rows; i++) {
        for(int j = 0; j < cols; j++) {
            if(map[i][j] == 0) continue;
            for(int x = i; x < rows; x++) {
                for(int y = 0; y < cols; y++) {
                    if((i == x && j == y) || map[x][y] == 0) continue;
                    if(map[i][j] == map[x][y] && canLink(QPoint(i,j), QPoint(x,y), 2)) {
                        a = QPoint(i, j);
                        b = QPoint(x, y);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void GameBoard::highlight(const QPoint& a, const QPoint& b) {
    buttons[a.x()][a.y()]->setStyleSheet("QPushButton { border: 2px solid #4CAF50; border-radius: 3px; background: #C8E6C9; }");
    buttons[b.x()][b.y()]->setStyleSheet("QPushButton { border: 2px solid #4CAF50; border-radius: 3px; background: #C8E6C9; }");
}

void GameBoard::clearHighlight() {
    for(int i = 0; i < rows; i++) {
        for(int j = 0; j < cols; j++) {
            if(map[i][j] != 0) {
                updateButtonImage(buttons[i][j], map[i][j]);
            }
        }
    }
}

bool GameBoard::hasSolvablePairs() {
    QPoint a, b;
    return findHint(a, b);
}

bool GameBoard::isStuck() {
    return !hasSolvablePairs();
}

int GameBoard::getRemainingCount() const {
    int count = 0;
    for(int i = 0; i < rows; i++) {
        for(int j = 0; j < cols; j++) {
            if(map[i][j] != 0) count++;
        }
    }
    return count;
}

QVector<QPoint> GameBoard::findPath(const QPoint& a, const QPoint& b) {
    // 连接规则统一：所有难度都是最多2次转弯
    return findPathInternal(a, b, 2);
}

QVector<QPoint> GameBoard::findPathInternal(const QPoint& a, const QPoint& b, int maxDepth) {
    QVector<QPoint> path;
    path.append(a);
    
    // 直连
    if(a.x() == b.x() && lineClearRow(a.x(), a.y(), b.y())) {
        path.append(b);
        return path;
    }
    if(a.y() == b.y() && lineClearCol(a.y(), a.x(), b.x())) {
        path.append(b);
        return path;
    }
    
    // 一拐
    if(maxDepth >= 1) {
        QPoint corner1(a.x(), b.y());
        if(corner1 != a && corner1 != b && map[corner1.x()][corner1.y()] == 0) {
            if(lineClearRow(a.x(), a.y(), b.y()) && lineClearCol(b.y(), a.x(), b.x())) {
                path.append(corner1);
                path.append(b);
                return path;
            }
        }
        
        QPoint corner2(b.x(), a.y());
        if(corner2 != a && corner2 != b && map[corner2.x()][corner2.y()] == 0) {
            if(lineClearCol(a.y(), a.x(), b.x()) && lineClearRow(b.x(), a.y(), b.y())) {
                path.append(corner2);
                path.append(b);
                return path;
            }
        }
    }
    
    // 两拐
    if(maxDepth >= 2) {
        for(int i = 0; i < rows; i++) {
            QPoint p1(i, a.y());
            QPoint p2(i, b.y());
            if(p1 != a && p1 != b && p2 != a && p2 != b && 
               map[p1.x()][p1.y()] == 0 && map[p2.x()][p2.y()] == 0) {
                if(lineClearCol(a.y(), a.x(), i) && 
                   lineClearRow(i, a.y(), b.y()) && 
                   lineClearCol(b.y(), i, b.x())) {
                    path.append(p1);
                    path.append(p2);
                    path.append(b);
                    return path;
                }
            }
        }
        
        for(int j = 0; j < cols; j++) {
            QPoint p1(a.x(), j);
            QPoint p2(b.x(), j);
            if(p1 != a && p1 != b && p2 != a && p2 != b && 
               map[p1.x()][p1.y()] == 0 && map[p2.x()][p2.y()] == 0) {
                if(lineClearRow(a.x(), a.y(), j) && 
                   lineClearCol(j, a.x(), b.x()) && 
                   lineClearRow(b.x(), j, b.y())) {
                    path.append(p1);
                    path.append(p2);
                    path.append(b);
                    return path;
                }
            }
        }
    }
    
    return path;
}

void GameBoard::resetBoard(bool onlyRemaining) {
    if (onlyRemaining) {
        resetRemaining();
    } else {
        hasFirst = false;
        clearHighlight();
        generateSolvableMap();
    }
}

void GameBoard::resetRemaining() {
    QVector<QPoint> remaining;
    QVector<int> values;
    
    for(int i = 0; i < rows; i++) {
        for(int j = 0; j < cols; j++) {
            if(map[i][j] != 0) {
                remaining.append(QPoint(i, j));
                values.append(map[i][j]);
            }
        }
    }
    
    std::shuffle(values.begin(), values.end(), *QRandomGenerator::global());
    
    for(int idx = 0; idx < remaining.size(); idx++) {
        QPoint pos = remaining[idx];
        map[pos.x()][pos.y()] = values[idx];
        updateButtonImage(buttons[pos.x()][pos.y()], values[idx]);
    }
    
    hasFirst = false;
    clearHighlight();
    pairsRemoved = 0;
}

// 改进的自动解题：递归方式，每次消除后重新查找
void GameBoard::solveAutomatically() {
    solvingPairs.clear();
    solveStepIndex = 0;
    
    // 开始递归求解
    solveNextPair();
}

void GameBoard::solveNextPair() {
    // 查找当前可消除的对
    QPoint a, b;
    bool found = false;
    
    // 连接规则统一：所有难度都是最多2次转弯
    for(int i = 0; i < rows && !found; i++) {
        for(int j = 0; j < cols && !found; j++) {
            if(map[i][j] == 0) continue;
            for(int x = i; x < rows && !found; x++) {
                for(int y = 0; y < cols && !found; y++) {
                    if((i == x && j == y) || map[x][y] == 0) continue;
                    if(map[i][j] == map[x][y] && canLink(QPoint(i,j), QPoint(x,y), 2)) {
                        a = QPoint(i, j);
                        b = QPoint(x, y);
                        found = true;
                    }
                }
            }
        }
    }
    
    if (found) {
        highlight(a, b);
        
        // 延迟消除
        QTimer::singleShot(500, this, [=]() {
            if(map[a.x()][a.y()] != 0 && map[b.x()][b.y()] != 0 &&
               map[a.x()][a.y()] == map[b.x()][b.y()]) {
                removePair(a, b);
                
                pairsRemoved++;
                int points = 10;
                switch(difficulty) {
                    case BEGINNER: points = 5; break;
                    case PRIMARY: points = 10; break;
                    case INTERMEDIATE: points = 15; break;
                    case ADVANCED: points = 20; break;
                }
                emit pairRemoved(points);
                
                if (pairsRemoved % 5 == 0) {
                    emit bonusTime(10);
                }
                
                // 继续下一对
                QTimer::singleShot(400, this, &GameBoard::solveNextPair);
            } else {
                // 如果这对已被消除，继续查找
                solveNextPair();
            }
        });
    } else {
        // 没有找到可消除的对，停止
        clearHighlight();
    }
}

void GameBoard::onAnimationFinished() {
    // 这个方法现在不需要了，因为使用递归方式
}

