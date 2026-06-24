#pragma once

#include <QDialog>
#include <QWidget>
#include <QStackedWidget>
#include <QToolButton>
#include <QComboBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QString>
#include <QLabel>
#include "obs-data.h"

class StreamlabsApiClient;

class StreamOutputDialog : public QDialog {
	Q_OBJECT
private:
	QWidget *WizardServicePage();

	QToolButton *selectionButton(std::string title, QIcon icon, int selectionStep);

	QWidget *WizardInfoKick(bool edit = false);
	QWidget *WizardInfoYouTube(bool edit = false);
	QWidget *WizardInfoTwitter(bool edit = false);
	QWidget *WizardInfoUnknown(bool edit = false);
	QWidget *WizardInfoTwitch(bool edit = false);
	QWidget *WizardInfoTrovo(bool edit = false);
	QWidget *WizardInfoTikTok(bool edit = false);
	QWidget *WizardInfoFacebook(bool edit = false);
	QWidget *WizardInfoStreamlabsTikTok(bool edit = false);

	// Platform icons
	QIcon platformIconTwitch = QIcon(":/aitum/media/twitch.png");
	QIcon platformIconYouTube = QIcon(":/aitum/media/youtube.png");
	QIcon platformIconKick = QIcon(":/aitum/media/kick.png");
	QIcon platformIconTikTok = QIcon(":/aitum/media/tiktok.png");
	QIcon platformIconTwitter = QIcon(":/aitum/media/twitter.png");
	QIcon platformIconTrovo = QIcon(":/aitum/media/trovo.png");
	QIcon platformIconFacebook = QIcon(":/aitum/media/facebook.png");
	QIcon platformIconUnknown = QIcon(":/aitum/media/unknown.png");
	QIcon platformIconStreamlabs = QIcon(":/aitum/media/tiktok.png");

	obs_data_array_t *servicesData;

	void resetOutputs();
	void acceptOutputs();
	void validateOutputs(QPushButton *confirmButton);

	// Generators
	QLineEdit *generateOutputNameField(std::string text, QPushButton *confirmButton, bool edit = false);
	QLineEdit *generateOutputServerField(QPushButton *confirmButton, bool locked, bool edit = false);
	QComboBox *generateOutputServerCombo(std::string service, QPushButton *confirmButton, bool edit = false);
	QLineEdit *generateOutputKeyField(QPushButton *confirmButton, bool edit = false);
	QHBoxLayout *generateWizardButtonLayout(QPushButton *confirmButton, QPushButton *serviceButton, bool edit);
	QPushButton *generateBackButton();

	obs_data_t *getService(std::string serviceName);

	QStackedWidget *stackedWidget;
	QStringList otherNames;

	// Streamlabs API client (lazily created)
	StreamlabsApiClient *m_streamlabsClient = nullptr;
	StreamlabsApiClient *streamlabsClient();

	// Streamlabs auth state
	QString m_pendingToken;

private slots:
	void onStreamlabsLoginClicked(QPushButton *loginBtn, QPushButton *confirmBtn,
				      QLabel *usernameLabel, QLabel *statusLabel,
				      QLineEdit *titleField, QComboBox *categoryCombo,
				      QStackedWidget *stack);

public:
	StreamOutputDialog(QDialog *parent, QStringList otherNames);
	StreamOutputDialog(QDialog *parent, QString name, QString server, QString key, QStringList otherNames,
			   const QString &keySource = QString(), const QString &token = QString(),
			   const QString &title = QString(), const QString &gameMaskId = QString(),
			   const QString &audienceType = QString());
	~StreamOutputDialog();

	QString outputName;
	QString outputServer;
	QString outputKey;
	QString outputKeySource;
	QString outputStreamlabsToken;
	QString outputTitle;
	QString outputGameMaskId;
	QString outputAudienceType;
};
