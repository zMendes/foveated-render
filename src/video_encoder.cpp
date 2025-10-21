#include "video_encoder.h"
#include <stdexcept>
#include <iostream>

VideoEncoder::VideoEncoder(const std::string &filename, int width, int height, int fps)
    : m_width(width), m_height(height), m_fps(fps)
{
    // create output format context (MP4 container)
    if (avformat_alloc_output_context2(&m_fmtCtx, nullptr, nullptr, filename.c_str()) < 0 || !m_fmtCtx)
    {
        throw std::runtime_error("Failed to create format context");
    }

    // find H.264 encoder
    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec)
        throw std::runtime_error("H.264 codec not found");

    // create new stream
    m_stream = avformat_new_stream(m_fmtCtx, nullptr);
    if (!m_stream)
        throw std::runtime_error("Failed to create stream");

    // allocate codec context
    m_codecCtx = avcodec_alloc_context3(codec);
    if (!m_codecCtx)
        throw std::runtime_error("Could not allocate codec context");

    // set codec params
    m_codecCtx->width = m_width;
    m_codecCtx->height = m_height;
    m_codecCtx->time_base = {1, m_fps}; // pts unit = 1/fps
    m_stream->time_base = m_codecCtx->time_base;
    m_stream->avg_frame_rate = {m_fps, 1};
    m_stream->r_frame_rate = {m_fps, 1};

    m_codecCtx->framerate = {m_fps, 1};
    m_codecCtx->gop_size = 12;
    m_codecCtx->max_b_frames = 2;
    m_codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
    m_codecCtx->bit_rate = 400000;

    // optional: better preset
    av_opt_set(m_codecCtx->priv_data, "crf", "0", 0);            // lossless
    av_opt_set(m_codecCtx->priv_data, "preset", "ultrafast", 0); // faster encoding
    av_opt_set(m_codecCtx->priv_data, "tune", "zerolatency", 0); // optional

    if (m_fmtCtx->oformat->flags & AVFMT_GLOBALHEADER)
        m_codecCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (avcodec_open2(m_codecCtx, codec, nullptr) < 0)
        throw std::runtime_error("Could not open codec");

    // copy codec parameters to stream
    if (avcodec_parameters_from_context(m_stream->codecpar, m_codecCtx) < 0)
        throw std::runtime_error("Could not copy codec parameters");

    m_stream->time_base = m_codecCtx->time_base;
    m_stream->avg_frame_rate = {m_fps, 1};
    m_stream->r_frame_rate = {m_fps, 1};

    // open output file
    if (!(m_fmtCtx->oformat->flags & AVFMT_NOFILE))
    {
        if (avio_open(&m_fmtCtx->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0)
            throw std::runtime_error("Could not open output file");
    }

    // write container header
    if (avformat_write_header(m_fmtCtx, nullptr) < 0)
        throw std::runtime_error("Error writing header");

    // swscale context for RGB -> YUV420
    m_swsCtx = sws_getContext(m_width, m_height, AV_PIX_FMT_RGB24,
                              m_width, m_height, AV_PIX_FMT_YUV420P,
                              SWS_BILINEAR, nullptr, nullptr, nullptr);

    // allocate reusable YUV frame
    m_frame = av_frame_alloc();
    m_frame->format = AV_PIX_FMT_YUV420P;
    m_frame->width = m_width;
    m_frame->height = m_height;
    if (av_frame_get_buffer(m_frame, 32) < 0)
        throw std::runtime_error("Could not allocate frame buffer");
}

void VideoEncoder::addFrame(uint8_t *rgbData)
{
    // temporary RGB frame
    AVFrame *rgbFrame = av_frame_alloc();
    rgbFrame->format = AV_PIX_FMT_RGB24;
    rgbFrame->width = m_width;
    rgbFrame->height = m_height;

    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize,
                         rgbData, AV_PIX_FMT_RGB24, m_width, m_height, 1);

    // convert RGB -> YUV420
    sws_scale(m_swsCtx, rgbFrame->data, rgbFrame->linesize,
              0, m_height, m_frame->data, m_frame->linesize);

    // CFR pts
    m_frame->pts = m_frameIndex++;

    av_frame_free(&rgbFrame);

    if (avcodec_send_frame(m_codecCtx, m_frame) < 0)
        throw std::runtime_error("Error sending frame to encoder");

    AVPacket *pkt = av_packet_alloc();
    if (!pkt)
        throw std::runtime_error("Could not allocate packet");

    while (avcodec_receive_packet(m_codecCtx, pkt) == 0)
    {
        // rescale packet timestamps from codec to stream time_base
        av_packet_rescale_ts(pkt, m_codecCtx->time_base, m_stream->time_base);

        pkt->stream_index = m_stream->index;
        pkt->duration = 1;

        if (av_interleaved_write_frame(m_fmtCtx, pkt) < 0)
            throw std::runtime_error("Error writing packet");

        av_packet_unref(pkt);
    }

    av_packet_free(&pkt);
}

void VideoEncoder::finish()
{
    if (m_finished)
        return;

    avcodec_send_frame(m_codecCtx, nullptr); // flush
    AVPacket *pkt = av_packet_alloc();

    while (true)
    {
        int ret = avcodec_receive_packet(m_codecCtx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            break;
        else if (ret < 0)
            throw std::runtime_error("Error during flush");

        pkt->stream_index = m_stream->index;
        av_packet_rescale_ts(pkt, m_codecCtx->time_base, m_stream->time_base);
        if (av_interleaved_write_frame(m_fmtCtx, pkt) < 0)
            throw std::runtime_error("Error writing flush packet");
    }
    av_packet_free(&pkt);

    if (av_write_trailer(m_fmtCtx) < 0)
        throw std::runtime_error("Error writing trailer");

    m_finished = true;
}

VideoEncoder::~VideoEncoder()
{
    try
    {
        finish();
    }
    catch (...)
    {
        std::cerr << "[VideoEncoder] Warning: exception during finish\n";
    }

    if (m_frame)
        av_frame_free(&m_frame);
    if (m_codecCtx)
        avcodec_free_context(&m_codecCtx);
    if (m_fmtCtx)
    {
        if (!(m_fmtCtx->oformat->flags & AVFMT_NOFILE))
            avio_closep(&m_fmtCtx->pb);
        avformat_free_context(m_fmtCtx);
    }
    if (m_swsCtx)
        sws_freeContext(m_swsCtx);
}
