#ifndef VIDEOEDITOR_H
#define VIDEOEDITOR_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QWidget>
#include <QtWidgets/QTreeWidgetItem>
#include <QtCore/QFile>
#include <QtCore/QThread>
#include <vector>
#include <fstream>
#include <iostream>

extern "C" {
class ISVCDecoder;
class ISVCEncoder;
}

namespace Ui
{
class VideoEditor;
}


class FileCache
{
public:
	FileCache( const std::string& filename, uint32_t cache_size = 1024 * 1024 )
		: mCacheSize( cache_size )
		, mCache( new uint8_t[cache_size] )
		, mReadOffset( 0 )
		, mFile( new std::ifstream( filename ) )
		{
		}
	~FileCache()
	{
		delete mFile;
	}

	int64_t seek( uint64_t ofs )
	{
		if ( ofs >= (uint64_t)mFile->tellg() - (uint64_t)mCacheSize and ofs <= (uint64_t)mFile->tellg() ) {
			std::cout << "no seek\n";
			return mFile->tellg();
		}
		std::cout << "seek (" << ofs << " | " << mFile->tellg() << ")\n";
		mFile->seekg( ofs );
		if ( mFile->eof() ) {
			return -1;
		}
		int64_t ret = mFile->tellg();
		mFile->readsome( (char*)mCache, mCacheSize );
		mReadOffset = 0;
		return ret;
	}

	int64_t read( void* buf, uint64_t n )
	{
		if ( mReadOffset + n >= mCacheSize ) {
			std::cout << "need seek\n";
			if ( seek( (uint64_t)mFile->tellg() + n ) < 0 ) {
				return -1;
			}
		}
		memcpy( buf, mCache + mReadOffset, n );
		mReadOffset += n;
		return n;
	}

protected:
	uint32_t mCacheSize;
	uint8_t* mCache;
	uint32_t mReadOffset;
	std::ifstream* mFile;
};


class VideoEditor : public QMainWindow
{
	Q_OBJECT

public:
	typedef enum {
		TrackTypeVideo,
		TrackTypeAudio,
	} TrackType;

	VideoEditor();
	~VideoEditor();

public slots:
	void Open();
	void Export();
	void Play();
	void setWhiteBalanceTemperature( int t );
	void setVibrance( int v );

private:
	uint64_t GetTicks();
	void WaitTick( uint64_t final );
	QTreeWidgetItem* addTreeRoot( const QString& name, const QString& value = "" );
	QTreeWidgetItem* addTreeChild( QTreeWidgetItem* parent, const QString& name, const QString& value = "" );
	bool play();

	typedef struct {
		uint32_t id;
		TrackType type;
		QString format;
		QString filename;
		QFile* file;
		FileCache* cacheFile;
		union {
			struct {
				uint32_t width;
				uint32_t height;
				uint32_t framerate;
			};
			struct {
				uint32_t channels;
				uint32_t samplerate;
			};
		};
	} Track;

	typedef struct {
		uint64_t time;
		uint32_t id;
		uint64_t offset;
		uint32_t size;
	} Timepoint;

	class PlayerThread : public QThread
	{
	public:
		PlayerThread( VideoEditor* e ) : mEditor( e ) {}
	protected:
		virtual void run() { while ( mEditor->play() ); }
		VideoEditor* mEditor;
	};

	Ui::VideoEditor* ui;
	std::vector< Track* > mTracks;
	QString mInputFilename;
	QFile* mInputFile;
	uint64_t mLastTimestamp;

	PlayerThread* mPlayer;
	uint32_t mPlayerIdx;
	uint64_t mPlayerStartTime;
	std::vector< Timepoint > mTimeline;
	uint64_t mPlayerFPSTimer;
	uint32_t mPlayerFPSCounter;
	uint32_t mPlayerFPS;

	ISVCDecoder* mDecoder;
	ISVCEncoder* mEncoder;
};

#endif // VIDEOEDITOR_H
