#ifndef GAMEBOARD_H
#define GAMEBOARD_H
#include <QWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QVector>
#include <QPoint>
#include <QTimer>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QPixmap>

enum Difficulty {
    BEGINNER,    // 入门级：直连比例高
    PRIMARY,     // 初级：拐一个弯比例高
    INTERMEDIATE,// 中级：拐2个弯比例高
    ADVANCED     // 高级：最多3个弯
};

class GameBoard : public QWidget{
    Q_OBJECT
public:
    explicit GameBoard(int r,int c,QWidget *parent=nullptr);
    ~GameBoard();
    void resetBoard(bool onlyRemaining=false);
    void resetRemaining();
    bool findHint(QPoint &a,QPoint &b);
    void highlight(const QPoint &a,const QPoint &b);
    void clearHighlight();
    bool hasSolvablePairs();
    bool isStuck();
    void solveAutomatically();
    void solveNextPair(); // 递归求解下一对
    void setDifficulty(Difficulty d);
    Difficulty getDifficulty() const { return difficulty; }
    int getRemainingCount() const;
    QVector<QPoint> findPath(const QPoint& a, const QPoint& b);
    void drawConnectionLine(const QPoint& a, const QPoint& b);
    
signals:
    void pairRemoved(int points);
    void bonusTime(int seconds);
    void pairMatched(); // 配对成功信号，用于播放音效
    
private slots:
    void onAnimationFinished();
    void onConnectionAnimationFinished();
    
private:
    int rows,cols;
    QGridLayout *grid;
    QVector<QVector<int>> map;
    QVector<QVector<QPushButton*>> buttons;
    QPushButton *firstBtn;
    QPoint firstPos;
    bool hasFirst;
    Difficulty difficulty;
    QVector<QPair<QPoint, QPoint>> solvingPairs; // 存储配对，而不是路径
    QTimer *solveTimer;
    int solveStepIndex;
    int pairsRemoved;
    QVector<QPixmap> imageCache; // 缓存加载的图片
    QLabel *connectionLine; // 用于显示连线
    QTimer *connectionTimer;
    
    void generateMap();
    bool canLink(const QPoint&a,const QPoint&b, int maxTurns = -1);
    bool lineClearRow(int r,int c1,int c2);
    bool lineClearCol(int c,int r1,int r2);
    void setupButton(int i, int j, int value);
    QPixmap getImageForValue(int value);
    void updateButtonImage(QPushButton* btn, int value);
    QVector<QPoint> findPathInternal(const QPoint& a, const QPoint& b, int maxDepth);
    bool verifySolvability();
    bool verifySolvabilityQuick(); // 快速验证
    bool canLinkInternal(const QVector<QVector<int>>& tempMap, const QPoint& a, const QPoint& b, int maxTurns);
    void generateSolvableMap();
    void loadImages();
    void removePair(const QPoint& a, const QPoint& b);
};

#endif
