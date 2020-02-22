encode_video:encode_video.c
	gcc $< -o $@ -I /usr/local/ffmpeg/include/ -L /usr/local/ffmpeg/lib/ -lavcodec -lavutil -lswresample -L /usr/local/x264/lib/ -lx264 -L /usr/local/mp3lame/lib -lmp3lame
