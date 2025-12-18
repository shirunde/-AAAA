#ifndef RECORDDIALOG_H
#define RECORDDIALOG_H
#include <QDialog>
#include <QTableWidget>
#include <QPushButton>

class RecordDialog: public QDialog {
    Q_OBJECT
public:
    explicit RecordDialog(QWidget *parent = nullptr);
    
private slots:
    void refreshRecords();
    void loadCloudRecords();
    
private:
    QTableWidget *table;
    QPushButton *refreshBtn;
    QPushButton *cloudBtn;
    void populateTable();
};

#endif
