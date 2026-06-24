#include "api-client.hpp"
#include "oauth-callback-server.hpp"
#include <QDesktopServices>
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QEventLoop>
#include <QTimer>
#include <QDateTime>
#include <curl/curl.h>

const QString STREAMLABS_LOGIN_URL = QStringLiteral("https://streamlabs.com/slobs/login");
const QString STREAMLABS_API_BASE = QStringLiteral("https://streamlabs.com/api/v5/slobs");
const QString STREAMLABS_AUTH_DATA_URL = QStringLiteral("https://streamlabs.com/api/v5/slobs/auth/data");

const QString USER_AGENT = QStringLiteral(
	"Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
	"AppleWebKit/537.36 (KHTML, like Gecko) "
	"StreamlabsDesktop/1.20.4 Chrome/122.0.6261.156 "
	"Electron/29.3.1 Safari/537.36");

static size_t curlWriteCb(void *contents, size_t size, size_t nmemb, void *userp)
{
	QByteArray *buf = static_cast<QByteArray *>(userp);
	size_t total = size * nmemb;
	buf->append(static_cast<const char *>(contents), static_cast<int>(total));
	return total;
}

StreamlabsApiClient::StreamlabsApiClient(QObject *parent)
	: QObject(parent)
{
}

QString StreamlabsApiClient::generateRandomString(int length)
{
	static const char chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
	QString result;
	result.reserve(length);
	for (int i = 0; i < length; i++)
		result.append(chars[rand() % (sizeof(chars) - 1)]);
	return result;
}

QString StreamlabsApiClient::generateCodeVerifier()
{
	return generateRandomString(128);
}

QString StreamlabsApiClient::generateCodeChallenge(const QString &verifier)
{
	QByteArray hash = QCryptographicHash::hash(verifier.toUtf8(), QCryptographicHash::Sha256);
	return QString::fromLatin1(hash.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
}

void StreamlabsApiClient::startOAuthLogin()
{
}

StreamlabsApiClient::CurlResult StreamlabsApiClient::curlRequest(const QString &url,
								  const QByteArray &postData,
								  const QString &token,
								  const QString &contentType,
								  int timeoutMs)
{
	CurlResult result;

	CURL *curl = curl_easy_init();
	if (!curl) {
		result.error = QStringLiteral("curl_easy_init failed");
		return result;
	}

	QByteArray urlUtf8 = url.toUtf8();
	QByteArray userAgent = USER_AGENT.toUtf8();

	curl_easy_setopt(curl, CURLOPT_URL, urlUtf8.constData());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCb);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result.data);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, static_cast<long>(timeoutMs));
	curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent.constData());
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	struct curl_slist *headers = nullptr;
	headers = curl_slist_append(headers, "Accept: application/json");

	if (!token.isEmpty()) {
		QByteArray authHeader = "Authorization: Bearer " + token.toUtf8();
		headers = curl_slist_append(headers, authHeader.constData());
	}

	QByteArray ctUtf8;
	if (!contentType.isEmpty()) {
		ctUtf8 = contentType.toUtf8();
		QByteArray hdr = "Content-Type: " + ctUtf8;
		headers = curl_slist_append(headers, hdr.constData());
	}

	if (headers)
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

	if (!postData.isEmpty() || !contentType.isEmpty()) {
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		if (!postData.isEmpty()) {
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postData.constData());
			curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(postData.size()));
		}
	}

	CURLcode res = curl_easy_perform(curl);

	if (res != CURLE_OK) {
		result.error = QString::fromLatin1(curl_easy_strerror(res));
	} else {
		long httpCode = 0;
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
		result.statusCode = static_cast<int>(httpCode);
		result.ok = true;
	}

	curl_easy_cleanup(curl);
	if (headers)
		curl_slist_free_all(headers);

	return result;
}

QJsonObject StreamlabsApiClient::curlRequestJson(const QString &url, const QByteArray &postData,
						  const QString &token, const QString &contentType,
						  int timeoutMs)
{
	CurlResult r = curlRequest(url, postData, token, contentType, timeoutMs);
	m_lastHttpStatus = r.statusCode;

	if (!r.ok) {
		m_lastError = QStringLiteral("Request failed: %1 (HTTP %2)")
				      .arg(r.error)
				      .arg(r.statusCode);
		return QJsonObject();
	}

	if (r.statusCode < 200 || r.statusCode >= 300) {
		QString body = QString::fromUtf8(r.data).left(500);
		m_lastError = QStringLiteral("HTTP %1\n%2").arg(r.statusCode).arg(body);
		return QJsonObject();
	}

	QJsonDocument doc = QJsonDocument::fromJson(r.data);
	if (doc.isNull() || !doc.isObject()) {
		m_lastError = QStringLiteral("Invalid JSON response");
		return QJsonObject();
	}

	return doc.object();
}

QString StreamlabsApiClient::waitForToken(int timeoutMs)
{
	m_codeVerifier = generateCodeVerifier();
	QString codeChallenge = generateCodeChallenge(m_codeVerifier);

	OAuthCallbackServer callbackServer;

	if (!callbackServer.listen()) {
		m_lastError = QStringLiteral("Failed to start local callback server");
		return QString();
	}

	quint16 port = callbackServer.port();

	QUrl url(STREAMLABS_LOGIN_URL);
	QUrlQuery query;
	query.addQueryItem(QStringLiteral("skip_splash"), QStringLiteral("true"));
	query.addQueryItem(QStringLiteral("external"), QStringLiteral("electron"));
	query.addQueryItem(QStringLiteral("tiktok"), QString());
	query.addQueryItem(QStringLiteral("force_verify"), QStringLiteral("true"));
	query.addQueryItem(QStringLiteral("origin"), QStringLiteral("slobs"));
	query.addQueryItem(QStringLiteral("port"), QString::number(port));
	query.addQueryItem(QStringLiteral("code_challenge"), codeChallenge);
	query.addQueryItem(QStringLiteral("code_flow"), QStringLiteral("true"));
	url.setQuery(query);

	// Open browser for user to login
	if (!QDesktopServices::openUrl(url)) {
		m_lastError = QStringLiteral("Failed to open browser for login");
		return QString();
	}

	// Wait for callback using event loop (processes TCP connections)
	QEventLoop waitLoop;
	QTimer waitTimer;
	waitTimer.setSingleShot(true);
	connect(&callbackServer, &OAuthCallbackServer::codeReady, &waitLoop, &QEventLoop::quit);
	connect(&waitTimer, &QTimer::timeout, &waitLoop, &QEventLoop::quit);
	waitTimer.start(timeoutMs);
	waitLoop.exec();

	if (!callbackServer.codeReceived()) {
		m_lastError = QStringLiteral("Login timed out after %1 seconds").arg(timeoutMs / 1000);
		callbackServer.close();
		return QString();
	}

	QString authCode = callbackServer.authCode();
	callbackServer.close();

	if (authCode.isEmpty()) {
		m_lastError = QStringLiteral("No auth code received");
		return QString();
	}

	// Exchange auth code for token via curl GET with query params
	QByteArray encodedVerifier = QUrl::toPercentEncoding(m_codeVerifier);
	QByteArray encodedCode = QUrl::toPercentEncoding(authCode);
	QString exchangeUrl = STREAMLABS_AUTH_DATA_URL +
			      QStringLiteral("?code_verifier=%1&code=%2")
				      .arg(QString::fromLatin1(encodedVerifier),
					   QString::fromLatin1(encodedCode));

	CurlResult r = curlRequest(exchangeUrl, QByteArray(), QString(), QString(), 30000);

	if (!r.ok) {
		m_lastError = QStringLiteral("Token exchange failed: %1 (HTTP %2)")
				      .arg(r.error)
				      .arg(r.statusCode);
		return QString();
	}

	if (r.statusCode != 200) {
		QString body = QString::fromUtf8(r.data).left(500);
		m_lastError = QStringLiteral("Token exchange failed: HTTP %1\n%2")
				      .arg(r.statusCode).arg(body);
		return QString();
	}

	QJsonDocument doc = QJsonDocument::fromJson(r.data);
	if (doc.isNull() || !doc.isObject()) {
		m_lastError = QStringLiteral("Invalid JSON from token exchange");
		return QString();
	}

	QJsonObject obj = doc.object();
	if (!obj.value(QStringLiteral("success")).toBool()) {
		m_lastError = QStringLiteral("Streamlabs reported failure\n%1")
				      .arg(QString::fromUtf8(r.data).left(500));
		return QString();
	}

	QJsonObject data = obj.value(QStringLiteral("data")).toObject();
	QString token = data.value(QStringLiteral("oauth_token")).toString();

	if (token.isEmpty()) {
		m_lastError = QStringLiteral("No oauth_token in response");
		return QString();
	}

	return token;
}

StreamlabsAccountInfo StreamlabsApiClient::getAccountInfo(const QString &token)
{
	StreamlabsAccountInfo info;
	QString url = STREAMLABS_API_BASE + QStringLiteral("/tiktok/info");
	QJsonObject json = curlRequestJson(url, QByteArray(), token);

	if (json.isEmpty())
		return info;

	QJsonObject user = json.value(QStringLiteral("user")).toObject();
	info.username = user.value(QStringLiteral("username")).toString();
	info.canBeLive = json.value(QStringLiteral("can_be_live")).toBool();

	QJsonObject appStatus = json.value(QStringLiteral("application_status")).toObject();
	info.applicationStatus = appStatus.value(QStringLiteral("status")).toString();

	return info;
}

QVector<StreamlabsCategory> StreamlabsApiClient::searchCategories(const QString &token, const QString &query)
{
	QVector<StreamlabsCategory> results;

	if (query.isEmpty())
		return results;

	QString truncated = query.left(25);
	QString url = STREAMLABS_API_BASE + QStringLiteral("/tiktok/info?category=%1").arg(truncated);
	QJsonObject json = curlRequestJson(url, QByteArray(), token);

	if (json.isEmpty())
		return results;

	QJsonArray categories = json.value(QStringLiteral("categories")).toArray();
	for (const QJsonValue &val : categories) {
		QJsonObject cat = val.toObject();
		StreamlabsCategory sc;
		sc.fullName = cat.value(QStringLiteral("full_name")).toString();
		sc.gameMaskId = cat.value(QStringLiteral("game_mask_id")).toString();
		results.append(sc);
	}

	StreamlabsCategory other;
	other.fullName = QStringLiteral("Other");
	other.gameMaskId = QString();
	results.append(other);

	return results;
}

StreamlabsStreamInfo StreamlabsApiClient::startStream(const QString &token, const QString &title,
						       const QString &categoryId, const QString &audienceType)
{
	StreamlabsStreamInfo info;

	QString boundary = QStringLiteral("----WebKitFormBoundary7MA4YWxkTrZu0gW");

	QByteArray data;
	auto addField = [&](const QString &name, const QString &value) {
		data.append(QStringLiteral("--%1\r\n").arg(boundary).toUtf8());
		data.append(QStringLiteral("Content-Disposition: form-data; name=\"%1\"\r\n\r\n").arg(name).toUtf8());
		data.append(value.toUtf8());
		data.append(QStringLiteral("\r\n").toUtf8());
	};

	addField(QStringLiteral("title"), title);
	addField(QStringLiteral("device_platform"), QStringLiteral("win32"));
	addField(QStringLiteral("category"), categoryId);
	addField(QStringLiteral("audience_type"), audienceType);
	data.append(QStringLiteral("--%1--\r\n").arg(boundary).toUtf8());

	QString contentType = QStringLiteral("multipart/form-data; boundary=%1").arg(boundary);
	QString url = STREAMLABS_API_BASE + QStringLiteral("/tiktok/stream/start");

	QJsonObject json = curlRequestJson(url, data, token, contentType);

	if (json.isEmpty())
		return info;

	info.streamId = json.value(QStringLiteral("id")).toString();
	info.rtmpUrl = json.value(QStringLiteral("rtmp")).toString();
	info.streamKey = json.value(QStringLiteral("key")).toString();

	return info;
}

bool StreamlabsApiClient::endStream(const QString &token, const QString &streamId)
{
	if (streamId.isEmpty())
		return false;

	QString url = STREAMLABS_API_BASE + QStringLiteral("/tiktok/stream/%1/end").arg(streamId);
	QJsonObject json = curlRequestJson(url, QByteArray(), token,
					   QStringLiteral("application/x-www-form-urlencoded"));

	return json.value(QStringLiteral("success")).toBool();
}

// Async versions
void StreamlabsApiClient::getAccountInfoAsync(const QString &token,
					       std::function<void(StreamlabsAccountInfo, bool)> callback)
{
	QMetaObject::invokeMethod(this, [this, token, callback]() {
		auto info = getAccountInfo(token);
		callback(info, !info.username.isEmpty());
	});
}

void StreamlabsApiClient::searchCategoriesAsync(const QString &token, const QString &query,
						 std::function<void(QVector<StreamlabsCategory>, bool)> callback)
{
	QMetaObject::invokeMethod(this, [this, token, query, callback]() {
		auto results = searchCategories(token, query);
		callback(results, !results.isEmpty());
	});
}

void StreamlabsApiClient::startStreamAsync(const QString &token, const QString &title,
					   const QString &categoryId, const QString &audienceType,
					   std::function<void(StreamlabsStreamInfo, bool)> callback)
{
	QMetaObject::invokeMethod(this, [this, token, title, categoryId, audienceType, callback]() {
		auto info = startStream(token, title, categoryId, audienceType);
		callback(info, !info.streamId.isEmpty());
	});
}

void StreamlabsApiClient::endStreamAsync(const QString &token, const QString &streamId,
					 std::function<void(bool)> callback)
{
	QMetaObject::invokeMethod(this, [this, token, streamId, callback]() {
		bool success = endStream(token, streamId);
		callback(success);
	});
}
