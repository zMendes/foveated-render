#pragma once

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

#include <string>

class VideoEncoder
{
public:
    VideoEncoder(const std::string &filename, int width, int height, int fps);
    ~VideoEncoder();

    void addFrame(uint8_t *rgbData);
    void finish();

private:
    int m_width;
    int m_height;
    int m_fps;
    int m_frameIndex = 0;
    bool m_finished = false;

    AVFormatContext *m_fmtCtx = nullptr;
    AVStream *m_stream = nullptr;
    AVCodecContext *m_codecCtx = nullptr;
    SwsContext *m_swsCtx = nullptr;
    AVFrame *m_frame = nullptr;
};
