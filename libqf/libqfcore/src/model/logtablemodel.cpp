#include "logtablemodel.h"

#include "../core/logdevice.h"

#include <QColor>

namespace qf {
namespace core {
namespace model {

LogTableModel::TableRow::TableRow(qf::core::Log::Level severity, const QString &domain, const QString &file, const QString &msg, const QDateTime &time_stamp)
{
	m_data.resize(Cols::Count);
	m_data[Cols::Severity] = QVariant::fromValue(severity);
	m_data[Cols::Domain] = domain;
	m_data[Cols::File] = file;
	m_data[Cols::Message] = msg;
	m_data[Cols::Timestamp] = time_stamp;
}

QVariant LogTableModel::TableRow::value(int col) const
{
	QVariant val = m_data.value(col);
	return val;
}

LogTableModel::LogTableModel(QObject *parent)
	: Super(parent)
{
}

QVariant LogTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
		switch (section) {
		case TableRow::Domain:
			return tr("Domain");
		case TableRow::File:
			return tr("File");
		case TableRow::Severity:
			return tr("Severity");
		case TableRow::Timestamp:
			return tr("Time");
		case TableRow::Message:
			return tr("Message");
		};
		return Super::headerData(section, orientation, role);
	}
	//if (orientation == Qt::Vertical && role == Qt::DisplayRole) {
	//	return rowCount() - section;
	//}
	return Super::headerData(section, orientation, role);
}

int LogTableModel::rowCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return m_rows.count();
}

int LogTableModel::columnCount(const QModelIndex &parent) const
{
	Q_UNUSED(parent)
	return TableRow::Count;
}

QVariant LogTableModel::data(const QModelIndex &index, int role) const
{
	if (index.row() < 0 && index.row() >= rowCount()) {
		return QVariant();
	}
	switch (role) {
	case Qt::DisplayRole: {
		QVariant ret = data(index, Qt::EditRole);
		if(ret.userType() == qMetaTypeId<qf::core::Log::Level>())
			ret = qf::core::Log::levelToString(ret.value<qf::core::Log::Level>());
		return ret;
	}
	case Qt::EditRole:
		return m_rows[index.row()].value(index.column());
	case Qt::BackgroundRole: {
		auto severity = m_rows[index.row()].value(TableRow::Severity).value<qf::core::Log::Level>();
		switch (severity) {
		case qf::core::Log::Level::Invalid:
		case qf::core::Log::Level::Fatal:
		case qf::core::Log::Level::Error:
			return QColor(Qt::red).lighter(170);
		case qf::core::Log::Level::Warning:
			return QColor(Qt::cyan).lighter(170);
		case qf::core::Log::Level::Info:
			return QColor(Qt::yellow).lighter(170);
		default:
			return QVariant();
		}
	}
	};
	return QVariant();
}

void LogTableModel::clear()
{
	beginResetModel();
	m_rows.clear();
	endResetModel();
}

LogTableModel::TableRow LogTableModel::rowAt(int row) const
{
	return m_rows.value(row);
}

void LogTableModel::addLogEntry(qf::core::Log::Level severity, const QString &domain, const QString &file, int line, const QString &msg, const QDateTime &time_stamp)
{
	beginInsertRows(QModelIndex(), rowCount(), rowCount());
	QString module = qf::core::LogDevice::moduleFromFileName(file);
	m_rows.append(TableRow(severity, domain, QString("%1:%2").arg(module).arg(line), msg, time_stamp));
	endInsertRows();
}

}}}