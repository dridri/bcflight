#include "BlackBox.h"
#include "ui_blackbox.h"
#include <QFileDialog>
#include <QTextStream>
#include <QCheckBox>
#include <QPalette>
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

	QStringList groups;
	groups.append( "log" );

	QTextStream in( &inputFile );
	while ( not in.atEnd() ) {
		instantValues[ names.indexOf("log") - 1 ] = ""; // Reset log
		for ( QString& value : instantValues ) {
			if ( value.startsWith( "$" ) ) {
				value = value.mid( 1 );
			}
		}
		QString sline = in.readLine();
		QStringList line;
		for ( int32_t i = 0; i < sline.size(); ) {
			if ( line.size() == 2 ) {
				QString s = sline.mid( i );
				if ( s[0] == '"' ) {
					s = s.mid( 1 );
					if ( s[s.length()-1] == '"' ) {
						s = s.mid( 0, s.length() - 1 );
					}
				}
				line.append( s );
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
			instantValues.append( QString("$") + line[2] );
			QString group = line[1].split(":")[0];
			if ( not groups.contains( group ) ) {
				groups.append( group );
			}
		} else {
			instantValues[ names.indexOf( line[1] ) - 1 ] = QString("$") + line[2];
		}
		values.append( qMakePair( line[0].toULongLong(), instantValues ) );
	}

	inputFile.close();
	mData->updated();

	for ( QString group : groups ) {
		QCheckBox* check = new QCheckBox( group );
		check->setChecked( true );
		ui->groupsLayout->addWidget( check );
	}
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
	const QPair< uint64_t, QStringList >& data = values[index.row()];

	if ( role == Qt::FontRole ) {
		QFont font;
		if ( index.column() > 0 and index.column() - 1 < data.second.size() and data.second[ index.column() - 1 ].startsWith("$") ) {
			font.setBold(true);
		}
		return font;
	} else if ( role == Qt::BackgroundColorRole ) {
		if ( index.column() > 0 and index.column() - 1 < data.second.size() and data.second[ index.column() - 1 ].startsWith("$") ) {
			return table->palette().color( QPalette::AlternateBase );
		}
	} else if ( role == Qt::DisplayRole ) {
		if ( index.column() == 0 ) {
			return (qulonglong)data.first;
		}
		if ( index.column() - 1 < data.second.size() ) {
			QString str = data.second[ index.column() - 1 ];
			if ( str.startsWith("$") ) {
				str = str.mid( 1 );
			}
			return str;
		}
	}

	return QVariant();
}
