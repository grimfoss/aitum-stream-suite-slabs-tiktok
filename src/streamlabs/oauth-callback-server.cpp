#include "oauth-callback-server.hpp"
#include <QNetworkInterface>

OAuthCallbackServer::OAuthCallbackServer(QObject *parent)
	: QObject(parent)
{
	m_server = new QTcpServer(this);
	connect(m_server, &QTcpServer::newConnection, this, &OAuthCallbackServer::onNewConnection);
}

OAuthCallbackServer::~OAuthCallbackServer()
{
	close();
}

bool OAuthCallbackServer::listen()
{
	return m_server->listen(QHostAddress::LocalHost, 0);
}

void OAuthCallbackServer::close()
{
	m_codeReceived.storeRelaxed(false);
	m_timedOut.storeRelaxed(false);
	m_authCode.clear();
	if (m_server && m_server->isListening()) {
		m_server->close();
	}
	if (m_socket) {
		m_socket->disconnectFromHost();
		m_socket->deleteLater();
		m_socket = nullptr;
	}
}

quint16 OAuthCallbackServer::port() const
{
	return m_server->serverPort();
}

void OAuthCallbackServer::onNewConnection()
{
	m_socket = m_server->nextPendingConnection();
	if (!m_socket)
		return;
	connect(m_socket, &QTcpSocket::readyRead, this, &OAuthCallbackServer::onReadyRead);
}

void OAuthCallbackServer::onReadyRead()
{
	if (!m_socket)
		return;

	QByteArray requestData = m_socket->readAll();
	QString requestStr = QString::fromUtf8(requestData);

	QStringList lines = requestStr.split("\r\n");
	if (lines.isEmpty())
		return;

	QString requestLine = lines.first();
	QStringList parts = requestLine.split(' ');
	if (parts.size() < 2) {
		m_socket->disconnectFromHost();
		return;
	}

	QString path = parts[1];
	QUrl url(QStringLiteral("http://127.0.0.1%1").arg(path));
	QUrlQuery query(url);

	if (query.hasQueryItem(QStringLiteral("code"))) {
		m_authCode = query.queryItemValue(QStringLiteral("code"));

		QByteArray responseBody =
			"<html><body><h2>Authentication successful! You can close this tab and return to OBS.</h2></body></html>";
		QByteArray response = QStringLiteral(
			"HTTP/1.1 200 OK\r\n"
			"Content-Type: text/html\r\n"
			"Content-Length: %1\r\n"
			"Connection: close\r\n"
			"\r\n%2")
					 .arg(responseBody.size())
					 .arg(QString::fromUtf8(responseBody))
					 .toUtf8();

		m_socket->write(response);
		m_socket->flush();
		m_socket->disconnectFromHost();

		m_codeReceived.storeRelaxed(true);
		emit codeReady();
	} else {
		QByteArray responseBody = "<html><body><h2>Authentication failed.</h2></body></html>";
		QByteArray response = QStringLiteral(
			"HTTP/1.1 400 Bad Request\r\n"
			"Content-Type: text/html\r\n"
			"Content-Length: %1\r\n"
			"Connection: close\r\n"
			"\r\n%2")
					 .arg(responseBody.size())
					 .arg(QString::fromUtf8(responseBody))
					 .toUtf8();

		m_socket->write(response);
		m_socket->flush();
		m_socket->disconnectFromHost();
	}
}
