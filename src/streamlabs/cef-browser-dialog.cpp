#include "cef-browser-dialog.hpp"
#include "../docks/browser-panel.hpp"
#include <QVBoxLayout>
#include <QLabel>
#include <QShowEvent>
#include <QDesktopServices>
#include <QUrl>

CefBrowserDialog::CefBrowserDialog(const QString &url, QWidget *parent)
	: QDialog(parent)
{
	setWindowTitle(QStringLiteral("Streamlabs Login"));
	resize(900, 700);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	QCef *cef = obs_browser_init_panel();
	if (cef) {
		m_cefAvailable = true;
		m_browserWidget = cef->create_widget(this, url.toStdString(),
						     nullptr);
		if (m_browserWidget)
			layout->addWidget(m_browserWidget);
	} else {
		auto *label = new QLabel(
			QStringLiteral(
				"Opening browser for Streamlabs login..."),
			this);
		label->setAlignment(Qt::AlignCenter);
		layout->addWidget(label);
		QDesktopServices::openUrl(QUrl(url));
	}

	setLayout(layout);
}

CefBrowserDialog::~CefBrowserDialog() = default;

void CefBrowserDialog::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);
	if (m_browserWidget)
		m_browserWidget->setFocus();
}
