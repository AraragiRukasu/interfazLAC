#ifndef READERTHREAD_H
#define READERTHREAD_H

#include <QObject>
#include <QThread>
#include "PC.h"
#include <QSerialPort>

using std::vector;

class ReaderThread : public QThread
{
    Q_OBJECT
    QSerialPort* thread_serial_port;
    uint readport(vector<char> &pila, uint16_t* first_byte, QSerialPort& serial_port);
    LACAN_MSG mensaje_recibido(char *sub_pila);
public:
    ReaderThread(QSerialPort& serial_port);
signals:
    void receivedMsg(LACAN_MSG msg);
    void msgLost(uint amountLostTillNow);
public slots:
    void handleRead();
};

#endif // READERTHREAD_H
