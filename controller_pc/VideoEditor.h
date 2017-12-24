#ifndef VIDEOEDITOR_H
#define VIDEOEDITOR_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QWidget>
#include <QtWidgets/QTreeWidgetItem>
#include <QtCore/QFile>
#include <vector>

namespace Ui
{
class VideoEditor;
}

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

private:
	QTreeWidgetItem* addTreeRoot( const QString& name, const QString& value = "" );
	QTreeWidgetItem* addTreeChild( QTreeWidgetItem* parent, const QString& name, const QString& value = "" );

	typedef struct {
		uint32_t id;
		TrackType type;
		QString format;
		QString filename;
		QFile* file;
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

	Ui::VideoEditor* ui;
	std::vector< Track* > mTracks;
	QFile* mInputFile;
	uint64_t mLastTimestamp;
};

#endif // VIDEOEDITOR_H
