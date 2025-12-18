#include "recorddialog.h"
#include "recordmanager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>

RecordDialog::RecordDialog(QWidget *parent) : QDialog(parent) {
    setWindowTitle("Game Records");
    setMinimumSize(600, 400);
    
    table = new QTableWidget(this);
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({"Score", "Time Left", "Player", "Difficulty", "Date/Time"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    refreshBtn = new QPushButton("Refresh", this);
    cloudBtn = new QPushButton("Load from Cloud", this);
    
    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    buttonLayout->addWidget(refreshBtn);
    buttonLayout->addWidget(cloudBtn);
    
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(table);
    mainLayout->addLayout(buttonLayout);
    setLayout(mainLayout);
    
    connect(refreshBtn, &QPushButton::clicked, this, &RecordDialog::refreshRecords);
    connect(cloudBtn, &QPushButton::clicked, this, &RecordDialog::loadCloudRecords);
    
    populateTable();
}

void RecordDialog::populateTable() {
    auto records = RecordManager::loadRecords();
    table->setRowCount(records.size());
    
    for (int i = 0; i < records.size(); i++) {
        const Record& r = records[i];
        table->setItem(i, 0, new QTableWidgetItem(QString::number(r.score)));
        table->setItem(i, 1, new QTableWidgetItem(QString::number(r.time)));
        table->setItem(i, 2, new QTableWidgetItem(r.playerName));
        table->setItem(i, 3, new QTableWidgetItem(r.difficulty));
        table->setItem(i, 4, new QTableWidgetItem(r.dateTime));
    }
    
    table->resizeColumnsToContents();
}

void RecordDialog::refreshRecords() {
    populateTable();
}

void RecordDialog::loadCloudRecords() {
    RecordManager::loadFromCloud();
    // 实际应用中应该连接信号来更新表格
    // connect(RecordManager::instance(), &RecordManager::loadCompleted, 
    //         this, [this](const QVector<Record>& records) { ... });
    populateTable();
}
