#pragma once

#include "SD.h"
#include "FS.h"
#include "LittleFS.h"
#include "JPEGDEC.h"
#include <stdlib.h>
#include <vector>
#include <string.h>

struct JPGinf
{
	int16_t width;
  	int16_t height;
  	int32_t jpgFileSize;
  	int32_t rawFileSize;
  	File jpgImage;
  	File rawImage;
};

class JPEGProcessor
{
public:
	inline JPEGProcessor(const char* jpgFileName)
	{
		_jpgFileName = jpgFileName;
	}

	inline ~JPEGProcessor()
	{
		_bytes.clear();
		free(_inf);
		free(_msg);
	}

	inline const char* debugMessage()
	{
		return _msg;
	}

	inline void process()
	{
		_rawFileName = _jpgFileName;
		if (_rawFileName.endsWith(".jpeg"))
			_rawFileName.replace(".jpeg", ".raw");
		else if (_rawFileName.endsWith(".jpg"))
			_rawFileName.replace(".jpg", ".raw");
		
		_removeFilesLFS();

		if (!SD.exists(_jpegFileName))
		{
			sprintf(_msg, "(process) %s - no such file found", _jpgFileName);
			errorState = 1;
			return;
		}
		if (!_retranslateSDtoLFS(_jpgFileName))
		{
			sprintf(_msg, "(process) Can't retranslate %s to LittleFS", _jpgFileName);
			errorState = 1;
			return;			
		}
		_jpegdec.open(_jpgFileName, _myOpen, _myClose, _myRead, _mySeek, _saveRawImage);
		_inf->width = _jpegdec.getWidth(); _inf->height = _jpegdec.getHeight();
		imageWidth = _inf->width; imageHeight = _inf->height;
		if (!_createEmptyRawFile())
		{
			errorState = 1;
			return;
		}
		if (!_jpegdec.decode(0, 0, 0))
		{
			errorState = 1;
			return;
		}
		_jpegdec.close();
		_inf->rawImage.close();
		if (!_resizeRawImage())
		{
			errorState = 1;
			return;
		}
	}

	static uint16_t imageWidth;
	static uint16_t imageHeight;
	static uint8_t errorState = 0;
private:
	static String _jpgFileName;
	static String _rawFileName;
	static std::vector<uint8_t> _bytes;
	static struct JPGinf* _inf = new struct JPGinf;
	static JPEGDEC _jpegdec;
	static char _msg[128];

	inline void _removeFilesLFS()
	{
		Dir d = LittleFS.openDir("/");
		while (d.next())
			LittleFS.remove(d.fileName());
	}

	inline bool _createEmptyRawFile()
	{
		_inf->rawImage = SD.open(_rawFileName, "w");
		if (!_inf->rawImage)
		{
			sprintf(_msg, "(createEmptyRawFile) Can't open %s file for writing", _rawFileName);
			return false;		
		}
		for (int32_t i = 0; i < _inf->width * 2 * _inf->height; i++)
		{
			if (_bytes.size() == 8000)
			{
				_inf->rawImage.write(_bytes.data(), _bytes.size());
				_bytes.clear();
			}
			_bytes.push_back(0);
		}
		_inf->rawImage.write(_bytes.data(), _bytes.size());
		_bytes.clear();
		return true;
	}

	inline void* _myOpen(const char* filename, int32_t* size)
	{
		_inf->jpgImage = LittleFS.open(filename, "r");
  		*size = _inf->jpgImage.size();
  		sprintf(_msg, "File size %d\n", *size);
  		return &_inf->jpgImage;
	}

	inline void _myClose(void* handle)
	{
		if (_inf->jpgImage) _inf->jpgImage.close();
	}

	inline int32_t _myRead(JPEGFILE* handle, uint8_t* buffer, int32_t length)
	{
		if (!_inf->jpgImage)
  		{
  			strcpy(_msg, "(myRead) err");
  			return 0;
  		}
  		return _inf->jpgImage.read(buffer, length);
	}

	inline int32_t _mySeek(JPEGFILE* handle, int32_t position)
	{
		if (!_inf->jpgImage)
  		{
  		  strcpy(_msg, "(mySeek) err");
  		  return 0;
  		}
  		return _inf->jpgImage.seek(position);
	}

	inline int _saveRawImage(JPEGDRAW* pDraw)
	{
		uint16_t MCUWidth = pDraw->iWidth; uint16_t MCUHeight = pDraw->iHeight;
  		uint16_t xpos = pDraw->x; uint16_t ypos = pDraw->y;
  		uint16_t counter = 0;
  		for (uint16_t y = ypos; y < ypos + MCUHeight; y++)
  			for (uint16_t x = xpos; x < xpos + MCUWidth; x++)
  		  	{
  		    	if (!_inf->rawImage.seek(x * 2 + y * 2 * _inf->width))
  		    	{
  		    		sprintf(_msg, "(saveRawImage) seek error at %d\n", x * 2 + y * 2 * _inf->width);
  			      	return 0;
  		    	}
  		    	_bytes.push_back((uint8_t)(pDraw->pPixels[counter] & 0xff));
  		    	_bytes.push_back((uint8_t)(pDraw->pPixels[counter] >> 8));
  		    	_inf->rawImage.write(s.data(), s.size());
  		    	_bytes.clear();
  		    	counter++;
  		  	}
  		sprintf(_msg, "Written %d bytes to cover1.bmp %d %d %d %d\n", s.size(), xpos, ypos, MCUWidth, MCUHeight);
  		return 1;
	}

	inline bool _retranslateSDtoLFS(const char* fileName)
	{
		if (!SD.exists(fileName))
  		{
    		sprintf(_msg, "(retranslateSDtoLFS) %s doesn't exist\n", fileName);
    		return false;
  		}
  		File sdFile = SD.open(fileName, "r");
  		if (!sdFile)
  		{
    		sprintf(_msg, "(retranslateSDtoLFS)Can't open file %s for reading on SD\n", fileName);
    		return false;
  		}
  		File lfsFile = LittleFS.open(fileName, "w");
  		if (!lfsFile)
  		{
  			sprintf(_msg, "(retranslateSDtoLFS) Can't open file %s for writing on LittleFS\n", fileName)
    		return false;
  		}
  		String str = String();
  		int* pFileSize = new int;
  		*pFileSize = sdFile.size();
  		for (int i = 0; i < *pFileSize; i++)
    		lfsFile.write(sdFile.read());
  		free(pFileSize);
  		lfsFile.close();
  		sdFile.close();
  		return true;
	}

	inline bool _retranslateLFStoSD(const char* fileName)
	{
		if (!LittleFS.exists(fileName))
  		{    	
    		sprintf(_msg, "(retranslateLFStoSD) %s doesn't exist\n", fileName);
    		return false;
  		}
  		File lfsFile = LittleFS.open(fileName, "r");
  		if (!lfsFile)
  		{
    		sprintf(_msg, "(retranslateLFStoSD)Can't open file %s for reading on LittleFS\n", fileName);
    		return false;
  		}
  		File sdFile = SD.open(fileName, "w");
  		if (!sdFile)
  		{
  			sprintf(_msg, "(retranslateLFStoSD) Can't open file %s for writing on SD\n", fileName)
    		return false;
  		}
  		String str = String();
  		int* pFileSize = new int;
  		*pFileSize = lfsFile.size();
  		for (int i = 0; i < *pFileSize; i++)
    		sdFile.write(lfsFile.read());
  		free(pFileSize);
  		lfsFile.close();
  		sdFile.close();
  		return true;
	}

	inline bool _resizeRawImage()
	{
		_inf->rawImage = SD.open(_rawFileName, "r");
  		if (!_inf->rawImage)
  		{
    		sprintf(_msg, "(resizeRawImage) Can't open %s file for reading", _rawFileName);
    		return false;
  		}
  		File rawImageResized = LittleFS.open(_rawFileName, "w");
  		if (!rawImageResized)
  		{
  			strcpy(_msg, "(resizeRawImage) Can't open %s file for writing", _rawFileName);
  			return false;
  		}
  		double scaleWidth = _inf->width / 220; double scaleHeight = _inf->height / 176;
  		uint16_t xscaled, yscaled;
  		std::vector<uint8_t> frame;
  		for (uint8_t y = 0; y < 176; y++)
    		for (uint8_t x = 0; x < 220; x++)    
    		{
      			xscaled = static_cast<uint16_t>(x * (_inf->width / 220));
      			yscaled = static_cast<uint16_t>(y * (_inf->height / 176));
      
      			if (!_inf->rawImage.seek(xscaled * 2 + yscaled * _inf->width * 2))
      			{
        			sprintf(_msg, "(resizeRawImage) Can't seek to %d", xscaled * 2 + yscaled * _inf->width * 2);
        			return false;
      			}
      			frame.push_back(_inf->rawImage.read());
      			frame.push_back(_inf->rawImage.read());
      			if (frame.size() == 3520)
      			{
        			rawImageResized.write(frame.data(), frame.size());
        			frame.clear();
      			}
    		}
  		_inf->rawImage.close();
  		SD.remove(_rawFileName.c_str());
  		_retranslateLFStoSD(_rawFileName.c_str());
  		LittleFS.remove(_rawFileName.c_str());
  		return true;
	}
}