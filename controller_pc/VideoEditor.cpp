#include <mp4v2/mp4v2.h>
#include <QtWidgets/QFileDialog>
#include <QtCore/QTextStream>
#include <QtCore/QDebug>
#include "VideoEditor.h"
#include "ui_videoEditor.h"

VideoEditor::VideoEditor()
	: QMainWindow()
	, ui( new Ui::VideoEditor )
	, mInputFile( nullptr )
	, mLastTimestamp( 0 )
{
	ui->setupUi(this);

	connect( ui->btnOpen, SIGNAL(pressed()), this, SLOT(Open()) );
	connect( ui->btnExport, SIGNAL(pressed()), this, SLOT(Export()) );
}


VideoEditor::~VideoEditor()
{
	delete ui;
}


void VideoEditor::Open()
{
	QString fileName = QFileDialog::getOpenFileName( this, "Open record", "", "*.csv" );

	if ( fileName == "" ) {
		return;
	}

	if ( mInputFile ) {
		mInputFile->close();
		delete mInputFile;
	}
	mInputFile = new QFile( fileName );
	if ( not mInputFile->open( QIODevice::ReadOnly ) ) {
		return;
	}

	for ( Track* track : mTracks ) {
		delete track;
	}
	mTracks.clear();
	ui->tree->clear();
	QTreeWidgetItem* itemInfos = addTreeRoot( "Infos" );
	QTreeWidgetItem* itemTracks = addTreeRoot( "Tracks" );

	QTextStream in( mInputFile );
	while ( not in.atEnd() ) {
		QString sline = in.readLine();
		QStringList line = sline.split( "," );
		if ( line[0] == "new_track" ) {
			Track* track = new Track;
			track->id = line[1].toUInt();
			track->filename = line[3];
			track->format = line[3].split(".").last();
			QStringList infos = track->filename.split( QRegExp( "_|\\." ) );
			QTreeWidgetItem* itemTrack = addTreeChild( itemTracks, QString::number( track->id ) );
			addTreeChild( itemTrack, "type", line[2] );
			addTreeChild( itemTrack, "file", track->filename );
			addTreeChild( itemTrack, "format", infos.last() );
			if ( line[2] == "video" ) {
				track->type = TrackTypeVideo;
				track->width = infos[1].split("x")[0].toUInt();
				track->height = infos[1].split("x")[1].toUInt();
				track->framerate = infos[2].split("fps")[0].toUInt();
				addTreeChild( itemTrack, "width", QString::number(track->width) + " px" );
				addTreeChild( itemTrack, "height", QString::number(track->height) + " px" );
				addTreeChild( itemTrack, "framerate", QString::number(track->framerate) + " fps" );
			} else if ( line[2] == "audio" ) {
				track->type = TrackTypeAudio;
				track->samplerate = infos[1].split("hz")[0].toUInt();
				track->channels = infos[2].split("ch")[0].toUInt();
				addTreeChild( itemTrack, "samplerate", QString::number(track->samplerate) + " Hz" );
				addTreeChild( itemTrack, "channels", QString::number(track->channels) );
			}
			track->file = new QFile( fileName.mid( 0, fileName.lastIndexOf("/") + 1 ) + track->filename );
			track->file->open( QIODevice::ReadOnly );
			mTracks.emplace_back( track );
		} else if ( sline[0] >= '0' and sline[0] <= '9' ) {
			mLastTimestamp = line[1].toUInt();
		}
	}
	addTreeChild( itemInfos, "tracks", QString::number( mTracks.size() ) );
	addTreeChild( itemInfos, "duration", "00m:00s" );
	addTreeChild( itemInfos, "total size", "0 KB" );

	ui->tree->expandAll();
}


void VideoEditor::Export()
{
	QString fileName = QFileDialog::getSaveFileName( this, "Save record", "", "*.mp4" );
	if ( fileName == "" ) {
		return;
	}

// 	MP4LogSetLevel( MP4_LOG_VERBOSE4 );
	MP4FileHandle handle = MP4Create( fileName.toUtf8().data(), 0 );
	std::vector< MP4TrackId > trackIds;

	for ( Track* track : mTracks ) {
		MP4TrackId trackId;
		if ( track->type == TrackTypeVideo ) {
			if ( track->format == "h264" ) {
				trackId = MP4AddH264VideoTrack( handle, 90 * 1000, MP4_INVALID_DURATION, track->width, track->height, 0, 0, 0, 3 );
			}
		} else if ( track->type == TrackTypeAudio ) {
			if ( track->format == "mp3" ) {
				trackId = MP4AddAudioTrack( handle, track->samplerate, MP4_INVALID_DURATION, MP4_MP3_AUDIO_TYPE );
				MP4SetTrackName( handle, trackId, ".mp3" );
				MP4SetTrackIntegerProperty( handle, trackId, "mdia.minf.stbl.stsd.mp4a.channels", track->channels );
			}
		}
		trackIds.emplace_back( trackId );
	}

	MP4Duration first_record_time = 0;

	mInputFile->seek( 0 );
	QTextStream in( mInputFile );
	while ( not in.atEnd() ) {
		QString sline = in.readLine();
		QStringList line = sline.split( "," );
		if ( sline[0] == '#' or line[0] == "new_track" ) {
			continue;
		}
		uint32_t iID = line[0].toUInt();
		MP4Duration record_time = line[1].toUInt();
		uint32_t offset = line[2].toUInt();
		uint32_t size = line[3].toUInt();
		MP4TrackId& trackId = trackIds[iID];
		Track* track = mTracks[iID];

		ui->progress->setValue( record_time * 100 / mLastTimestamp );

		if ( first_record_time == 0 ) {
			first_record_time = record_time;
		}
		record_time -= first_record_time;

		uint8_t* buf = new uint8_t[ size ];
		track->file->seek( offset );
		qint64 read_ret = track->file->read( (char*)buf, size );

		MP4Duration duration = MP4_INVALID_DURATION;
		QString nextLine;
		qint64 lastpos = in.pos();
		while ( not in.atEnd() ) {
			nextLine = in.readLine();
			if ( nextLine[0] >= '0' and nextLine[0] <= '9' and nextLine.split(",")[0].toUInt() == iID ) {
				duration = ( (MP4Duration)nextLine.split(",")[1].toUInt() - first_record_time ) - record_time;
				break;
			}
		}
		in.seek( lastpos );

		if ( track->type == TrackTypeAudio ) {
			if ( duration == MP4_INVALID_DURATION ) {
				printf( "invalid duration\n" );
// 				duration = 1152;
			} else {
				printf( "audio timestamp : %lu => %lu [%lu]\n", record_time, record_time * track->samplerate / 1000000, duration );
				MP4WriteSample( handle, trackId, buf, size, duration * track->samplerate / 1000000, 0/*record_time * track->samplerate / 1000000*/, false );
			}
		} else {
			if ( duration == MP4_INVALID_DURATION ) {
				printf( "invalid duration\n" );
			} else {
				printf( "video timestamp : %lu => %lu [%lu]\n", record_time, record_time * 90000 / 1000000, duration );
				MP4WriteSample( handle, trackId, buf, size, duration * 90000 / 1000000, 0/*record_time * 90000 / 1000000*/, true );
			}
		}
// 		getchar();
		delete buf;
	}

	MP4Close( handle );
}


QTreeWidgetItem* VideoEditor::addTreeRoot( const QString& name, const QString& value )
{
	QTreeWidgetItem* treeItem = new QTreeWidgetItem( ui->tree );

	treeItem->setText( 0, name );
	treeItem->setText( 1, value );

	return treeItem;
}


QTreeWidgetItem* VideoEditor::addTreeChild( QTreeWidgetItem* parent, const QString& name, const QString& value )
{
	QTreeWidgetItem* treeItem = new QTreeWidgetItem();

	treeItem->setText( 0, name );
	treeItem->setText( 1, value );

    parent->addChild( treeItem );
	return treeItem;
}
