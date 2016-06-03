#ifndef HSTATUSBAR_H
#define HSTATUSBAR_H

#include <QtWidgets/QWidget>
#include <QtGui/QPaintEvent>

class HStatusBar : public QWidget
{
	Q_OBJECT

public:
	HStatusBar( QWidget* parent );
	~HStatusBar();

	void setValue( int32_t v );
	void setMaxValue( int32_t v );
	void setSuffix( const QString& sfx );

protected:
	virtual void paintEvent( QPaintEvent* ev );

private:
	int32_t mValue;
	int32_t mMaxValue;
	QString mSuffix;
};

#endif // HSTATUSBAR_H
