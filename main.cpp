#include <tchar.h>

extern "C"
{
#include "libavformat/avformat.h"
};

int _tmain(int argc, _TCHAR* argv[])
{
	//（Input AVFormatContext and Output AVFormatContext）
	AVFormatContext* ifmt_ctx = NULL;
	AVPacket pkt;
	int ret, i;
	int videoindex = 1;
	int frame_index = 0;
	
	uint8_t sps[100];
	uint8_t pps[100];	
	int spsLength = 0;
	int ppsLength = 0;
	uint8_t startcode[4] = { 00,00,00,01 };
	FILE* fp;

	const char* in_filename, * out_filename, * origin_out_filename;

	in_filename = argv[1];//输入文件名（Input file URL）
	out_filename = argv[2];//输出文件名（Output file URL）
	origin_out_filename = argv[3];//输出文件名（Output file URL）

	av_register_all();
	fp = fopen(out_filename, "wb+");

	if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0) {
		printf("Could not open input file.");
		return ret;
	}

	if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {
		printf("Failed to retrieve input stream information");
		avformat_close_input(&ifmt_ctx);
		return ret;
	}

//	av_dump_format(ifmt_ctx, 0, in_filename, 0);

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
	
	int flag = 1;

	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;

	//AVIOContext* s = ofmt_ctx->pb;
	//s->direct = 1;
	//avio_write(s, (const unsigned char*)"hello", 5);


	while (1) {
		//Get an AVPacket
		if (av_read_frame(ifmt_ctx, &pkt) < 0)
			break;
		if (pkt.stream_index == videoindex) {
//			printf("Write Video Packet. size:%d\tpts:%lld\n", pkt.size, pkt.pts);	
		}
		else {
			continue;
		}
		
		if (flag)
		{
			fwrite(startcode, 4, 1, fp);
			fwrite(sps, spsLength, 1, fp);
			fwrite(startcode, 4, 1, fp);
			fwrite(pps, ppsLength, 1, fp);

			pkt.data[0] = 0x00;
			pkt.data[1] = 0x00;
			pkt.data[2] = 0x00;
			pkt.data[3] = 0x01;
			fwrite(pkt.data, pkt.size, 1, fp);

			flag = 0;
		}
		else
		{
			pkt.data[0] = 0x00;
			pkt.data[1] = 0x00;
			pkt.data[2] = 0x00;
			pkt.data[3] = 0x01;
			fwrite(pkt.data, pkt.size, 1, fp);
		}

		av_free_packet(&pkt);
		frame_index++;
	}

	fclose(fp);
	fp = NULL;

	return 0;
}