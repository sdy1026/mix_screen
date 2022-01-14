#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scale.h"
#include "rotate.h"

#include <memory>

typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;
typedef __int64             int64_t;
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned __int64    uint64_t;


struct SMixVideoFrame
{
	int32_t x;
	int32_t y;
	int32_t w;
	int32_t h;
	uint8_t *data;
};

typedef struct SMixVideoFrame
{
	int32_t w;
	int32_t h;
	uint8_t *data;
}SVideoFrame;

void videoMix(std::shared_ptr<SMixVideoFrame> base_frame , 
					std::shared_ptr<SMixVideoFrame> first_frame, int32_t x, int32_t y)
{
	auto bigW = base_frame->w;
	auto bigH = base_frame->h;

	auto smallW = first_frame->w;
	auto smallH = first_frame->h;

	if (x+smallW>bigW)
	{
		x=bigW-smallW;
	}
	if (y+smallH>bigH)
	{
		y=bigH-smallH;
	}
	if (x<0)
	{
		x=0;
	}
	if (y<0)
	{
		y=0;
	}  
	auto dstOffset = bigW * y+x;
	for (int i = 0; i < smallH; ++i)
	{
		memcpy(base_frame->data + dstOffset + i * bigW, first_frame->data + i * smallW, smallW);
	}
	dstOffset = (bigW * bigH) +(y*bigW/4+x/4)+(x/4); 
	for (int i = 0; i < smallH / 2; ++i)
	{
		memcpy(base_frame->data + dstOffset+ i * bigW / 2, 
						first_frame->data +smallW*smallH+ i * smallW/2, smallW/2);
	}
	dstOffset = (bigW * bigH + (bigW * bigH) / 4) + (y * bigW / 4 +x/4)+x/4;
	for (int i = 0; i < smallH / 2; ++i)
	{
		memcpy(base_frame->data + dstOffset+ i * bigW / 2, 
					first_frame->data + smallW * smallH+smallW*smallH/4 + i * smallW / 2, smallW / 2);
	}
}

int main()
{
	uint8_t* parse_yuv11;
	int32_t src_width = 1280;
	int32_t src_height = 720;

	int big_width = src_width;
	int big_height = src_height;
	int small_width = 320;
	int small_height = 240;
	
	int half_width = src_width/2;
	int half_height= src_height/2;

	int bufsize11 = src_width * src_height * 1.5;
	parse_yuv11 = (uint8_t*)malloc(bufsize11);
	memset(parse_yuv11, 0x80, bufsize11);

	int bufSize_half =half_width*half_height*1.5;
	uint8_t *parse_yuv_half = (uint8_t*)malloc(bufSize_half);
	memset(parse_yuv_half, 0, bufSize_half);

	uint8_t* parse_yuv;
	parse_yuv = (uint8_t*)malloc(bufsize11);
	memset(parse_yuv, 0, bufsize11);

	int bufsize = small_width * small_height * 1.5;
	uint8_t* parse_yuv_small;
	parse_yuv_small = (uint8_t*)malloc(bufsize);
	memset(parse_yuv_small, 0, bufsize);

	FILE* fp_yuv;

	FILE* fp_yuv_scaled;

	FILE* fp_outyuv;
	FILE* fp_outyuv_2layout;
	FILE *fp_outyuv_2layout_tmp;
	fp_yuv = fopen("test_1280x720_30.yuv", "rb");
	fp_yuv_scaled =fopen("out_scaled.yuv","wb");
	fp_outyuv = fopen("outyuv.yuv", "wb");
	fp_outyuv_2layout = fopen("outyuv_2layout.yuv", "wb");
	fp_outyuv_2layout_tmp = fopen("outyuv_2layout_temp.yuv", "wb");

	for (int i = 0; i < 200; i++){
		fread(parse_yuv, sizeof(uint8_t), src_width * src_height * 3 / 2, fp_yuv);

		//memcpy(parse_yuv_small, parse_yuv, 640 * 480 * 3 / 2);

		//scale
		libyuv::I420Scale(parse_yuv, big_width,
			parse_yuv + big_width * big_height, big_width / 2,
			parse_yuv + (big_width * big_height * 5) / 4, big_width / 2,
			big_width, big_height,
			parse_yuv_small, small_width,
			parse_yuv_small + small_width * small_height, small_width / 2,
			parse_yuv_small + (small_width * small_height * 5) / 4, small_width / 2,
			small_width, small_height,
			libyuv::kFilterLinear);
		fwrite(parse_yuv_small, sizeof(uint8_t), (small_width * small_height * 3) / 2, fp_yuv_scaled);

		libyuv::I420Scale(parse_yuv, big_width,
			parse_yuv + big_width * big_height, big_width / 2,
			parse_yuv + (big_width * big_height * 5) / 4, big_width / 2,
			big_width, big_height,
			parse_yuv_half, half_width,
			parse_yuv_half + half_width * half_height, half_width / 2,
			parse_yuv_half + (half_width * half_height * 5) / 4, half_width / 2,
			half_width, half_height,
			libyuv::kFilterLinear);
		fwrite(parse_yuv_half, sizeof(uint8_t), (half_width * half_height * 3) / 2, fp_outyuv_2layout_tmp);
		
		//! mix left-right layout.
		int toTopHeight = (big_height - half_height)/2;

		auto yData = parse_yuv_half;
		auto uData = parse_yuv_half + half_width * half_height;
		auto vData = parse_yuv_half + half_width * half_height * 5 / 4;
		auto yStride = half_width;
		auto uStride = half_width / 2;
		auto vStride = half_width / 2;
		//! left.
		auto dstOffset = big_width * toTopHeight;
		for (int i = 0; i < half_height; ++i)
		{
			memcpy(parse_yuv11 + dstOffset + i * big_width, yData + i * half_width, yStride);
		}
		dstOffset = big_width * big_height + toTopHeight/2 * big_width / 2 ;
		for (int i = 0; i < half_height/2; ++i)
		{
			memcpy(parse_yuv11  + dstOffset + i * big_width/2 , uData + i * uStride, uStride);
		}
		dstOffset =big_width * big_height + (big_width * big_height)/4+ toTopHeight/2 * big_width / 2 ;;
 		for (int i = 0; i < half_height / 2; ++i)
		{
 			memcpy(parse_yuv11 + dstOffset + i * big_width/2, vData + i * vStride, vStride);
 		}
		//! right.
 		dstOffset = big_width * toTopHeight + big_width / 2;
 		for (int i = 0; i < half_height; ++i)
 		{
 			memcpy(parse_yuv11 + dstOffset + i * big_width, yData+i*half_width, yStride);
 		}
		dstOffset = big_width * big_height + toTopHeight/2 * big_width / 2;
		for (int i = 0; i < half_height / 2; ++i)
		{
			memcpy(parse_yuv11 + dstOffset + big_width /4 + i * big_width / 2 , uData + i * uStride, uStride);
		}
		dstOffset = big_width * big_height + (big_width * big_height) / 4 + toTopHeight * big_width / 4;
		for (int i = 0; i < half_height / 2; ++i)
		{
			memcpy(parse_yuv11 + dstOffset + big_width /4 +i * big_width / 2, vData + i * vStride, vStride);
		}

		fwrite(parse_yuv11, sizeof(uint8_t), (big_width * big_height * 3) / 2, fp_outyuv_2layout);

		//mix
		libyuv::RotatePlane(parse_yuv_small, small_width,
			parse_yuv + big_width *(big_height - small_height) + (big_width - small_width), big_width,
			small_width, small_height, libyuv::kRotate0);
		libyuv::RotatePlane(parse_yuv_small + small_width * small_height, small_width / 2,
			parse_yuv + big_width * big_height + big_width / 2 * (big_height - small_height) / 2 + (big_width - small_width) / 2, big_width / 2,
			small_width / 2, small_height / 2, libyuv::kRotate0);
		libyuv::RotatePlane(parse_yuv_small + small_width * small_height * 5 / 4, small_width / 2,
			parse_yuv + big_width * big_height * 5 / 4 + +big_width / 2 * (big_height - small_height) / 2 + (big_width - small_width) / 2, big_width / 2,
			small_width / 2, small_height / 2, libyuv::kRotate0);

		
		fwrite(parse_yuv, sizeof(uint8_t), (big_width * big_height * 3) / 2, fp_outyuv);

	}
	
	fclose(fp_outyuv_2layout_tmp);
	fclose(fp_yuv);
	fclose(fp_yuv_scaled);
	fclose(fp_outyuv_2layout);
	fclose(fp_outyuv);
	return 0;
}
