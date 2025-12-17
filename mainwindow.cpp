#include "mainwindow.h"
#include "recorddialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QGroupBox>
#include <QFileInfo>
#include <QDir>
#include <QDebug>
#include <QUrl>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), score(0), timeLeft(120), initialTime(120), hintCount(3),
      isPaused(false), isPlaying(false)
{
    setupUI();
    
    // 获取应用程序目录
    QDir appDir = QDir::current();
    QString basePath = appDir.absolutePath();
    
    // 初始化音效 - 尝试多种路径
    matchSoundAudio = new QAudioOutput(this);
    matchSoundPlayer = new QMediaPlayer(this);
    matchSoundPlayer->setAudioOutput(matchSoundAudio);
    QStringList matchPaths = {
        QDir::cleanPath(basePath + "/sounds/相连音效.mp3"),
        QDir::cleanPath(basePath + "/../sounds/相连音效.mp3"),
        "sounds/相连音效.mp3",
        "./sounds/相连音效.mp3"
    };
    for (const QString& path : matchPaths) {
        QFileInfo fi(path);
        if (fi.exists() && fi.isReadable()) {
            matchSoundPlayer->setSource(QUrl::fromLocalFile(path));
            qDebug() << "Loaded match sound:" << path;
            break;
        }
    }
    matchSoundAudio->setVolume(0.5);
    
    winSoundAudio = new QAudioOutput(this);
    winSoundPlayer = new QMediaPlayer(this);
    winSoundPlayer->setAudioOutput(winSoundAudio);
    QStringList winPaths = {
        QDir::cleanPath(basePath + "/sounds/胜利.mp3"),
        QDir::cleanPath(basePath + "/../sounds/胜利.mp3"),
        "sounds/胜利.mp3",
        "./sounds/胜利.mp3"
    };
    for (const QString& path : winPaths) {
        QFileInfo fi(path);
        if (fi.exists() && fi.isReadable()) {
            winSoundPlayer->setSource(QUrl::fromLocalFile(path));
            qDebug() << "Loaded win sound:" << path;
            break;
        }
    }
    winSoundAudio->setVolume(0.6);
    
    // 背景音乐
    bgmAudio = new QAudioOutput(this);
    bgmPlayer = new QMediaPlayer(this);
    bgmPlayer->setAudioOutput(bgmAudio);
    QStringList bgmPaths = {
        QDir::cleanPath(basePath + "/sounds/bgm.mp3"),
        QDir::cleanPath(basePath + "/../sounds/bgm.mp3"),
        "sounds/bgm.mp3",
        "./sounds/bgm.mp3"
    };
    for (const QString& path : bgmPaths) {
        QFileInfo fi(path);
        if (fi.exists() && fi.isReadable()) {
            bgmPlayer->setSource(QUrl::fromLocalFile(path));
            qDebug() << "Loaded BGM:" << path;
            break;
        }
    }
    bgmAudio->setVolume(0.3);
    bgmPlayer->setLoops(QMediaPlayer::Infinite);
    
    // 连接信号
    connect(board, &GameBoard::pairRemoved, this, &MainWindow::onPairRemoved);
    connect(board, &GameBoard::bonusTime, this, &MainWindow::onBonusTime);
    connect(board, &GameBoard::pairMatched, this, &MainWindow::onPairMatched);
    
    updateUI();
}

MainWindow::~MainWindow() {
    // Qt会自动清理子对象
}

void MainWindow::setupUI() {
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    // 设置窗口样式
    setStyleSheet("QMainWindow { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, "
                  "stop:0 #E3F2FD, stop:1 #BBDEFB); }");
    
    // 创建标签
    scoreLabel = new QLabel("Score: 0", this);
    timeLabel  = new QLabel("Time: 120s", this);
    hintLabel  = new QLabel("Hints: 3", this);
    difficultyLabel = new QLabel("Difficulty:", this);
    
    scoreLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #1976D2; padding: 5px;");
    timeLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #1976D2; padding: 5px;");
    hintLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #1976D2; padding: 5px;");
    difficultyLabel->setStyleSheet("font-size: 14px; color: #424242; padding: 5px;");
    
    // 时间进度条
    timeProgressBar = new QProgressBar(this);
    timeProgressBar->setRange(0, initialTime);
    timeProgressBar->setValue(initialTime);
    timeProgressBar->setTextVisible(true);
    timeProgressBar->setFormat("%v / %m 秒");
    timeProgressBar->setStyleSheet(
        "QProgressBar { border: 2px solid #1976D2; border-radius: 5px; text-align: center; "
        "background: #E3F2FD; height: 25px; } "
        "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "stop:0 #4CAF50, stop:0.5 #8BC34A, stop:1 #CDDC39); border-radius: 3px; }");
    
    // 创建按钮 - 美化样式
    QString buttonStyle = 
        "QPushButton { font-size: 12px; font-weight: bold; padding: 8px 15px; "
        "border-radius: 5px; border: 2px solid #1976D2; background: #2196F3; "
        "color: white; min-width: 80px; } "
        "QPushButton:hover { background: #1976D2; border-color: #0D47A1; } "
        "QPushButton:pressed { background: #0D47A1; } "
        "QPushButton:disabled { background: #BBDEFB; border-color: #90CAF9; color: #757575; }";
    
    startBtn = new QPushButton("▶ Start", this);
    pauseBtn = new QPushButton("⏸ Pause", this);
    pauseBtn->setEnabled(false);
    endBtn   = new QPushButton("■ End", this);
    hintBtn  = new QPushButton("💡 Hint", this);
    hintBtn->setEnabled(false);
    resetBtn = new QPushButton("🔄 Reset", this);
    resetBtn->setEnabled(false);
    autoSolveBtn = new QPushButton("⚡ Auto", this);
    autoSolveBtn->setEnabled(false);
    recordBtn= new QPushButton("📊 Records", this);

    startBtn->setStyleSheet(buttonStyle);
    pauseBtn->setStyleSheet(buttonStyle);
    endBtn->setStyleSheet(buttonStyle.replace("#2196F3", "#F44336").replace("#1976D2", "#C62828"));
    hintBtn->setStyleSheet(buttonStyle.replace("#2196F3", "#FF9800").replace("#1976D2", "#E65100"));
    resetBtn->setStyleSheet(buttonStyle.replace("#2196F3", "#FFC107").replace("#1976D2", "#F57C00"));
    autoSolveBtn->setStyleSheet(buttonStyle.replace("#2196F3", "#9C27B0").replace("#1976D2", "#6A1B9A"));
    recordBtn->setStyleSheet(buttonStyle.replace("#2196F3", "#607D8B").replace("#1976D2", "#37474F"));

    // 难度选择
    difficultyCombo = new QComboBox(this);
    difficultyCombo->addItem("🌱 Beginner", BEGINNER);
    difficultyCombo->addItem("⭐ Primary", PRIMARY);
    difficultyCombo->addItem("🔥 Intermediate", INTERMEDIATE);
    difficultyCombo->addItem("💀 Advanced", ADVANCED);
    difficultyCombo->setCurrentIndex(1);
    difficultyCombo->setStyleSheet(
        "QComboBox { font-size: 13px; padding: 5px; border: 2px solid #1976D2; "
        "border-radius: 5px; background: white; min-width: 150px; } "
        "QComboBox:hover { border-color: #0D47A1; } "
        "QComboBox::drop-down { border: none; width: 30px; } "
        "QComboBox::down-arrow { image: none; border-left: 5px solid transparent; "
        "border-right: 5px solid transparent; border-top: 5px solid #1976D2; width: 0; height: 0; }");
    difficultyCombo->setEnabled(true);
    
    // 自动重置选项
    autoResetCheck = new QCheckBox("Auto Reset on Stuck", this);
    autoResetCheck->setChecked(true);
    autoResetCheck->setStyleSheet(
        "QCheckBox { font-size: 13px; color: #424242; spacing: 5px; } "
        "QCheckBox::indicator { width: 18px; height: 18px; border: 2px solid #1976D2; "
        "border-radius: 3px; background: white; } "
        "QCheckBox::indicator:checked { background: #2196F3; }");

    // 布局
    QHBoxLayout *infoLayout = new QHBoxLayout;
    infoLayout->addWidget(scoreLabel);
    infoLayout->addWidget(timeLabel);
    infoLayout->addWidget(hintLabel);
    infoLayout->addWidget(difficultyLabel);
    infoLayout->addWidget(difficultyCombo);
    infoLayout->addStretch();
    
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(startBtn);
    buttonLayout->addWidget(pauseBtn);
    buttonLayout->addWidget(endBtn);
    buttonLayout->addWidget(hintBtn);
    buttonLayout->addWidget(resetBtn);
    buttonLayout->addWidget(autoSolveBtn);
    buttonLayout->addWidget(recordBtn);
    buttonLayout->addWidget(autoResetCheck);
    buttonLayout->addStretch();

    board = new GameBoard(6, 6, this);
    board->setDifficulty(PRIMARY);

    QVBoxLayout *main = new QVBoxLayout;
    main->setSpacing(10);
    main->setContentsMargins(10, 10, 10, 10);
    main->addLayout(infoLayout);
    main->addWidget(timeProgressBar);
    main->addLayout(buttonLayout);
    main->addWidget(board, 1);
    central->setLayout(main);

    // 定时器
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::updateTime);
    
    stuckCheckTimer = new QTimer(this);
    connect(stuckCheckTimer, &QTimer::timeout, this, &MainWindow::checkStuck);
    stuckCheckTimer->start(2000); // 每2秒检查一次是否僵局

    // 连接信号
    connect(startBtn, &QPushButton::clicked, this, &MainWindow::startGame);
    connect(pauseBtn, &QPushButton::clicked, this, &MainWindow::pauseGame);
    connect(endBtn, &QPushButton::clicked, this, &MainWindow::endGame);
    connect(hintBtn, &QPushButton::clicked, this, &MainWindow::showHint);
    connect(resetBtn, &QPushButton::clicked, this, &MainWindow::resetRemaining);
    connect(autoSolveBtn, &QPushButton::clicked, this, &MainWindow::autoSolve);
    connect(recordBtn, &QPushButton::clicked, this, [=](){ 
        RecordDialog d(this); 
        d.exec(); 
    });
    connect(difficultyCombo, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this, &MainWindow::onDifficultyChanged);

    setWindowTitle("连连看游戏 - Enhanced Version");
    resize(750, 750);
}

void MainWindow::startGame() {
    score = 0;
    timeLeft = initialTime = 120;
    hintCount = 3;
    isPaused = false;
    isPlaying = true;
    
    Difficulty d = static_cast<Difficulty>(difficultyCombo->currentData().toInt());
    board->setDifficulty(d);
    board->resetBoard(false);
    
    timer->start(1000);
    bgmPlayer->play();
    
    updateUI();
}

void MainWindow::pauseGame() {
    if (isPaused) {
        resumeGame();
        return;
    }
    
    isPaused = true;
    timer->stop();
    bgmPlayer->pause();
    pauseBtn->setText("▶ Resume");
    hintBtn->setEnabled(false);
    resetBtn->setEnabled(false);
    autoSolveBtn->setEnabled(false);
}

void MainWindow::resumeGame() {
    isPaused = false;
    timer->start(1000);
    bgmPlayer->play();
    pauseBtn->setText("⏸ Pause");
    
    if (isPlaying) {
        hintBtn->setEnabled(hintCount > 0);
        resetBtn->setEnabled(true);
        autoSolveBtn->setEnabled(true);
    }
}

void MainWindow::updateTime() {
    if (!isPaused && isPlaying) {
    timeLeft--;
        timeLabel->setText(QString("Time: %1s").arg(timeLeft));
        timeProgressBar->setValue(timeLeft);
        
        // 根据剩余时间改变进度条颜色
        if (timeLeft < initialTime * 0.2) {
            timeProgressBar->setStyleSheet(
                "QProgressBar { border: 2px solid #D32F2F; border-radius: 5px; text-align: center; "
                "background: #FFEBEE; height: 25px; } "
                "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
                "stop:0 #F44336, stop:1 #E91E63); border-radius: 3px; }");
        } else if (timeLeft < initialTime * 0.5) {
            timeProgressBar->setStyleSheet(
                "QProgressBar { border: 2px solid #F57C00; border-radius: 5px; text-align: center; "
                "background: #FFF3E0; height: 25px; } "
                "QProgressBar::chunk { background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
                "stop:0 #FF9800, stop:1 #FFC107); border-radius: 3px; }");
        }
        
        if (timeLeft <= 0) {
            endGame();
        }
    }
}

void MainWindow::onPairRemoved(int points) {
    if (!isPlaying) return;
    
    score += points;
    scoreLabel->setText(QString("Score: %1").arg(score));
    
    // 检查是否完成
    if (board->getRemainingCount() == 0) {
        playWinSound();
        QMessageBox::information(this, "🎉 Congratulations!", 
                                QString("You completed the game!\nFinal Score: %1\nTime Bonus: +%2 points")
                                .arg(score).arg(timeLeft * 2));
        score += timeLeft * 2; // 时间奖励
        scoreLabel->setText(QString("Score: %1").arg(score));
        endGame();
    }
}

void MainWindow::onBonusTime(int seconds) {
    timeLeft += seconds;
    initialTime += seconds; // 也增加初始时间，让进度条正确显示
    timeProgressBar->setMaximum(initialTime);
    timeProgressBar->setValue(timeLeft);
    timeLabel->setText(QString("Time: %1s").arg(timeLeft));
    // 不再弹窗，静默添加时间
}

void MainWindow::onPairMatched() {
    playMatchSound();
}

void MainWindow::playMatchSound() {
    if (matchSoundPlayer && matchSoundPlayer->source().isLocalFile()) {
        matchSoundPlayer->stop();
        matchSoundPlayer->setPosition(0);
        matchSoundPlayer->play();
    }
}

void MainWindow::playWinSound() {
    if (winSoundPlayer && winSoundPlayer->source().isLocalFile()) {
        winSoundPlayer->stop();
        winSoundPlayer->setPosition(0);
        winSoundPlayer->play();
    }
}

void MainWindow::showHint() {
    if (!isPlaying || isPaused) return;
    if (hintCount <= 0) {
        QMessageBox::warning(this, "No Hints", "You have used all hints!");
        return;
    }
    
    QPoint a, b;
    if (board->findHint(a, b)) {
        hintCount--;
        hintLabel->setText(QString("Hints: %1").arg(hintCount));
        board->highlight(a, b);
        
        // 3秒后清除高亮
        QTimer::singleShot(3000, board, &GameBoard::clearHighlight);
        
        if (hintCount == 0) {
            hintBtn->setEnabled(false);
        }
    } else {
        QMessageBox::information(this, "No Hints", "No available pairs to hint!");
    }
}

void MainWindow::resetRemaining() {
    if (!isPlaying || isPaused) return;
    
    // 检查重置前是否有解
    bool hadSolution = board->hasSolvablePairs();
    
    if (hadSolution) {
        int penalty = qMin(score, 50); // 最多扣50分
        score = qMax(0, score - penalty);
        scoreLabel->setText(QString("Score: %1").arg(score));
        
        QMessageBox::warning(this, "⚠️ Penalty", 
                            QString("You had solvable pairs! Penalty: -%1 points")
                            .arg(penalty));
    }
    
    board->resetRemaining();
    hintBtn->setEnabled(hintCount > 0);
}

void MainWindow::autoSolve() {
    if (!isPlaying || isPaused) return;
    
    int ret = QMessageBox::question(this, "Auto Solve", 
                                    "This will automatically solve the puzzle. Continue?",
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        hintBtn->setEnabled(false);
        resetBtn->setEnabled(false);
        autoSolveBtn->setEnabled(false);
        board->solveAutomatically();
    }
}

void MainWindow::checkStuck() {
    if (!isPlaying || isPaused) return;
    
    if (board->isStuck() && board->getRemainingCount() > 0) {
        if (autoResetCheck->isChecked()) {
            QMessageBox::information(this, "⚠️ Stuck Detected", 
                                    "No solvable pairs found! Auto-resetting remaining pieces...");
            resetRemaining();
        } else {
            QMessageBox::warning(this, "⚠️ Stuck!", 
                                "No solvable pairs found! Use Reset Remaining button.");
        }
    }
}

void MainWindow::onDifficultyChanged(int index) {
    if (!isPlaying) {
        Difficulty d = static_cast<Difficulty>(difficultyCombo->itemData(index).toInt());
        board->setDifficulty(d);
    }
}

void MainWindow::endGame() {
    isPlaying = false;
    isPaused = false;
    timer->stop();
    bgmPlayer->stop();
    
    saveGameRecord();
    
    QString message = QString("Game Over!\n\nFinal Score: %1\nTime Remaining: %2s")
                     .arg(score).arg(timeLeft);
    QMessageBox::information(this, "Game Over", message);
    
    updateUI();
}

void MainWindow::updateUI() {
    scoreLabel->setText(QString("Score: %1").arg(score));
    timeLabel->setText(QString("Time: %1s").arg(timeLeft));
    hintLabel->setText(QString("Hints: %1").arg(hintCount));
    timeProgressBar->setMaximum(initialTime);
    timeProgressBar->setValue(timeLeft);
    
    startBtn->setEnabled(!isPlaying);
    pauseBtn->setEnabled(isPlaying);
    pauseBtn->setText(isPaused ? "▶ Resume" : "⏸ Pause");
    hintBtn->setEnabled(isPlaying && !isPaused && hintCount > 0);
    resetBtn->setEnabled(isPlaying && !isPaused);
    autoSolveBtn->setEnabled(isPlaying && !isPaused);
    difficultyCombo->setEnabled(!isPlaying);
}

void MainWindow::saveGameRecord() {
    if (score > 0) {
        QString difficultyStr = difficultyCombo->currentText();
        RecordManager::saveRecord(score, timeLeft, "Player", difficultyStr);
}
}
