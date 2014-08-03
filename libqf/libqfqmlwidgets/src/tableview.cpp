#include "tableview.h"
#include "headerview.h"
#include "action.h"
#include "tableitemdelegate.h"
#include "dialogs/messagebox.h"

#include <qf/core/string.h>
#include <qf/core/collator.h>
#include <qf/core/log.h>
#include <qf/core/assert.h>
#include <qf/core/exception.h>

#include <QKeyEvent>
#include <QMenu>
#include <QAbstractButton>
#include <QSettings>
#include <QJsonDocument>

namespace qfc = qf::core;
namespace qfu = qf::core::utils;
using namespace qf::qmlwidgets;

TableView::TableView(QWidget *parent) :
	Super(parent), framework::IPersistentSettings(this)
{
	setItemDelegate(new TableItemDelegate(this));
	{
		HeaderView *h = new HeaderView(Qt::Horizontal, this);
		setHorizontalHeader(h);
		connect(this, &TableView::searchStringChanged, h, &HeaderView::setSearchString);
	}
	{
		HeaderView *h = new HeaderView(Qt::Vertical, this);
		setVerticalHeader(h);
	}
	setSortingEnabled(true);

	createActions();
	{
		/// top left corner actions
		foreach(QAbstractButton *bt, findChildren<QAbstractButton*>()) {
			if(bt->metaObject()->className() == QString("QTableCornerButton")) { /// src/gui/itemviews/qtableview.cpp:103
				//qfInfo() << "addidng actions";
				bt->setText("M");
				bt->setToolTip(trUtf8("Right click for menu."));
				bt->setContextMenuPolicy(Qt::ActionsContextMenu);
				QList<QAction*> lst;
				for(auto a : contextMenuActionsForGroups(AllActions))
					lst << a;
				bt->addActions(lst);
			};
		}
	}
}

TableView::~TableView()
{
	savePersistentSettings();
}

qf::core::model::TableModel *TableView::tableModel() const
{
	return qobject_cast<qf::core::model::TableModel*>(Super::model());
}

void TableView::setTableModel(core::model::TableModel *m)
{
	qf::core::model::TableModel *old_m = tableModel();
	if (old_m != m) {
		Super::setModel(m);
		connect(m, &core::model::TableModel::columnsAutoGenerated, this, &TableView::loadPersistentSettings, Qt::QueuedConnection);
		emit modelChanged();
	}
}

void TableView::refreshActions()
{
	qfLogFuncFrame() << "model:" << model();
	enableAllActions(false);
	qfc::model::TableModel *m = tableModel();
	if(!m)
		return;
	//action("calculate")->setEnabled(true);
	action("copy")->setEnabled(true);
	//action("copySpecial")->setEnabled(true);
	//action("select")->setEnabled(true);
	action("reload")->setEnabled(true);
	action("resizeColumnsToContents")->setEnabled(true);
	//action("showCurrentCellText")->setEnabled(true);
	//action("saveCurrentCellBlob")->setEnabled(true);
	//action("loadCurrentCellBlob")->setEnabled(true);
	//action("insertRowsStatement")->setEnabled(true);
	//action("import")->setEnabled(true);
	//action("importCSV")->setEnabled(true);
	//action("export")->setEnabled(true);
	//action("exportReport")->setEnabled(true);
	//action("exportCSV")->setEnabled(true);
	//action("exportXML")->setEnabled(true);
	//action("exportXLS")->setEnabled(true);
	//action("exportHTML")->setEnabled(true);

	//action("insertRow")->setVisible(isInsertRowActionVisible());
	//action("removeSelectedRows")->setVisible(isRemoveRowActionVisibleInExternalMode());
	//action("postRow")->setVisible(true);
	//action("revertRow")->setVisible(true);

	//action("viewRowExternal")->setVisible(true);
	//action("editRowExternal")->setVisible(true);

	bool is_insert_rows_allowed = true;//m->isInsertRowsAllowed() && !isReadOnly();
	bool is_edit_rows_allowed = true;//m->isEditRowsAllowed() && !isReadOnly();
	bool is_delete_rows_allowed = true;//m->rowCount()>0 && m->isDeleteRowsAllowed() && !isReadOnly();
	bool is_copy_rows_allowed = true;//m->rowCount()>0 && is_insert_rows_allowed;
	//qfInfo() << "\tinsert allowed:" << is_insert_rows_allowed;
	//qfTrash() << "\tdelete allowed:" << is_delete_rows_allowed;
	//qfTrash() << "\tedit allowed:" << is_edit_rows_allowed;
	//action("insertRow")->setVisible(is_insert_rows_allowed && isInsertRowActionVisible());
	action("copyRow")->setEnabled(is_copy_rows_allowed);
	//action("copyRow")->setVisible(isCopyRowActionVisible());
	action("removeSelectedRows")->setEnabled(is_delete_rows_allowed);// && action("removeSelectedRows")->isVisible());
	//action("postRow")->setVisible((is_edit_rows_allowed || is_insert_rows_allowed) && action("postRow")->isVisible());
	//action("revertRow")->setVisible(action("postRow")->isVisible() && action("revertRow")->isVisible());
	//action("editRowExternal")->setVisible(is_edit_rows_allowed && action("editRowExternal")->isVisible());

	QModelIndex curr_ix = currentIndex();
	qfu::TableRow curr_row;
	if(curr_ix.isValid())
		curr_row = m->tableRow(curr_ix.row());
	//qfTrash() << QF_FUNC_NAME << "valid:" << r.isValid() << "dirty:" << r.isDirty();
	if(curr_row.isDirty()) {
		action("postRow")->setEnabled(true);
		action("revertRow")->setEnabled(true);
	}
	else {
		//action("insertRow")->setEnabled(isInsertRowActionVisible());
		action("copyRow")->setEnabled(is_copy_rows_allowed && curr_ix.isValid());
		action("reload")->setEnabled(true);
		action("sortAsc")->setEnabled(true);
		action("sortDesc")->setEnabled(true);
		//action("filter")->setEnabled(true);
		//action("addColumnFilter")->setEnabled(true);
		//action("removeColumnFilter")->setEnabled(true);
		//action("deleteColumnFilters")->setEnabled(true);
		action("setValueInSelection")->setEnabled(true);
		action("setNullInSelection")->setEnabled(true);
		action("generateSequenceInSelection")->setEnabled(true);
		action("paste")->setEnabled(is_edit_rows_allowed && is_insert_rows_allowed);
	}
	action("revertRow")->setEnabled(action("postRow")->isEnabled());
}

void TableView::reload()
{
	qfLogFuncFrame();
	if(horizontalHeader()) {
		savePersistentSettings();
		horizontalHeader()->setSortIndicator(-1, Qt::AscendingOrder);
	}
	qf::core::model::TableModel *table_model = tableModel();
	if(table_model) {
		QModelIndex ix = currentIndex();
		table_model->reload();
		//qfTrash() << "\t emitting reloaded()";
		//emit reloaded();
		//qfTrash() << "\ttable:" << table();
		setCurrentIndex(ix);
		//updateDataArea();
	}
	refreshActions();
}

void TableView::enableAllActions(bool on)
{
	for(auto a : m_actions) {
		a->setEnabled(on);
		//if(on) a->setVisible(true);
	}
}

bool TableView::postRow(int row_no)
{
	qfLogFuncFrame() << row_no;
	bool ret = false;
	qfc::model::TableModel *m = tableModel();
	if(m) {
		ret = m->postRow(row_no, true);
	}
	return ret;
}

void TableView::revertRow(int row_no)
{
	qfLogFuncFrame() << row_no;
	qfc::model::TableModel *m = tableModel();
	if(m) {
		m->revertRow(row_no);
	}
}

void TableView::updateRow(int row)
{
	QModelIndex ix = currentIndex();
	if(row < 0) row = ix.row();
	if(row < 0) return;
	if(ix.row() != row)
		ix = ix.sibling(row, ix.column());
	QRect r = visualRect(ix);
	// expand rect to whole row
	if(r.isEmpty()) return;
	r.setX(0);
	r.setWidth(width());
	viewport()->update(r);
	// update header
	QHeaderView *vh = verticalHeader();
	if(vh) {
		r = QRect(0, vh->sectionViewportPosition(ix.row()), vh->viewport()->width(), vh->sectionSize(ix.row()));
		verticalHeader()->viewport()->update(r);
	}
}

qf::core::utils::Table::SortDef TableView::seekSortDefinition() const
{
	qfLogFuncFrame();
	qf::core::utils::Table::SortDef ret;
	if(tableModel()) {
		ret = tableModel()->table().tableProperties().sortDefinition().value(0);
	}
	return ret;
}

int TableView::seekColumn() const
{
	int ret = -1;
	QHeaderView *h = horizontalHeader();
	if(h) {
		if(h->isSortIndicatorShown() && h->sortIndicatorOrder() == Qt::AscendingOrder) {
			ret = h->sortIndicatorSection();
		}
	}
	return ret;
}

void TableView::seek(const QString &prefix_str)
{
	qfLogFuncFrame() << prefix_str;
	if(prefix_str.isEmpty())
		return;
	if(!model())
		return;
	int col = seekColumn();
	if(col >= 0) {
		qf::core::Collator sort_collator = tableModel()->table().sortCollator();
		sort_collator.setCaseSensitivity(Qt::CaseInsensitive);
		sort_collator.setIgnorePunctuation(true);
		//qfWarning() << sort_collator.compare(QString::fromUtf8("s"), QString::fromUtf8("š")) << QString::fromUtf8("š").toUpper();
		//qfWarning() << "collator CS:" << (sort_collator.caseSensitivity() == Qt::CaseSensitive);
		for(int i=0; i<model()->rowCount(); i++) {
			QModelIndex ix = model()->index(i, col, QModelIndex());
			QString data_str = model()->data(ix, Qt::DisplayRole).toString();//.mid(0, prefix_str.length()).toLower();
			/// QTBUG-37689 QCollator allways sorts case sensitive
			/// workarounded by own implementation of qf::core::Collator
			QStringRef ps(&prefix_str);
			QStringRef ds(&data_str, 0, prefix_str.length());
			//QString ps = prefix_str.toLower();
			//QString ds = data_str.mid(0, ps.length()).toLower();
			int cmp = sort_collator.compare(ps, ds);
			//qfInfo() << ps << "cmp" << ds << "->" << cmp;
			if(cmp <= 0) {
				setCurrentIndex(ix);
				break;
			}
		}
	}
}

void TableView::cancelSeek()
{
	if(!m_seekString.isEmpty()) {
		m_seekString = QString();
		emit searchStringChanged(m_seekString);
	}
}

void TableView::loadPersistentSettings()
{
	QString path = persistentSettingsPath();
	qfLogFuncFrame() << path;
	if(!path.isEmpty()) {
		HeaderView *horiz_header = qobject_cast<HeaderView*>(horizontalHeader());
		if(!horiz_header || horiz_header->count() == 0)
			return;
		qf::core::model::TableModel *mod = tableModel();
		if(!mod || mod->columnCount() == 0)
			return;

		QSettings settings;
		settings.beginGroup(path);

		QString s = settings.value("horizontalheader").toString();
		QJsonDocument jd = QJsonDocument::fromJson(s.toUtf8());
		QVariantMap m;
		m = jd.toVariant().toMap();
		QVariantMap sections = m.value("sections").toMap();
		QMap<int, QString> visual_order;
		{
			QMapIterator<QString, QVariant> it(sections);
			while(it.hasNext()) {
				it.next();
				QVariantMap section = it.value().toMap();
				QString field_name = it.key();
				int visual_ix = section.value("visualIndex").toInt();
				visual_order[visual_ix] = field_name;
				for(int logical_ix=0; logical_ix<horiz_header->count(); logical_ix++) {
					QString col_name = mod->headerData(logical_ix, horiz_header->orientation(), qf::core::model::TableModel::FieldNameRole).toString();
					qfDebug() << col_name << "cmp" << field_name << "=" << qf::core::Utils::fieldNameCmp(col_name, field_name);
					if(qf::core::Utils::fieldNameCmp(col_name, field_name)) {
						int size = section.value("size").toInt();
						horiz_header->resizeSection(logical_ix, size);
						break;
					}
				}
			}
		}
		{
			// block signals for each particular horiz_header->moveSection(v_ix, visual_ix) call
			// Qt5 crashes sometimes when more sections are moved
			/* backtrace
			0	QScopedPointer<QObjectData, QScopedPointerDeleter<QObjectData> >::data	qscopedpointer.h	143	0x7ffff6810a8c
			1	qGetPtrHelper<QScopedPointer<QObjectData, QScopedPointerDeleter<QObjectData> > >	qglobal.h	941	0x7ffff6ccaa85
			2	QGraphicsEffect::d_func	qgraphicseffect.h	112	0x7ffff6ccab1c
			3	QGraphicsEffect::source	qgraphicseffect.cpp	514	0x7ffff6cc8ae5
			4	QWidgetPrivate::invalidateGraphicsEffectsRecursively	qwidget.cpp	1849	0x7ffff6862d97
			5	QWidgetPrivate::setDirtyOpaqueRegion	qwidget.cpp	1865	0x7ffff685d471
			6	QWidget::setVisible	qwidget.cpp	7370	0x7ffff687209d
			7	QAbstractScrollAreaPrivate::layoutChildren	qabstractscrollarea.cpp	523	0x7ffff6a712be
			8	QAbstractScrollArea::setViewportMargins	qabstractscrollarea.cpp	940	0x7ffff6a72123
			9	QTableView::updateGeometries	qtableview.cpp	2114	0x7ffff6b6791a
			10	QTableView::columnMoved	qtableview.cpp	2956	0x7ffff6b6aeef
			11	QTableView::qt_static_metacall	moc_qtableview.cpp	189	0x7ffff6b6ca0e
			12	QMetaObject::activate	qobject.cpp	3680	0x7ffff55e62e9
			13	QMetaObject::activate	qobject.cpp	3546	0x7ffff55e576d
			14	QHeaderView::sectionMoved	moc_qheaderview.cpp	375	0x7ffff6b38689
			15	QHeaderView::moveSection	qheaderview.cpp	798	0x7ffff6b382f4
			16	qf::qmlwidgets::TableView::loadPersistentSettings	tableview.cpp	186	0x7ffff78e3664
			 */
			horiz_header->blockSignals(true);
			QMapIterator<int, QString> it(visual_order);
			it.toBack();
			while(it.hasPrevious()) {
				it.previous();
				int visual_ix = it.key();
				QString field_name = it.value();
				qfDebug() << "moving column:" << field_name << "to visual index:" << visual_ix;
				for(int v_ix=0; v_ix<visual_ix; v_ix++) {
					int log_ix = horiz_header->logicalIndex(v_ix);
					QF_ASSERT(log_ix >= 0, "internal error", continue);
					QString col_name = mod->headerData(log_ix, horiz_header->orientation(), qf::core::model::TableModel::FieldNameRole).toString();
					qfDebug() << "\tvisual index:" << v_ix << "-> logical index:" << log_ix << "col name:" << col_name;
					if(qf::core::Utils::fieldNameCmp(col_name, field_name)) {
						qfDebug() << "\t\tmoving:" << v_ix << "->" << visual_ix;
						if(v_ix != visual_ix)
							horiz_header->moveSection(v_ix, visual_ix);
						break;
					}
				}
			}
			horiz_header->blockSignals(false);
		}
	}
}

void TableView::savePersistentSettings()
{
	QString path = persistentSettingsPath();
	qfLogFuncFrame() << path;
	if(!path.isEmpty()) {
		QSettings settings;
		settings.beginGroup(path);
		HeaderView *horiz_header = qobject_cast<HeaderView*>(horizontalHeader());
		qf::core::model::TableModel *mod = tableModel();
		if(horiz_header && mod) {
			QVariantMap sections;
			for(int i=0; i<horiz_header->count() && i<mod->columnCount(); i++) {
				QString col_name = mod->headerData(i, horiz_header->orientation(), qf::core::model::TableModel::FieldNameRole).toString();
				if(!col_name.isEmpty()) {
					/// remove schema name from column
					QString fn, tn;
					qf::core::Utils::parseFieldName(col_name, &fn, &tn);
					col_name = fn;
					if(!tn.isEmpty())
						col_name = tn + '.' + col_name;
					QVariantMap section;
					//section["fieldName"] = col_name;
					section["size"] = horiz_header->sectionSize(i);
					section["visualIndex"] = horiz_header->visualIndex(i);
					sections[col_name] = section;
				}
			}
			QVariantMap m;
			m["sections"] = sections;
			QJsonDocument jd = QJsonDocument::fromVariant(m);
			settings.setValue("horizontalheader", QString::fromUtf8(jd.toJson(QJsonDocument::Compact)));
		}
	}
}

void TableView::keyPressEvent(QKeyEvent *e)
{
	qfLogFuncFrame() << "key:" << e->key() << "modifiers:" << e->modifiers();
	if(!model()) {
		e->ignore();
		return;
	}

	bool incremental_search = false;
	bool incremental_search_key_accepted = false;
	qfc::String old_seek_string = m_seekString;
	//bool modified = (e->modifiers() != Qt::NoModifier && e->modifiers() != Qt::KeypadModifier);
	bool key_enter = (e->key() == Qt::Key_Return && e->modifiers() == 0)
			|| (e->key() == Qt::Key_Enter && e->modifiers() == Qt::KeypadModifier);
	if(e->modifiers() == Qt::ControlModifier) {
		if(e->key() == Qt::Key_C) {
			//copy();
			//e->accept();
			//return;
		}
		else if(e->key() == Qt::Key_V) {
			//paste();
			//e->accept();
			//return;
		}
		else if(e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return) {
			qfDebug() << "\tCTRL+ENTER";
			////postRow();
			e->accept();
			return;
		}
	}
	else if(key_enter) {
		qfDebug() << "\t ENTER pressed";
	}
	else {
		qfu::Table::SortDef sd = seekSortDefinition();
		if(sd.isValid() && sd.ascending && seekColumn() >= 0 && (currentIndex().column() == seekColumn() || !currentIndex().isValid())) {
			incremental_search = true;
			if(!currentIndex().isValid())
				setCurrentIndex(model()->index(0, seekColumn(), QModelIndex()));
			//qfInfo() << "incremental search currentIndex row:" << currentIndex().row() << "col:" << currentIndex().column();
			/// Pokud je nektery sloupec serazen vzestupne zkusi se provest incremental search,
			/// pak se event dal nepropaguje
			QChar seekChar = qfc::String(e->text()).value(0);
			//bool is_valid_seek_char = true;
			if(e->key() == Qt::Key_Home
					|| e->key() == Qt::Key_End
					|| e->key() == Qt::Key_Left
					|| e->key() == Qt::Key_Up
					|| e->key() == Qt::Key_Right
					|| e->key() == Qt::Key_Down
					|| e->key() == Qt::Key_PageUp
					|| e->key() == Qt::Key_PageDown) {
				incremental_search = false;
				seekChar = QChar();
			}
			else if(seekChar == '\n' || seekChar == '\r')
				seekChar = QChar();
			qfDebug().nospace() << "\tincremental search seekChar unicode: 0x" << QString::number(seekChar.unicode(),16) << " key: 0x" << QString::number(e->key(),16);
			bool shift_only = (e->key() == Qt::Key_Shift);
			//bool accept = false;
			if(incremental_search) {
				if(e->key() == Qt::Key_Backspace) {
					m_seekString = old_seek_string.slice(0, -1);
					incremental_search_key_accepted = true;
				}
				else if(e->key() == Qt::Key_Escape) {
					m_seekString = QString();
					incremental_search_key_accepted = true;
				}
				else if(seekChar.isNull() && !shift_only) {
					m_seekString = QString();
				}
				else {
					m_seekString += seekChar;
					qfDebug() << "new seek text:" << m_seekString;
					incremental_search_key_accepted = true;
				}
				if(!m_seekString.isEmpty()) {
					if(m_seekString != old_seek_string) {
						seek(m_seekString);
					}
					incremental_search_key_accepted = true;
				}
			}
		}
	}
	if(m_seekString != old_seek_string)
		emit searchStringChanged(m_seekString);
	if(incremental_search && incremental_search_key_accepted) {
		qfDebug() << "\tUSED for incremental search";
		e->accept();
		return;
	}
	else {
		cancelSeek();
	}
	bool event_should_be_accepted = false;
	/// nejedna se o inkrementalni vyhledavani, zkusime editaci
	if(state() == EditingState) {
		qfDebug() << "\teditor exists";
		//QModelIndex current = currentIndex();
		//QModelIndex newCurrent;
		/// cursor keys moves selection, check editor data before
		/// some of switched keys shoul be filtered by editor
		qfDebug() << "\tHAVE EDITOR key:" << e->key();
		switch (e->key()) {
		case Qt::Key_Down:
		case Qt::Key_Up:
		case Qt::Key_Left:
		case Qt::Key_Right:
		case Qt::Key_Home:
		case Qt::Key_End:
		case Qt::Key_PageUp:
		case Qt::Key_PageDown:
		case Qt::Key_Tab:
		case Qt::Key_Backtab:
		case Qt::Key_Return:
		case Qt::Key_Enter:
			qfDebug() << "accepting event";
			/// je to trochu jedno, protoze to vypada, ze accept flag, kterej prijde dialogu je ten, jak ho nastavi editor (widget dodany delegatem) ve sve funkci keyPressEvent(...)
			//e->accept();
			event_should_be_accepted = true;
			/*****
				QFItemDelegate *it = qobject_cast<QFItemDelegate*>(itemDelegate());
				if(it) {
					if(!it->canCloseEditor()) return;
				}
				*/
			//qfDebug().color(QFLog::Yellow) << "set focus to table view";
			setFocus(); /// jinak se mi zavre delegat a focus skoci na jinej widget
			break;
		}
	}
	else {
		if(key_enter) {
			///timhle jsem zajistil, ze se editor otevira na enter a ne jen na F2, jak je defaultne v QT
			/// viz: QAbstractItemView::keyPressEvent(...)
			qfDebug() << "\tkey chci otevrit editor, state:" << state();
			if(edit(currentIndex(), EditKeyPressed, e)) {
				qfDebug() << "accepting event";
				e->accept();
			}
			else {
				qfDebug() << "ignoring event";
				e->ignore();
			}
			//qfDebug() << "accepting event";
			//e->accept();
			qfDebug() << "\t exiting after open editor try, event accepted:" << e->isAccepted() << "event:" << e;
			return;
		}
	}
	qfDebug() << "\tcalling parent implementation QTableView::keyPressEvent(e), state:" << state() << "event accepted:" << e->isAccepted();
	QTableView::keyPressEvent(e);
	/// parent implementace muze zmenit accepted() flag eventu
	qfDebug() << "\tcalled parent implementation QTableView::keyPressEvent(e), state:" << state() << "event accepted:" << e->isAccepted();
	if(event_should_be_accepted) e->accept();
}

void TableView::mousePressEvent(QMouseEvent * e)
{
	qfLogFuncFrame();

	cancelSeek();
	QPoint pos = e->pos();
	QModelIndex ix = indexAt(pos);
	/*
	qfTrash() << "\trow:" << ix.row() << "col:" << ix.column();
	QFItemDelegate *it = qobject_cast<QFItemDelegate*>(itemDelegate());
	if(it) {
		//qfTrash() << "\teditor" << w;
		// pokud existuje editor, je pole rozeditovany a melo by se zkontrolovat
		if(!it->canCloseEditor()) return;
	}
	*/
	Super::mousePressEvent(e);
	if(ix.isValid() && currentIndex() != ix) {
		/// pokud je bunka typu bool a kliknu primo na zaskrtavatko, defaultni implementace nezpusobi, ze se tam presune i currentIndex()
		/// ja to ale potrebuju, aby se nova hodnota ulozila pri currentChanged(), tak si to takhle delam sam
		setCurrentIndex(ix);
	}
}

void TableView::createActions()
{
	Action *a;
	{
		a = new Action(tr("Resize columns to contents"), this);
		//a->setShortcut(QKeySequence(tr("Ctrl+R", "reload SQL table")));
		//a->setShortcutContext(Qt::WidgetShortcut);
		a->setOid("resizeColumnsToContents");
		m_actionGroups[SizeActions] << a->oid();
		m_actions[a->oid()] = a;
		connect(a, &Action::triggered, this, &TableView::resizeColumnsToContents);
	}
	{
		a = new Action(tr("Reload"), this);
		a->setIcon(QIcon(":/qf/qmlwidgets/images/reload.png"));
		a->setShortcut(QKeySequence(tr("Ctrl+R", "reload SQL table")));
		a->setShortcutContext(Qt::WidgetShortcut);
		a->setOid("reload");
		m_actionGroups[ViewActions] << a->oid();
		m_actions[a->oid()] = a;
		connect(a, SIGNAL(triggered()), this, SLOT(reload()));
	}
	{
		a = new Action(tr("Copy"), this);
		a->setIcon(QIcon(":/qf/qmlwidgets/images/copy.png"));
		a->setShortcut(QKeySequence(tr("Ctrl+C", "Copy selection")));
		a->setShortcutContext(Qt::WidgetShortcut);
		a->setOid("copy");
		m_actionGroups[ViewActions] << a->oid();
		m_actions[a->oid()] = a;
		//connect(a, SIGNAL(triggered()), this, SLOT(copy()));
	}
	{
		a = new Action(tr("Copy special"), this);
		a->setIcon(QIcon(":/qf/qmlwidgets/images/copy.png"));
		//a->setShortcut(QKeySequence(tr("Ctrl+C", "Copy selection")));
		//a->setShortcutContext(Qt::WidgetShortcut);
		a->setOid("copySpecial");
		m_actionGroups[ViewActions] << a->oid();
		m_actions[a->oid()] = a;
		//connect(a, SIGNAL(triggered()), this, SLOT(copySpecial()));
	}
	{
		a = new Action(tr("Paste"), this);
		a->setIcon(QIcon(":/qf/qmlwidgets/images/paste.png"));
		a->setShortcut(QKeySequence(tr("Ctrl+V", "Paste rows")));
		a->setShortcutContext(Qt::WidgetShortcut);
		a->setOid("paste");
		m_actionGroups[PasteActions] << a->oid();
		m_actions[a->oid()] = a;
		//connect(a, SIGNAL(triggered()), this, SLOT(paste()), Qt::QueuedConnection); /// hazelo mi to vyjjimky v evendloopu
	}
	{
		a = new Action(QIcon(":/qf/qmlwidgets/images/new.png"), tr("Insert row"), this);
		a->setShortcut(QKeySequence(tr("Ctrl+Ins", "insert row SQL table")));
		a->setShortcutContext(Qt::WidgetShortcut);
		a->setOid("insertRow");
		m_actionGroups[RowActions] << a->oid();
		m_actions[a->oid()] = a;
		//connect(a, SIGNAL(triggered()), this, SLOT(insertRow()));
	}
	{
		a = new Action(QIcon(":/qf/qmlwidgets/images/delete.png"), tr("Delete selected rows"), this);
		a->setShortcut(QKeySequence(tr("Ctrl+Del", "delete row SQL table")));
		a->setShortcutContext(Qt::WidgetShortcut);
		a->setOid("removeSelectedRows");
		m_actionGroups[RowActions] << a->oid();
		m_actions[a->oid()] = a;
		//connect(a, SIGNAL(triggered()), this, SLOT(removeSelectedRows()));
	}
	{
		a = new Action(tr("Post row edits"), this);
		a->setIcon(QIcon(":/qf/qmlwidgets/images/sql_post.png"));
		a->setShortcut(QKeySequence(tr("Ctrl+Return", "post row SQL table")));
		a->setShortcutContext(Qt::WidgetShortcut);
		a->setOid("postRow");
		m_actionGroups[RowActions] << a->oid();
		m_actions[a->oid()] = a;
		connect(a, SIGNAL(triggered()), this, SLOT(postRow()));
	}
	{
		a = new Action(tr("Revert row edits"), this);
		a->setIcon(QIcon(":/qf/qmlwidgets/images/revert.png"));
		a->setShortcut(QKeySequence(tr("Ctrl+Z", "revert edited row")));
		a->setShortcutContext(Qt::WidgetShortcut);
		a->setOid("revertRow");
		m_actionGroups[RowActions] << a->oid();
		m_actions[a->oid()] = a;
		connect(a, SIGNAL(triggered()), this, SLOT(revertRow()));
	}
	{
		a = new Action(tr("Copy row"), this);
		a->setIcon(QIcon(":/qf/qmlwidgets/images/clone.png"));
		a->setOid("copyRow");
		a->setVisible(false);
		m_actionGroups[RowActions] << a->oid();
		a->setShortcut(QKeySequence(tr("Ctrl+D", "insert row copy")));
		a->setShortcutContext(Qt::WidgetShortcut);
		m_actionGroups[RowActions] << a->oid();
		m_actions[a->oid()] = a;
		//connect(a, SIGNAL(triggered()), this, SLOT(copyRow()));
	}
	{
		a = new Action(tr("Zobrazit ve formulari"), this);
		a->setIcon(QIcon(":/qf/qmlwidgets/images/view.png"));
		a->setToolTip(tr("Zobrazit radek v formulari pro cteni"));
		a->setShortcutContext(Qt::WidgetShortcut);
		//connect(a, SIGNAL(triggered()), this, SLOT(emitViewRowInExternalEditor()));
		a->setOid("viewRowExternal");
		m_actions[a->oid()] = a;
	}
	{
		a = new Action(tr("Upravit ve formulari"), this);
		a->setIcon(QIcon(":/qf/qmlwidgets/images/edit.png"));
		a->setToolTip(tr("Upravit radek ve formulari"));
		a->setShortcutContext(Qt::WidgetShortcut);
		//connect(a, SIGNAL(triggered()), this, SLOT(emitEditRowInExternalEditor()));
		a->setOid("editRowExternal");
		m_actions[a->oid()] = a;
	}
	{
		a = new Action(QIcon(":/qf/qmlwidgets/images/sort-asc.png"), tr("Sort ascending"), this);
		a->setOid("sortAsc");
		a->setCheckable(true);
		//a->setToolTip(tr("Upravit radek v externim editoru"));
		a->setShortcutContext(Qt::WidgetShortcut);
		m_actionGroups[SortActions] << a->oid();
		//connect(a, SIGNAL(triggered(bool)), this, SLOT(sortAsc(bool)));
		m_actions[a->oid()] = a;
	}
	{
		a = new Action(QIcon(":/qf/qmlwidgets/images/sort-desc.png"), tr("Sort descending"), this);
		a->setOid("sortDesc");
		a->setCheckable(true);
		//a->setToolTip(tr("Upravit radek v externim editoru"));
		//a->setShortcutContext(Qt::WidgetShortcut);
		m_actionGroups[SortActions] << a->oid();
		//connect(a, SIGNAL(triggered(bool)), this, SLOT(sortDesc(bool)));
		m_actions[a->oid()] = a;
	}
	{
		a = new Action(tr("Edit cell content"), this);
		//a->setToolTip(tr("Upravit radek v externim editoru"));
		a->setShortcut(QKeySequence(tr("Ctrl+Shift+T", "Edit cell content")));
		a->setShortcutContext(Qt::WidgetShortcut);
		//connect(a, SIGNAL(triggered()), this, SLOT(showCurrentCellText()));
		a->setOid("showCurrentCellText");
		m_actionGroups[CellActions] << a->oid();
		m_actions[a->oid()] = a;
	}
	{
		a = new Action(tr("Save BLOB"), this);
		//a->setToolTip(tr("Upravit radek v externim editoru"));
		//a->setShortcutContext(Qt::WidgetShortcut);
		//connect(a, SIGNAL(triggered()), this, SLOT(saveCurrentCellBlob()));
		a->setOid("saveCurrentCellBlob");
		m_actionGroups[BlobActions] << a->oid();
		m_actions[a->oid()] = a;
	}
	{
		a = new Action(tr("Load BLOB from file"), this);
		//a->setToolTip(tr("Upravit radek v externim editoru"));
		//a->setShortcutContext(Qt::WidgetShortcut);
		//connect(a, SIGNAL(triggered()), this, SLOT(loadCurrentCellBlob()));
		a->setOid("loadCurrentCellBlob");
		m_actionGroups[BlobActions] << a->oid();
		m_actions[a->oid()] = a;
	}
	{
		a = new Action(tr("Insert rows statement"), this);
		//a->setToolTip(tr("Upravit radek v externim editoru"));
		//a->setShortcutContext(Qt::WidgetShortcut);
		//connect(a, SIGNAL(triggered()), this, SLOT(insertRowsStatement()));
		a->setOid("insertRowsStatement");
		m_actionGroups[RowActions] << a->oid();
		m_actions[a->oid()] = a;
	}
	{
		a = new Action(tr("Set NULL in selection"), this);
		//a->setToolTip(tr("Upravit radek v externim editoru"));
		a->setShortcut(QKeySequence(tr("Ctrl+Shift+L", "Set NULL in selection")));
		a->setShortcutContext(Qt::WidgetShortcut);
		//connect(a, SIGNAL(triggered()), this, SLOT(setNullInSelection()));
		a->setOid("setNullInSelection");
		m_actionGroups[SetValueActions] << a->oid();
		m_actions[a->oid()] = a;
	}
	{
		a = new Action(tr("Set value in selection"), this);
		a->setShortcut(QKeySequence(tr("Ctrl+Shift+E", "Set value in selection")));
		a->setShortcutContext(Qt::WidgetShortcut);
		//connect(a, SIGNAL(triggered()), this, SLOT(setValueInSelection()));
		a->setOid("setValueInSelection");
		m_actionGroups[SetValueActions] << a->oid();
		m_actions[a->oid()] = a;
	}
	{
		a = new Action(tr("Generate sequence in selection"), this);
		//a->setShortcut(QKeySequence(tr("Ctrl+Shift+E", "Set value in selection")));
		a->setShortcutContext(Qt::WidgetShortcut);
		//connect(a, SIGNAL(triggered()), this, SLOT(generateSequenceInSelection()));
		a->setOid("generateSequenceInSelection");
		m_actionGroups[SetValueActions] << a->oid();
		m_actions[a->oid()] = a;
	}
	{
		a = new Action(tr("Select"), this);
		a->setOid("select");
		m_actionGroups[SelectActions] << a->oid();
		m_actions[a->oid()] = a;
		QMenu *m = new QMenu(this);
		a->setMenu(m);
		{
			a = new Action(tr("Select current column"), this);
			a->setShortcutContext(Qt::WidgetShortcut);
			//connect(a, SIGNAL(triggered()), this, SLOT(selectCurrentColumn()));
			a->setOid("selectCurrentColumn");
			m->addAction(a);
		}
		{
			a = new Action(tr("Select current row"), this);
			a->setShortcutContext(Qt::WidgetShortcut);
			//connect(a, SIGNAL(triggered()), this, SLOT(selectCurrentRow()));
			a->setOid("selectCurrentRow");
		}
		m->addAction(a);
	}
	{
		a = new Action(tr("Calculate"), this);
		a->setOid("calculate");
		m_actionGroups[CalculateActions] << a->oid();
		m_actions[a->oid()] = a;
		QMenu *m = new QMenu(this);
		a->setMenu(m);
		{
			a = new Action(tr("Sum column"), this);
			//connect(a, SIGNAL(triggered()), this, SLOT(sumColumn()));
			a->setOid("sumColumn");
			m->addAction(a);
		}
		{
			a = new Action(tr("Sum selection"), this);
			//connect(a, SIGNAL(triggered()), this, SLOT(sumSelection()));
			a->setOid("sumSelection");
			m->addAction(a);
		}
	}
	{
		a = new Action(tr("Export"), this);
		a->setOid("export");
		m_actionGroups[ExportActions] << a->oid();
		m_actions[a->oid()] = a;
		QMenu *m = new QMenu(this);
		a->setMenu(m);
		{
			a = new Action(tr("Report"), this);
			//connect(a, SIGNAL(triggered()), this, SLOT(exportReport()));
			a->setOid("exportReport");
			m_actions[a->oid()] = a;
			m->addAction(a);
		}
		{
			a = new Action(tr("CSV"), this);
			//connect(a, SIGNAL(triggered()), this, SLOT(exportCSV()));
			a->setOid("exportCSV");
			m_actions[a->oid()] = a;
			m->addAction(a);
		}
		{
			a = new Action(tr("HTML"), this);
			//connect(a, SIGNAL(triggered()), this, SLOT(exportHTML()));
			a->setOid("exportHTML");
			m_actions[a->oid()] = a;
			m->addAction(a);
		}
#ifdef QF_XLSLIB
		{
			a = new Action(tr("XLS - MS Excel"), this);
			//connect(a, SIGNAL(triggered()), this, SLOT(exportXLS()));
			a->setOid("exportXLS");
			m_actions[a->oid()] = a;
			m->addAction(a);
		}
#endif
		{
			a = new Action(tr("XML (MS Excel 2003)"), this);
			//connect(a, SIGNAL(triggered()), this, SLOT(exportXML()));
			a->setOid("exportXML");
			m_actions[a->oid()] = a;
			m->addAction(a);
		}
	}
	{
		a = new Action(tr("Import"), this);
		a->setOid("import");
		m_actionGroups[ImportActions] << a->oid();
		m_actions[a->oid()] = a;
		QMenu *m = new QMenu(this);
		a->setMenu(m);
		{
			a = new Action(tr("CSV"), this);
			//connect(a, SIGNAL(triggered()), this, SLOT(importCSV()));
			a->setOid("importCSV");
			m_actions[a->oid()] = a;
			m->addAction(a);
		}
	}

	m_toolBarActions << action("insertRow");
	m_toolBarActions << action("copyRow");
	m_toolBarActions << action("removeSelectedRows");
	m_toolBarActions << action("postRow");
	m_toolBarActions << action("revertRow");
	//m_toolBarActions << action("viewRowExternal");
	//m_toolBarActions << action("editRowExternal");
	a = new Action(this); a->setSeparator(true);
	m_toolBarActions << a;
	m_toolBarActions << action("reload");
	a = new Action(this); a->setSeparator(true);
	m_toolBarActions << a;
	m_toolBarActions << action("sortAsc");
	m_toolBarActions << action("sortDesc");

	//f_contextMenuActions = standardContextMenuActions();

	{
		for(Action *a : m_actions) {
			if(!a->shortcut().isEmpty()) {
				//qfInfo() << "\t inserting action" << a->text() << a->shortcut().toString();
				addAction(a); /// aby chodily shortcuty, musi byt akce pridany widgetu
			}
		}
		//qfDebug() << "\t default actions inserted";
	}
}

QList<Action *> TableView::contextMenuActionsForGroups(int action_groups)
{
	qfLogFuncFrame();
	//static Action *act_separator = nullptr;
	QList<Action*> ret;
	QList<int> grps;
	grps << SizeActions << SortActions << FilterActions << ViewActions << PasteActions << RowActions << BlobActions << SetValueActions << CellActions << SelectActions << CalculateActions << ExportActions << ImportActions;
	int cnt = 0;
	foreach(int grp, grps) {
		if(action_groups & grp) {
			QStringList sl = m_actionGroups.value(grp);
			if(!sl.isEmpty() && (cnt++ > 0)) {
				Action *act_separator = m_separatorsForGroup.value((ActionGroup)grp);
				if(!act_separator) {
					act_separator = new Action(this);
					act_separator->setSeparator(true);
					m_separatorsForGroup[(ActionGroup)grp] = act_separator;
				}
				act_separator = m_separatorsForGroup.value((ActionGroup)grp);
				ret << act_separator;
			}
			for(auto oid : sl) {
				Action *a = m_actions.value(oid);
				if(a == nullptr)
					qfWarning() << QString("Cannot find action for oid: '%1'").arg(oid);
				else
					ret << a;
			}
		}
	}
	return ret;
}

Action *TableView::action(const QString &act_oid)
{
	Action *ret = m_actions.value(act_oid);
	QF_ASSERT_EX(ret != nullptr, QString("Invalid action id: '%1'").arg(act_oid));
	return ret;
}

void TableView::currentChanged(const QModelIndex& current, const QModelIndex& previous)
{
	qfLogFuncFrame() << "row" << previous.row() << "->" << current.row();
#if 0
	//emit currentChanging(current, previous);
	//qfDebug().color(QFLog::Yellow) << "set focus to table view";
	if(ignoreCurrentChanged) {
		/// ignoreCurrentChanged funguje jen jednou, tim zabranim zaseknuti funkce currentChanged() pri spatnem pouziti ignoreCurrentChanged.
		ignoreCurrentChanged = false;
		return;
	}
	if(isFocusOnCurrentChanged()) setFocus(); /// pokud nekdo neco resi s widgety jako reakci na signal currentChanging(), muze prijit o fokus a prestane chodit kurzorova navigace
#endif
	setFocus(); /// pokud nekdo neco resi s widgety jako reakci na signal currentChanging(), muze prijit o fokus a prestane chodit kurzorova navigace
	Super::currentChanged(current, previous);
	if(current.row() != previous.row() && previous.row() >= 0) {
		qfDebug() << "\tsaving previous row:" << previous.row();
		bool ok = false;
		try {
			ok = postRow(previous.row());
		}
		catch(qf::core::Exception &e) {
			dialogs::MessageBox::showException(this, e);
		}
		if(!ok)
			setCurrentIndex(previous);
		qfDebug() << "\t" << __LINE__;
		updateRow(previous.row());
		updateRow(current.row());
	}
	refreshActions();
	//emitSelected(previous, current);
	/// na selected() muze prijit table o fokus
	setFocus();
}
