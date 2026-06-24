#pragma once

#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>
#include <QUrlQuery>
#include <QByteArray>
#include <QString>
#include <QAtomicInt>

class OAuthCallbackServer : public QObject {
	Q_OBJECT

public:
	explicit OAuthCallbackServer(QObject *parent = nullptr);
	~OAuthCallbackServer();

	bool listen();
	void close();
	quint16 port() const;

	QString authCode() const { return m_authCode; }
	bool codeReceived() const { return m_codeReceived.loadRelaxed(); }
	bool timedOut() const { return m_timedOut.loadRelaxed(); }

signals:
	void codeReady();

private slots:
	void onNewConnection();
	void onReadyRead();

private:
	QTcpServer *m_server = nullptr;
	QTcpSocket *m_socket = nullptr;
	QString m_authCode;
	QAtomicInt m_codeReceived{false};
	QAtomicInt m_timedOut{false};
};
