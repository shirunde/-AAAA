#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QComboBox>
#include <QCheckBox>
#include <QProgressBar>
#include "gameboard.h"
#include "recordmanager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
private slots:
    void startGame();
    void pauseGame();
    void resumeGame();
    void endGame();
    void updateTime();
    void onPairRemoved(int points);
    void onBonusTime(int seconds);
    void onPairMatched(); // 配对成功，播放音效
    void showHint();
    void resetRemaining();
    void autoSolve();
    void checkStuck();
    void onDifficultyChanged(int index);
    
private:
    QLabel *scoreLabel;
    QLabel *timeLabel;
    QLabel *hintLabel;
    QLabel *difficultyLabel;
    QProgressBar *timeProgressBar;
    QPushButton *startBtn;
    QPushButton *pauseBtn;
    QPushButton *endBtn;
    QPushButton *hintBtn;
    QPushButton *resetBtn;
    QPushButton *autoSolveBtn;
    QPushButton *recordBtn;
    QComboBox *difficultyCombo;
    QCheckBox *autoResetCheck;
    
    GameBoard *board;
    QTimer *timer;
    QTimer *stuckCheckTimer;
    QMediaPlayer *bgmPlayer;
    QAudioOutput *bgmAudio;
    QMediaPlayer *matchSoundPlayer;
    QAudioOutput *matchSoundAudio;
    QMediaPlayer *winSoundPlayer;
    QAudioOutput *winSoundAudio;
    
    int score;
    int timeLeft;
    int initialTime;
    int hintCount;
    bool isPaused;
    bool isPlaying;
    
    void updateUI();
    void saveGameRecord();
    void setupUI();
    void playMatchSound();
    void playWinSound();
};

#endif
