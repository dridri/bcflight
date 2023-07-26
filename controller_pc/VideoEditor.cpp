#include <unistd.h>
#include <mp4v2/mp4v2.h>
#include <QtWidgets/QFileDialog>
#include <QtCore/QTextStream>
#include <QtCore/QDebug>
#include <iostream>
#include "VideoEditor.h"
#include "ui_videoEditor.h"
#include "ui/VideoViewer.h"

extern "C" {
#include "../../external/openh264-master/codec/api/svc/codec_api.h"
}

VideoEditor::VideoEditor()
	: QMainWindow()
	, ui( new Ui::VideoEditor )
	, mInputFile( nullptr )
	, mLastTimestamp( 0 )
	, mDecoder( nullptr )
	, mEncoder( nullptr )
{
	ui->setupUi(this);

	connect( ui->btnOpen, SIGNAL(pressed()), this, SLOT(Open()) );
	connect( ui->btnExport, SIGNAL(pressed()), this, SLOT(Export()) );
	connect( ui->play, SIGNAL(pressed()), this, SLOT(Play()) );
	connect( ui->temp, SIGNAL(valueChanged(int)), this, SLOT(setWhiteBalanceTemperature(int)) );
	connect( ui->vibrance, SIGNAL(valueChanged(int)), this, SLOT(setVibrance(int)) );

	mPlayer = new PlayerThread( this );
}


VideoEditor::~VideoEditor()
{
	delete ui;
}


void VideoEditor::Open()
{
	QString fileName = QFileDialog::getOpenFileName( this, "Open record", "/mnt/data/drone", "*.csv" );

	if ( fileName == "" ) {
		return;
	}
	mInputFilename = fileName;

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
			track->file = nullptr;
			track->cacheFile = nullptr;
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
			mTracks.emplace_back( track );
		} else if ( sline[0] >= '0' and sline[0] <= '9' ) {
			mLastTimestamp = line[1].toUInt();
		}
	}
	addTreeChild( itemInfos, "tracks", QString::number( mTracks.size() ) );
	addTreeChild( itemInfos, "duration", "00m:00s" );
	addTreeChild( itemInfos, "total size", "0 KB" );

	ui->tree->expandAll();

	mTimeline.clear();
	mInputFile->seek( 0 );
	in.seek( 0 );
	uint64_t first_record_time = 0;
	while ( not in.atEnd() ) {
		QString sline = in.readLine();
		QStringList line = sline.split( "," );
		if ( sline[0] == '#' or line[0] == "new_track" ) {
			continue;
		}
		Timepoint point;
		point.id = line[0].toUInt();
		point.time = line[1].toUInt();
		point.offset = line[2].toUInt();
		point.size = line[3].toUInt();
		if ( first_record_time == 0 ) {
			first_record_time = point.time;
		}
		point.time -= first_record_time;
		mTimeline.push_back( point );
	}

	if ( mDecoder ) {
		WelsDestroyDecoder( mDecoder );
		mDecoder = nullptr;
	}
	qDebug() << "WelsCreateDecoder :" << WelsCreateDecoder( &mDecoder );
	SDecodingParam decParam;
	memset( &decParam, 0, sizeof (SDecodingParam) );
	decParam.uiTargetDqLayer = UCHAR_MAX;
	decParam.eEcActiveIdc = ERROR_CON_SLICE_MV_COPY_CROSS_IDR;//ERROR_CON_SLICE_COPY;
	decParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_AVC;
	qDebug() << "mDecoder->Initialize :" << mDecoder->Initialize( &decParam );
}


void VideoEditor::setWhiteBalanceTemperature( int t )
{
	ui->preview->setWhiteBalanceTemperature( (float)t );
}


void VideoEditor::setVibrance( int t )
{
	ui->preview->setVibrance( (float)t / 50.0f );
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
		if ( not track->file ) {
			track->file = new QFile( mInputFilename.mid( 0, mInputFilename.lastIndexOf("/") + 1 ) + track->filename );
			track->file->open( QIODevice::ReadOnly );
		}
	
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
		delete[] buf;
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


void VideoEditor::Play()
{
	for ( auto track : mTracks ) {
		if ( not track->cacheFile ) {
			std::string file = ( mInputFilename.mid( 0, mInputFilename.lastIndexOf("/") + 1 ) + track->filename ).toStdString();
			track->cacheFile = new FileCache( file, 8 * 1024 * 1024 );
		} else {
			track->cacheFile->seek( 0 );
		}
	}

	mPlayerIdx = 0;
	mPlayer->start();
}


bool VideoEditor::play()
{
	uint8_t* data[3];
	SBufferInfo bufInfo;

	memset( data, 0, sizeof(data) );
	memset( &bufInfo, 0, sizeof(SBufferInfo) );

	VideoViewer::Plane& mY = ui->preview->planeY();
	VideoViewer::Plane& mU = ui->preview->planeU();
	VideoViewer::Plane& mV = ui->preview->planeV();

	uint8_t src[1024 * 512];
	Timepoint& point = mTimeline[mPlayerIdx++];
	Track* track = mTracks[point.id];

	if ( mPlayerStartTime == 0 ) {
		mPlayerStartTime = GetTicks();
	}

	// skip audio for new
	if ( track->type == TrackTypeAudio ) {
		return true;
	}

	mPlayerFPSCounter++;
	if ( GetTicks() - mPlayerFPSTimer >= 1000000 ) {
		mPlayerFPSTimer = GetTicks();
		mPlayerFPS = mPlayerFPSCounter;
		mPlayerFPSCounter = 0;
	}

	// skip if underrun
	if ( GetTicks() >= mPlayerStartTime + point.time ) {
		printf( "underrun [%d]\n", mPlayerFPS );
// 		return true;
	}

	track->cacheFile->seek( point.offset );
	track->cacheFile->read( src, point.size );

	DECODING_STATE ret = mDecoder->DecodeFrameNoDelay( src, (int)point.size, data, &bufInfo );
	printf( "        DecodeFrameNoDelay returned %08X [%d]\n", ret, mPlayerFPS );

	if ( bufInfo.iBufferStatus == 1 ) {
		mY.stride = bufInfo.UsrData.sSystemBuffer.iStride[0];
		mY.width = bufInfo.UsrData.sSystemBuffer.iWidth;
		mY.height = bufInfo.UsrData.sSystemBuffer.iHeight;
		mY.data = data[0];

		mU.stride = bufInfo.UsrData.sSystemBuffer.iStride[1];
		mU.width = bufInfo.UsrData.sSystemBuffer.iWidth / 2;
		mU.height = bufInfo.UsrData.sSystemBuffer.iHeight / 2;
		mU.data = data[1];

		mV.stride = bufInfo.UsrData.sSystemBuffer.iStride[1];
		mV.width = bufInfo.UsrData.sSystemBuffer.iWidth / 2;
		mV.height = bufInfo.UsrData.sSystemBuffer.iHeight / 2;
		mV.data = data[2];

// 		WaitTick( mPlayerStartTime + point.time );
		ui->preview->invalidate();
	}

	return true;
}


uint64_t VideoEditor::GetTicks()
{
	struct timespec now;
	clock_gettime( CLOCK_MONOTONIC, &now );
	return (uint64_t)now.tv_sec * 1000000ULL + (uint64_t)now.tv_nsec / 1000ULL;
}


void VideoEditor::WaitTick( uint64_t final )
{
	if ( GetTicks() >= final ) {
		std::cout << "skip " << ( GetTicks() - final ) << " (" << ( final - mPlayerStartTime ) << ")" << "\n";
		return;
	}
	usleep( final - GetTicks() - 10 );
}
