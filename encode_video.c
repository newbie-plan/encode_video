#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>

static void encode(AVCodecContext *cdc_ctx, AVFrame *frame, AVPacket *pkt, FILE *fp_out)
{
	int ret = 0;

	if (frame != NULL)
		printf("Send %d frame.\n", frame->pts);

	if ((ret = avcodec_send_frame(cdc_ctx, frame)) < 0)
	{
		fprintf(stderr, "avcodec_send_frame failed.\n");
		exit(1);
	}

	while ((ret = avcodec_receive_packet(cdc_ctx, pkt)) >= 0)
	{
		printf("Write %d packet.\n", pkt->pts);
		fwrite(pkt->data, 1, pkt->size, fp_out);
		av_packet_unref(pkt);
	}

	if ((ret != AVERROR(EAGAIN)) && (ret != AVERROR_EOF))
	{
		fprintf(stderr, "avcodec_receive_packet failed.\n");
		exit(1);
	}
}

void encode_video(const char *input_file, const char *output_file, const char *encoder_name)
{
	int ret = 0;
	int i = 0;
	AVCodec *codec = NULL;
	AVCodecContext *cdc_ctx = NULL;
	AVPacket *pkt = NULL;
	AVFrame *frame = NULL;
	FILE *fp_in, *fp_out;

	if ((codec = avcodec_find_encoder_by_name(encoder_name)) == NULL)
	{
		fprintf(stderr, "avcodec_find_encoder_by_name failed.\n");
		goto ret1;
	}

	if ((cdc_ctx = avcodec_alloc_context3(codec)) == NULL)
	{
		fprintf(stderr, "avcodec_alloc_context3 failed.\n");
		goto ret1;
	}
	cdc_ctx->bit_rate = 400000;
	cdc_ctx->width = 352;
	cdc_ctx->height = 288;
	cdc_ctx->time_base = (AVRational){1,25};
	cdc_ctx->framerate = (AVRational){25,1};
	cdc_ctx->gop_size = 10;
	cdc_ctx->max_b_frames = 1;
	cdc_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
	if (codec->id == AV_CODEC_ID_H264)
		av_opt_set(cdc_ctx->priv_data, "preset", "slow", 0);

	if ((ret = avcodec_open2(cdc_ctx, codec, NULL)) < 0)
	{
		fprintf(stderr, "avcodec_open2 failed.\n");
		goto ret2;
	}

	if ((pkt = av_packet_alloc()) == NULL)
	{
		fprintf(stderr, "av_packet_alloc failed.\n");
		goto ret3;
	}

	if ((frame = av_frame_alloc()) == NULL)
	{
		fprintf(stderr, "av_frame_alloc failed.\n");
		goto ret4;
	}
	frame->format = cdc_ctx->pix_fmt;
	frame->width = cdc_ctx->width;
	frame->height = cdc_ctx->height;

	if ((ret = av_frame_get_buffer(frame, 0)) < 0)
	{
		fprintf(stderr, "av_frame_get_buffer failed.\n");
		goto ret5;
	}

	if ((fp_in = fopen(input_file, "rb")) == NULL)
	{
		fprintf(stderr, "fopen %s failed.\n", input_file);
		goto ret5;
	}
	if ((fp_out = fopen(output_file, "wb")) == NULL)
	{
		fprintf(stderr, "fopen %s failed.\n", output_file);
		goto ret6;
	}

	while (feof(fp_in) == 0)
	{
		int y = 0;

		if ((ret = av_frame_make_writable(frame)) < 0)
		{
			fprintf(stderr, "frame is not writable.\n");
			goto ret7;
		}

		/*y*/
		for (y = 0; y < frame->height; y++)
			fread(&frame->data[0][y * frame->linesize[0]], 1, frame->width, fp_in);
		/*u*/
		for (y = 0; y < frame->height / 2; y++)
			fread(&frame->data[1][y * frame->linesize[1]], 1, frame->width / 2, fp_in);
		/*v*/
		for (y = 0; y < frame->height / 2; y++)
			fread(&frame->data[2][y * frame->linesize[2]], 1, frame->width / 2, fp_in);

		frame->pts = i++;

		encode(cdc_ctx, frame, pkt, fp_out);
	}

	encode(cdc_ctx, NULL, pkt, fp_out);


	fclose(fp_out);
	fclose(fp_in);
	av_frame_free(&frame);
	av_packet_free(&pkt);
	avcodec_close(cdc_ctx);
	avcodec_free_context(&cdc_ctx);
	return;
ret7:
	fclose(fp_out);
ret6:
	fclose(fp_in);
ret5:
	av_frame_free(&frame);
ret4:
	av_packet_free(&pkt);
ret3:
	avcodec_close(cdc_ctx);
ret2:
	avcodec_free_context(&cdc_ctx);
ret1:
	exit(1);
}

int main(int argc, const char *argv[])
{
	if (argc < 4)
	{
		fprintf(stderr, "Uage:<input file> <output file> <encoder_name>\n");
		exit(0);
	}

	encode_video(argv[1], argv[2], argv[3]);
	
	return 0;
}
