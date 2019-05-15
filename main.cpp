#include <tchar.h>

extern "C"
{
#include "libavformat/avformat.h"
};

int _tmain(int argc, _TCHAR* argv[])
{
	AVOutputFormat* ofmt = NULL;
	//（Input AVFormatContext and Output AVFormatContext）
	AVFormatContext* ifmt_ctx = NULL, *ofmt_ctx = NULL;
	AVPacket pkt;
	int ret, i;
	int videoindex = -1, audioindex = -1;
	int frame_index = 0;
	
	uint8_t sps[100];
	uint8_t pps[100];	
	int spsLength = 0;
	int ppsLength = 0;
	uint8_t startcode[4] = { 00,00,00,01 };
	FILE* fp;

	const char* in_filename, * out_filename;

	in_filename = argv[1];//输入文件名（Input file URL）
	out_filename = argv[2];//输出文件名（Output file URL）


	av_register_all();
//	fp = fopen(out_filename, "wb+");

	if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
		printf("Could not open input file.");
		return ret;
	}

	if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {
		printf("Failed to retrieve input stream information");
		avformat_close_input(&ifmt_ctx);
		return ret;
	}




	//Output
	avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
	if (!ofmt_ctx) {
		printf("Could not create output context\n");
		ret = AVERROR_UNKNOWN;
	}
	ofmt = ofmt_ctx->oformat;	

//	av_dump_format(ifmt_ctx, 0, in_filename, 0);

	for (i = 0; i < ifmt_ctx->nb_streams; i++) {
		//Create output AVStream according to input AVStream
		AVStream* in_stream = ifmt_ctx->streams[i];
		AVStream* out_stream = NULL;

		if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoindex = i;
			out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
		}
		else if (ifmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
			audioindex = i;
			continue;
		}
	
		if (!out_stream) {
		printf("Failed allocating output stream\n");
		ret = AVERROR_UNKNOWN;
		}
		//Copy the settings of AVCodecContext
		if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
			printf("Failed to copy context from input to output stream codec context\n");
	}
	out_stream->codec->codec_tag = 0;
	if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
		out_stream->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	}

	if (!(ofmt->flags & AVFMT_NOFILE)) {
		if (avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE) < 0) {
			printf("Could not open output file '%s'", out_filename);
		}
	}

	for (int i = 0; i < ifmt_ctx->streams[videoindex]->codec->extradata_size; i++)
	{
		printf("%02X ", ifmt_ctx->streams[videoindex]->codec->extradata[i]);
	}

	spsLength = ifmt_ctx->streams[videoindex]->codec->extradata[6] * 0xFF + ifmt_ctx->streams[videoindex]->codec->extradata[7];

	ppsLength = ifmt_ctx->streams[videoindex]->codec->extradata[8 + spsLength + 1] * 0xFF + ifmt_ctx->streams[videoindex]->codec->extradata[8 + spsLength + 2];

	for (int i = 0; i < spsLength; i++)
	{
		sps[i] = ifmt_ctx->streams[videoindex]->codec->extradata[i + 8];
	}

	for (int i = 0; i < ppsLength; i++)
	{
		pps[i] = ifmt_ctx->streams[videoindex]->codec->extradata[i + 8 + 2 + 1 + spsLength];
	}

	//Write file header
	if (avformat_write_header(ofmt_ctx, NULL) < 0) {
		printf("Error occurred when opening output file\n");
	}
	

	int flag = 1;

	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;

	//AVIOContext* s = ofmt_ctx->pb;
	//s->direct = 1;
	//avio_write(s, (const unsigned char*)"hello", 5);


	while (1) {
		AVStream* in_stream, * out_stream;
		//Get an AVPacket
		if (av_read_frame(ifmt_ctx, &pkt) < 0)
			break;
		in_stream = ifmt_ctx->streams[pkt.stream_index];
//		out_stream = ofmt_ctx->streams[pkt.stream_index];	
		if (pkt.stream_index == videoindex) {
			out_stream = ofmt_ctx->streams[0];
			printf("Write Video Packet. size:%d\tpts:%lld\n", pkt.size, pkt.pts);	
		}
		else {
			continue;
		}
		
		//AVPacket tmppkt;
		//if (in_stream->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		//{

		//	if (flag)
		//	{
		//		fwrite(startcode, 4, 1, fp);
		//		fwrite(sps, spsLength, 1, fp);
		//		fwrite(startcode, 4, 1, fp);
		//		fwrite(pps, ppsLength, 1, fp);

		//		pkt.data[0] = 0x00;
		//		pkt.data[1] = 0x00;
		//		pkt.data[2] = 0x00;
		//		pkt.data[3] = 0x01;
		//		fwrite(pkt.data, pkt.size, 1, fp);

		//		flag = 0;
		//	}
		//	else
		//	{
		//		pkt.data[0] = 0x00;
		//		pkt.data[1] = 0x00;
		//		pkt.data[2] = 0x00;
		//		pkt.data[3] = 0x01;
		//		fwrite(pkt.data, pkt.size, 1, fp);
		//	}

			pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
			pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
			pkt.pos = -1;

			pkt.stream_index = 0;
		//Write
			if (av_interleaved_write_frame(ofmt_ctx, &pkt) < 0) {
				printf("Error muxing packet\n");
				break;
			}
		//}
		//printf("Write %8d frames to output file\n", frame_index);
		av_free_packet(&pkt);
		frame_index++;
	}

//	fclose(fp);
	fp = NULL;

	av_write_trailer(ofmt_ctx);
	return 0;
}