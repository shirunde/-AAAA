#ifndef RECORDMANAGER_H
#define RECORDMANAGER_H
#include <QVector>
#include <QString>
#include <QObject>
#include <QNetworkAccessManager>

struct Record {
    int score;
    int time;
    QString playerName;
    QString difficulty;
    QString dateTime;
    int id; // 云端ID
};

class RecordManager : public QObject {
    Q_OBJECT
    
public:
    static RecordManager* instance();
    static void saveRecord(int score, int time, const QString& playerName = "Player", 
                          const QString& difficulty = "Primary");
    static QVector<Record> loadRecords();
    static void syncToCloud();
    static void loadFromCloud();
    
    void setPlayerName(const QString& name) { playerName = name; }
    QString getPlayerName() const { return playerName; }
    
signals:
    void syncCompleted(bool success);
    void loadCompleted(const QVector<Record>& records);
    
private slots:
    void onSyncFinished();
    void onLoadFinished();
    
private:
    RecordManager(QObject* parent = nullptr);
    static RecordManager* inst;
    QString playerName;
    QNetworkAccessManager* networkManager;
    
    void saveRecordLocal(int score, int time, const QString& playerName, 
                        const QString& difficulty);
    QVector<Record> loadRecordsLocal();
};

#endif
