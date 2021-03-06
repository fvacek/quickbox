#ifndef SPEAKERWIDGET_H
#define SPEAKERWIDGET_H

#include <QFrame>

#include <quickevent/gui/partwidget.h>

namespace Ui {
class SpeakerWidget;
}
namespace qf {
//namespace core { namespace model { class SqlTableModel; } }
namespace qmlwidgets {
class ForeignKeyComboBox;
}
}

namespace quickevent { namespace core { namespace og { class SqlTableModel; }}}
namespace quickevent { namespace core { namespace si { class PunchRecord; }}}

class ThisPartWidget;

class SpeakerWidget : public QFrame
{
	Q_OBJECT
private:
	typedef QFrame Super;
public:
	explicit SpeakerWidget(QWidget *parent = 0);
	~SpeakerWidget() Q_DECL_OVERRIDE;

	void settleDownInPartWidget(quickevent::gui::PartWidget *part_widget);

	void onDbEventNotify(const QString &domain, int connection_id, const QVariant &data);
	Q_SIGNAL void punchReceived(const quickevent::core::si::PunchRecord &punch);
private:
	//Q_SLOT void lazyInit();
	Q_SLOT void reset();
	Q_SLOT void reload();

	void updateTableView(int punch_id);

	void loadSettings();
	void saveSettings();

	bool isPartActive();

	void onCodeClassActivated(int class_id, int code);
private:
	Ui::SpeakerWidget *ui;
	quickevent::core::og::SqlTableModel *m_punchesModel = nullptr;
	qf::qmlwidgets::framework::PartWidget *m_partWidget = nullptr;
	bool m_resetRequest = false;
	bool m_settingsLoaded = false;
};

#endif // SPEAKERWIDGET_H
