#pragma once

#include <QDialog>
#include <QString>
#include <QPointer>

class CefBrowserDialog : public QDialog {
	Q_OBJECT
public:
	explicit CefBrowserDialog(const QString &url, QWidget *parent = nullptr);
	~CefBrowserDialog();

	bool isCefAvailable() const { return m_cefAvailable; }

protected:
	void showEvent(QShowEvent *event) override;

private:
	bool m_cefAvailable = false;
	QPointer<QWidget> m_browserWidget;
};
