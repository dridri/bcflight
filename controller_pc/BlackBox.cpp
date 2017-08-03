#include "BlackBox.h"
#include "ui_blackbox.h"
#include <QFileDialog>
#include <QTextStream>
#include <QDebug>

BlackBox::BlackBox()
{
	ui = new Ui::BlackBox;
	ui->setupUi(this);

	connect( ui->open, SIGNAL(pressed()), this, SLOT(openFile()) );

	mData = new BlackBoxData(ui->table);
	mData->table = ui->table;
    ui->table->setModel( mData );
}


BlackBox::~BlackBox()
{
    delete ui;
}


void BlackBox::openFile()
{
	QString fileName = QFileDialog::getOpenFileName( this, "Open BlackBox", "", "*.csv" );

	if ( fileName == "" ) {
		return;
	}

	QFile inputFile( fileName );
	if ( not inputFile.open( QIODevice::ReadOnly ) ) {
		return;
	}

	QStringList& names = mData->names;
	QStringList& instantValues = mData->instantValues;
	QList< QPair< uint64_t, QStringList > >& values = mData->values;

	names.clear();
	instantValues.clear();
	values.clear();
	names.append( "time (µs)" );
	names.append( "log" );
	instantValues.append( "" );

	QTextStream in( &inputFile );
	while ( not in.atEnd() ) {
		instantValues[ names.indexOf("log") - 1 ] = ""; // Reset log
		QString sline = in.readLine();
		QStringList line;
		for ( int32_t i = 0; i < sline.size(); ) {
			if ( line.size() == 2 ) {
				line.append( sline.mid( i ) );
				break;
			}
			int32_t end = i + 1;
			if ( sline[i] == '"' ) {
				end = sline.indexOf( "\"", end );
			}
			end = sline.indexOf( ",", end );
			if ( end < 0 ) {
				end = sline.size();
			}
			QString s = sline.mid( i, end - i );
			if ( s[0] == '"' ) {
				s = s.mid( 1 );
				if ( s[s.length()-1] == '"' ) {
					s = s.mid( 0, s.length() - 1 );
				}
			}
			line.append( s );
			i = end + 1;
		}
		if ( not names.contains( line[1] ) ) {
			names.append( line[1] );
			instantValues.append( line[2] );
		} else {
			instantValues[ names.indexOf( line[1] ) - 1 ] = line[2];
		}
		values.append( qMakePair( line[0].toULongLong(), instantValues ) );
	}

	inputFile.close();
	mData->updated();
}



BlackBoxData::BlackBoxData( QObject *parent )
	: QAbstractTableModel(parent)
{
}


void BlackBoxData::updated()
{
	emit headerDataChanged( Qt::Horizontal, 0, names.size() );
}


int BlackBoxData::rowCount( const QModelIndex& /*parent*/ ) const
{
   return values.size();
}


int BlackBoxData::columnCount( const QModelIndex& /*parent*/ ) const
{
    return names.size();
}


QVariant BlackBoxData::headerData( int section, Qt::Orientation orientation, int role ) const
{
	if ( role == Qt::DisplayRole and orientation == Qt::Horizontal ) {
		return names.at( section );
	}

	return QVariant();
}


QVariant BlackBoxData::data( const QModelIndex& index, int role ) const
{
	if ( role == Qt::DisplayRole ) {
		const QPair< uint64_t, QStringList >& data = values[index.row()];
		if ( index.column() == 0 ) {
			return (qulonglong)data.first;
		}
		if ( index.column() - 1 < data.second.size() ) {
			return data.second[ index.column() - 1 ];
		}
	}

	return QVariant();
}
