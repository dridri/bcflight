#ifndef DRM_H
#define DRM_H

class DRM
{
public:
	static int drmFd();

protected:
	static int sDrmFd;
};

#endif // DRM_H
