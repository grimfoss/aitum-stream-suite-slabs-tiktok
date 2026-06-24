#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QJsonObject>
#include <QJsonArray>
#include <functional>

struct StreamlabsStreamInfo {
	QString rtmpUrl;
	QString streamKey;
	QString streamId;
};

struct StreamlabsAccountInfo {
	QString username;
	bool canBeLive = false;
	QString applicationStatus;
};

struct StreamlabsCategory {
	QString fullName;
	QString gameMaskId;
};

class StreamlabsApiClient : public QObject {
	Q_OBJECT

public:
	explicit StreamlabsApiClient(QObject *parent = nullptr);

	// OAuth flow
	void startOAuthLogin();
	QString waitForToken(int timeoutMs = 300000);
	QString lastOAuthError() const { return m_lastError; }

	// API calls (blocking wrappers around async network)
	StreamlabsAccountInfo getAccountInfo(const QString &token);
	QVector<StreamlabsCategory> searchCategories(const QString &token, const QString &query);
	StreamlabsStreamInfo startStream(const QString &token, const QString &title,
					 const QString &categoryId, const QString &audienceType);
	bool endStream(const QString &token, const QString &streamId);

	// Async API
	void getAccountInfoAsync(const QString &token,
				 std::function<void(StreamlabsAccountInfo, bool success)> callback);
	void searchCategoriesAsync(const QString &token, const QString &query,
				   std::function<void(QVector<StreamlabsCategory>, bool success)> callback);
	void startStreamAsync(const QString &token, const QString &title,
			      const QString &categoryId, const QString &audienceType,
			      std::function<void(StreamlabsStreamInfo, bool success)> callback);
	void endStreamAsync(const QString &token, const QString &streamId,
			    std::function<void(bool success)> callback);

	// Error access
	QString lastError() const { return m_lastError; }
	int lastHttpStatus() const { return m_lastHttpStatus; }

private:
	QString generateCodeVerifier();
	QString generateCodeChallenge(const QString &verifier);
	QString generateRandomString(int length);

	struct CurlResult {
		QByteArray data;
		int statusCode = 0;
		QString error;
		bool ok = false;
	};
	CurlResult curlRequest(const QString &url, const QByteArray &postData = QByteArray(),
			       const QString &token = QString(),
			       const QString &contentType = QString(),
			       int timeoutMs = 30000);
	QJsonObject curlRequestJson(const QString &url, const QByteArray &postData = QByteArray(),
				    const QString &token = QString(),
				    const QString &contentType = QString(),
				    int timeoutMs = 30000);

	QString m_lastError;
	int m_lastHttpStatus = 0;

	// PKCE state
	QString m_codeVerifier;
};
