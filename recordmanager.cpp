#include "recordmanager.h"
#include <QFile>
#include <QTextStream>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <algorithm>

RecordManager* RecordManager::inst = nullptr;

RecordManager::RecordManager(QObject* parent) : QObject(parent), playerName("Player") {
    networkManager = new QNetworkAccessManager(this);
}

RecordManager* RecordManager::instance() {
    if (!inst) {
        inst = new RecordManager();
    }
    return inst;
}

void RecordManager::saveRecord(int score, int time, const QString& playerName, 
                               const QString& difficulty) {
    instance()->saveRecordLocal(score, time, playerName, difficulty);
    
    // 同时尝试同步到云端
    syncToCloud();
}

void RecordManager::saveRecordLocal(int score, int time, const QString& playerName, 
                                    const QString& difficulty) {
    QFile f("records.txt");
    if (f.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&f);
        QString dateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        out << score << " " << time << " " << playerName << " " 
            << difficulty << " " << dateTime << "\n";
        f.close();
    }
}

QVector<Record> RecordManager::loadRecords() {
    return instance()->loadRecordsLocal();
}

QVector<Record> RecordManager::loadRecordsLocal() {
    QVector<Record> rs;
    QFile f("records.txt");
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&f);
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList parts = line.split(" ");
            if (parts.size() >= 2) {
                Record r;
                r.score = parts[0].toInt();
                r.time = parts[1].toInt();
                r.playerName = parts.size() > 2 ? parts[2] : "Player";
                r.difficulty = parts.size() > 3 ? parts[3] : "Primary";
                r.dateTime = parts.size() > 4 ? parts.mid(4).join(" ") : 
                             QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
                r.id = -1; // 本地记录没有ID
            rs.push_back(r);
        }
    }
        f.close();
    }
    
    // 按分数排序
    std::sort(rs.begin(), rs.end(), [](const Record& a, const Record& b) {
        return a.score > b.score;
    });
    
    return rs;
}

void RecordManager::syncToCloud() {
    // 注意：这里使用模拟的云端API
    // 实际使用时需要替换为真实的API端点
    // 例如使用Firebase, Supabase, 或其他后端服务
    
    QVector<Record> localRecords = instance()->loadRecordsLocal();
    
    // 创建JSON数据
    QJsonArray recordsArray;
    for (const Record& r : localRecords) {
        QJsonObject recordObj;
        recordObj["score"] = r.score;
        recordObj["time"] = r.time;
        recordObj["playerName"] = r.playerName;
        recordObj["difficulty"] = r.difficulty;
        recordObj["dateTime"] = r.dateTime;
        recordsArray.append(recordObj);
    }
    
    QJsonObject root;
    root["records"] = recordsArray;
    QJsonDocument doc(root);
    
    // 发送到云端API
    // 这里使用一个示例API，实际使用时需要替换
    QNetworkRequest request(QUrl("https://your-api-endpoint.com/records"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    QNetworkReply* reply = instance()->networkManager->post(request, doc.toJson());
    QObject::connect(reply, &QNetworkReply::finished, instance(), 
                    &RecordManager::onSyncFinished);
    
    // 如果没有网络连接或API不可用，静默失败（本地已保存）
    // 实际应用中可以添加错误处理
}

void RecordManager::onSyncFinished() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        bool success = (reply->error() == QNetworkReply::NoError);
        if (success) {
            qDebug() << "Records synced to cloud successfully";
        } else {
            qDebug() << "Failed to sync to cloud:" << reply->errorString();
        }
        emit instance()->syncCompleted(success);
        reply->deleteLater();
    }
}

void RecordManager::loadFromCloud() {
    // 从云端加载记录
    QNetworkRequest request(QUrl("https://your-api-endpoint.com/records"));
    QNetworkReply* reply = instance()->networkManager->get(request);
    QObject::connect(reply, &QNetworkReply::finished, instance(), 
                    &RecordManager::onLoadFinished);
}

void RecordManager::onLoadFinished() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender());
    QVector<Record> records;
    
    if (reply && reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            QJsonArray recordsArray = root["records"].toArray();
            
            for (const QJsonValue& value : recordsArray) {
                QJsonObject obj = value.toObject();
                Record r;
                r.id = obj["id"].toInt();
                r.score = obj["score"].toInt();
                r.time = obj["time"].toInt();
                r.playerName = obj["playerName"].toString();
                r.difficulty = obj["difficulty"].toString();
                r.dateTime = obj["dateTime"].toString();
                records.append(r);
            }
        }
    }
    
    emit instance()->loadCompleted(records);
    if (reply) {
        reply->deleteLater();
    }
}

