#include "preferences.h"
#include "mainwindow.h"
#include "models.h"
#include "divelocationmodel.h"
#include "prefs-macros.h"
#include "qthelper.h"
#include "subsurfacestartup.h"

#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QShortcut>
#include <QNetworkProxy>
#include <QNetworkCookieJar>

#include "subsurfacewebservices.h"

#if !defined(Q_OS_ANDROID) && defined(FBSUPPORT)
#include "socialnetworks.h"
#include <QWebView>
#endif

PreferencesDialog *PreferencesDialog::instance()
{
	static PreferencesDialog *dialog = new PreferencesDialog(MainWindow::instance());
	return dialog;
}

PreferencesDialog::PreferencesDialog(QWidget *parent, Qt::WindowFlags f) : QDialog(parent, f)
{
	ui.setupUi(this);
	setAttribute(Qt::WA_QuitOnClose, false);

#if defined(Q_OS_ANDROID) || !defined(FBSUPPORT)
	for (int i = 0; i < ui.listWidget->count(); i++) {
		if (ui.listWidget->item(i)->text() == "Facebook") {
			delete ui.listWidget->item(i);
			QWidget *fbpage = ui.stackedWidget->widget(i);
			ui.stackedWidget->removeWidget(fbpage);
		}
	}
#endif

	ui.proxyType->clear();
	ui.proxyType->addItem(tr("No proxy"), QNetworkProxy::NoProxy);
	ui.proxyType->addItem(tr("System proxy"), QNetworkProxy::DefaultProxy);
	ui.proxyType->addItem(tr("HTTP proxy"), QNetworkProxy::HttpProxy);
	ui.proxyType->addItem(tr("SOCKS proxy"), QNetworkProxy::Socks5Proxy);
	ui.proxyType->setCurrentIndex(-1);

#if !defined(Q_OS_ANDROID) && defined(FBSUPPORT)
	FacebookManager *fb = FacebookManager::instance();
	facebookWebView = new QWebView(this);
	ui.fbWebviewContainer->layout()->addWidget(facebookWebView);
	if (fb->loggedIn()) {
		facebookLoggedIn();
	} else {
		facebookDisconnect();
	}
	connect(facebookWebView, &QWebView::urlChanged, fb, &FacebookManager::tryLogin);
	connect(fb, &FacebookManager::justLoggedIn, this, &PreferencesDialog::facebookLoggedIn);
	connect(ui.fbDisconnect, &QPushButton::clicked, fb, &FacebookManager::logout);
	connect(fb, &FacebookManager::justLoggedOut, this, &PreferencesDialog::facebookDisconnect);
#endif
	connect(ui.proxyType, SIGNAL(currentIndexChanged(int)), this, SLOT(proxyType_changed(int)));
	connect(ui.buttonBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(buttonClicked(QAbstractButton *)));

	//	connect(ui.defaultSetpoint, SIGNAL(valueChanged(double)), this, SLOT(defaultSetpointChanged(double)));
	QShortcut *close = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_W), this);
	connect(close, SIGNAL(activated()), this, SLOT(close()));
	QShortcut *quit = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q), this);
	connect(quit, SIGNAL(activated()), parent, SLOT(close()));
	loadSettings();
	setUiFromPrefs();
	rememberPrefs();
}

void PreferencesDialog::facebookLoggedIn()
{
#if !defined(Q_OS_ANDROID) && defined(FBSUPPORT)
	ui.fbDisconnect->show();
	ui.fbWebviewContainer->hide();
	ui.fbWebviewContainer->setEnabled(false);
	ui.FBLabel->setText(tr("To disconnect Subsurface from your Facebook account, use the button below"));
#endif
}

void PreferencesDialog::facebookDisconnect()
{
#if !defined(Q_OS_ANDROID) && defined(FBSUPPORT)
	// remove the connect/disconnect button
	// and instead add the login view
	ui.fbDisconnect->hide();
	ui.fbWebviewContainer->show();
	ui.fbWebviewContainer->setEnabled(true);
	ui.FBLabel->setText(tr("To connect to Facebook, please log in. This enables Subsurface to publish dives to your timeline"));
	if (facebookWebView) {
		facebookWebView->page()->networkAccessManager()->setCookieJar(new QNetworkCookieJar());
		facebookWebView->setUrl(FacebookManager::instance()->connectUrl());
	}
#endif
}

void PreferencesDialog::cloudPinNeeded()
{
	ui.cloud_storage_pin->setEnabled(prefs.cloud_verification_status == CS_NEED_TO_VERIFY);
	ui.cloud_storage_pin->setVisible(prefs.cloud_verification_status == CS_NEED_TO_VERIFY);
	ui.cloud_storage_pin_label->setEnabled(prefs.cloud_verification_status == CS_NEED_TO_VERIFY);
	ui.cloud_storage_pin_label->setVisible(prefs.cloud_verification_status == CS_NEED_TO_VERIFY);
	ui.cloud_storage_new_passwd->setEnabled(prefs.cloud_verification_status == CS_VERIFIED);
	ui.cloud_storage_new_passwd->setVisible(prefs.cloud_verification_status == CS_VERIFIED);
	ui.cloud_storage_new_passwd_label->setEnabled(prefs.cloud_verification_status == CS_VERIFIED);
	ui.cloud_storage_new_passwd_label->setVisible(prefs.cloud_verification_status == CS_VERIFIED);
	if (prefs.cloud_verification_status == CS_VERIFIED) {
		ui.cloudStorageGroupBox->setTitle(tr("Subsurface cloud storage (credentials verified)"));
	} else {
		ui.cloudStorageGroupBox->setTitle(tr("Subsurface cloud storage"));
	}
	MainWindow::instance()->enableDisableCloudActions();
}

void PreferencesDialog::showEvent(QShowEvent *event)
{
	setUiFromPrefs();
	rememberPrefs();
	QDialog::showEvent(event);
}

void PreferencesDialog::setUiFromPrefs()
{
	QSettings s;

	ui.save_uid_local->setChecked(s.value("save_uid_local").toBool());
	ui.default_uid->setText(s.value("subsurface_webservice_uid").toString().toUpper());

	ui.proxyHost->setText(prefs.proxy_host);
	ui.proxyPort->setValue(prefs.proxy_port);
	ui.proxyAuthRequired->setChecked(prefs.proxy_auth);
	ui.proxyUsername->setText(prefs.proxy_user);
	ui.proxyPassword->setText(prefs.proxy_pass);
	ui.proxyType->setCurrentIndex(ui.proxyType->findData(prefs.proxy_type));


	ui.cloud_storage_email->setText(prefs.cloud_storage_email);
	ui.cloud_storage_password->setText(prefs.cloud_storage_password);
	ui.save_password_local->setChecked(prefs.save_password_local);
	cloudPinNeeded();
	ui.cloud_background_sync->setChecked(prefs.cloud_background_sync);
}

void PreferencesDialog::restorePrefs()
{
	prefs = oldPrefs;
	setUiFromPrefs();
}

void PreferencesDialog::rememberPrefs()
{
	oldPrefs = prefs;
}

void PreferencesDialog::syncSettings()
{
	QSettings s;

	s.setValue("subsurface_webservice_uid", ui.default_uid->text().toUpper());
	set_save_userid_local(ui.save_uid_local->checkState());

	s.beginGroup("Network");
	s.setValue("proxy_type", ui.proxyType->itemData(ui.proxyType->currentIndex()).toInt());
	s.setValue("proxy_host", ui.proxyHost->text());
	s.setValue("proxy_port", ui.proxyPort->value());
	SB("proxy_auth", ui.proxyAuthRequired);
	s.setValue("proxy_user", ui.proxyUsername->text());
	s.setValue("proxy_pass", ui.proxyPassword->text());
	s.endGroup();

	s.beginGroup("CloudStorage");
	QString email = ui.cloud_storage_email->text();
	QString password = ui.cloud_storage_password->text();
	QString newpassword = ui.cloud_storage_new_passwd->text();
	if (prefs.cloud_verification_status == CS_VERIFIED && !newpassword.isEmpty()) {
		// deal with password change
		if (!email.isEmpty() && !password.isEmpty()) {
			// connect to backend server to check / create credentials
			QRegularExpression reg("^[a-zA-Z0-9@.+_-]+$");
			if (!reg.match(email).hasMatch() || (!password.isEmpty() && !reg.match(password).hasMatch())) {
				report_error(qPrintable(tr("Cloud storage email and password can only consist of letters, numbers, and '.', '-', '_', and '+'.")));
			} else {
				CloudStorageAuthenticate *cloudAuth = new CloudStorageAuthenticate(this);
				connect(cloudAuth, SIGNAL(finishedAuthenticate()), this, SLOT(cloudPinNeeded()));
				connect(cloudAuth, SIGNAL(passwordChangeSuccessful()), this, SLOT(passwordUpdateSuccessfull()));
				QNetworkReply *reply = cloudAuth->backend(email, password, "", newpassword);
				ui.cloud_storage_new_passwd->setText("");
				free(prefs.cloud_storage_newpassword);
				prefs.cloud_storage_newpassword = strdup(qPrintable(newpassword));
			}
		}
	} else if (prefs.cloud_verification_status == CS_UNKNOWN ||
		   prefs.cloud_verification_status == CS_INCORRECT_USER_PASSWD ||
		   email != prefs.cloud_storage_email ||
		   password != prefs.cloud_storage_password) {

		// different credentials - reset verification status
		prefs.cloud_verification_status = CS_UNKNOWN;
		if (!email.isEmpty() && !password.isEmpty()) {
			// connect to backend server to check / create credentials
			QRegularExpression reg("^[a-zA-Z0-9@.+_-]+$");
			if (!reg.match(email).hasMatch() || (!password.isEmpty() && !reg.match(password).hasMatch())) {
				report_error(qPrintable(tr("Cloud storage email and password can only consist of letters, numbers, and '.', '-', '_', and '+'.")));
			} else {
				CloudStorageAuthenticate *cloudAuth = new CloudStorageAuthenticate(this);
				connect(cloudAuth, SIGNAL(finishedAuthenticate()), this, SLOT(cloudPinNeeded()));
				QNetworkReply *reply = cloudAuth->backend(email, password);
			}
		}
	} else if (prefs.cloud_verification_status == CS_NEED_TO_VERIFY) {
		QString pin = ui.cloud_storage_pin->text();
		if (!pin.isEmpty()) {
			// connect to backend server to check / create credentials
			QRegularExpression reg("^[a-zA-Z0-9@.+_-]+$");
			if (!reg.match(email).hasMatch() || !reg.match(password).hasMatch()) {
				report_error(qPrintable(tr("Cloud storage email and password can only consist of letters, numbers, and '.', '-', '_', and '+'.")));
			}
			CloudStorageAuthenticate *cloudAuth = new CloudStorageAuthenticate(this);
			connect(cloudAuth, SIGNAL(finishedAuthenticate()), this, SLOT(cloudPinNeeded()));
			QNetworkReply *reply = cloudAuth->backend(email, password, pin);
		}
	}
	SAVE_OR_REMOVE("email", default_prefs.cloud_storage_email, email);
	SAVE_OR_REMOVE("save_password_local", default_prefs.save_password_local, ui.save_password_local->isChecked());
	if (ui.save_password_local->isChecked()) {
		SAVE_OR_REMOVE("password", default_prefs.cloud_storage_password, password);
	} else {
		s.remove("password");
		free(prefs.cloud_storage_password);
		prefs.cloud_storage_password = strdup(qPrintable(password));
	}
	SAVE_OR_REMOVE("cloud_verification_status", default_prefs.cloud_verification_status, prefs.cloud_verification_status);
	SAVE_OR_REMOVE("cloud_background_sync", default_prefs.cloud_background_sync, ui.cloud_background_sync->isChecked());

	// at this point we intentionally do not have a UI for changing this
	// it could go into some sort of "advanced setup" or something
	SAVE_OR_REMOVE("cloud_base_url", default_prefs.cloud_base_url, prefs.cloud_base_url);
	s.endGroup();

<<<<<<< HEAD
	s.beginGroup("geocoding");
#ifdef DISABLED
	s.setValue("enable_geocoding", ui.enable_geocoding->isChecked());
	s.setValue("parse_dive_without_gps", ui.parse_without_gps->isChecked());
	s.setValue("tag_existing_dives", ui.tag_existing_dives->isChecked());
#endif
	s.setValue("cat0", ui.first_item->currentIndex());
	s.setValue("cat1", ui.second_item->currentIndex());
	s.setValue("cat2", ui.third_item->currentIndex());
	s.endGroup();

=======
>>>>>>> Code Cleanup
	loadSettings();
	emit settingsChanged();
}

void PreferencesDialog::loadSettings()
{
	// This code was on the mainwindow, it should belong nowhere, but since we didn't
	// correctly fixed this code yet ( too much stuff on the code calling preferences )
	// force this here.
	loadPreferences();
	QSettings s;
	QVariant v;

	ui.save_uid_local->setChecked(s.value("save_uid_local").toBool());
	ui.default_uid->setText(s.value("subsurface_webservice_uid").toString().toUpper());
}

void PreferencesDialog::buttonClicked(QAbstractButton *button)
{
	switch (ui.buttonBox->standardButton(button)) {
	case QDialogButtonBox::Discard:
		restorePrefs();
		syncSettings();
		close();
		break;
	case QDialogButtonBox::Apply:
		syncSettings();
		break;
	case QDialogButtonBox::FirstButton:
		syncSettings();
		close();
		break;
	default:
		break; // ignore warnings.
	}
}
#undef SB

void PreferencesDialog::on_resetSettings_clicked()
{
	QSettings s;
	QMessageBox response(this);
	response.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
	response.setDefaultButton(QMessageBox::Cancel);
	response.setWindowTitle(tr("Warning"));
	response.setText(tr("If you click OK, all settings of Subsurface will be reset to their default values. This will be applied immediately."));
	response.setWindowModality(Qt::WindowModal);

	int result = response.exec();
	if (result == QMessageBox::Ok) {
		copy_prefs(&default_prefs, &prefs);
		setUiFromPrefs();
		Q_FOREACH (QString key, s.allKeys()) {
			s.remove(key);
		}
		syncSettings();
		close();
	}
}

void PreferencesDialog::passwordUpdateSuccessfull()
{
	ui.cloud_storage_password->setText(prefs.cloud_storage_password);
}

void PreferencesDialog::emitSettingsChanged()
{
	emit settingsChanged();
}

void PreferencesDialog::proxyType_changed(int idx)
{
	if (idx == -1) {
		return;
	}

	int proxyType = ui.proxyType->itemData(idx).toInt();
	bool hpEnabled = (proxyType == QNetworkProxy::Socks5Proxy || proxyType == QNetworkProxy::HttpProxy);
	ui.proxyHost->setEnabled(hpEnabled);
	ui.proxyPort->setEnabled(hpEnabled);
	ui.proxyAuthRequired->setEnabled(hpEnabled);
	ui.proxyUsername->setEnabled(hpEnabled & ui.proxyAuthRequired->isChecked());
	ui.proxyPassword->setEnabled(hpEnabled & ui.proxyAuthRequired->isChecked());
	ui.proxyAuthRequired->setChecked(ui.proxyAuthRequired->isChecked());
}
