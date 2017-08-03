#ifndef BLACKBOX_H
#define BLACKBOX_H

#include <QtWidgets/QWidget>
#include <QtWidgets/QTableView>
#include <QAbstractTableModel>

class BlackBoxData : public QAbstractTableModel
{
	Q_OBJECT

public:
	BlackBoxData( QObject* parent );
	int rowCount( const QModelIndex& parent = QModelIndex() ) const;
	int columnCount( const QModelIndex& parent = QModelIndex() ) const;
	QVariant headerData( int section, Qt::Orientation orientation, int role ) const;
	QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;

	void updated();

	QStringList names;
	QStringList instantValues;
	QList< QPair< uint64_t, QStringList > > values;
	QTableView* table;

signals:
	void headerDataChanged( Qt::Orientation orientation, int first, int last );
};

namespace Ui
{
	class BlackBox;
}

class BlackBox : public QWidget
{
	Q_OBJECT

public:
	BlackBox();
	~BlackBox();

public slots:
	void openFile();

private:
	Ui::BlackBox* ui;
	BlackBoxData* mData;
};

#endif // BLACKBOX_H
