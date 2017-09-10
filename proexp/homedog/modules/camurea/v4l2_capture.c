/*
 *  V4L2 video capture example
 *
 *  This program can be used and distributed without restrictions.
 *
 *      This program is provided with the V4L2 API
 * see https://linuxtv.org/docs.php for more information
 */
/*
* temp support mjpeg and yuyv format
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>
#include <getopt.h>             /* getopt_long() */

#include <fcntl.h>              /* low-level i/o */
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <linux/videodev2.h>

#define CLEAR(x) memset(&(x), 0, sizeof(x))
#pragma pack(2) 
enum io_method {
        IO_METHOD_READ,
        IO_METHOD_MMAP,
        IO_METHOD_USERPTR,
};



struct buffer {
        void   *start;
        size_t  length;
};

static char            *dev_name;
static enum io_method   io = IO_METHOD_MMAP;
static int              fd = -1;
struct buffer          *buffers;
static unsigned int     n_buffers;
static int              out_buf;
static int              force_format;
unsigned int 			pixelformat_type;
static int              frame_count = 6;
int numb = 0;  
unsigned int WIDTH; 
unsigned int HEIGHT;



 struct BITMAPFILEHEADER  
{  
	unsigned short  bfType; // 位图文件的类型，必须为BM(1-2字节）  
    unsigned long  bfSize; // 位图文件的大小，以字节为单位（3-6字节）  
    unsigned short bfReserved1; // 位图文件保留字，必须为0(7-8字节）  
    unsigned short bfReserved2; // 位图文件保留字，必须为0(9-10字节）  
    unsigned long bfOffBits; // 位图数据的起始位置，以相对于位图（11-14字节）  
    // 文件头的偏移量表示，以字节为单位  
} bf;  


//3：位图信息头（40字节）  
//BMP位图信息头数据用于说明位图的尺寸等信息。  
 struct BITMAPINFOHEADER{  
    unsigned long biSize; // 本结构所占用字节数（15-18字节）  
    unsigned long  biWidth; // 位图的宽度，以像素为单位（19-22字节）  
    unsigned long  biHeight; // 位图的高度，以像素为单位（23-26字节）  
    unsigned short biPlanes; // 目标设备的级别，必须为1(27-28字节）  
    unsigned short biBitCount;// 每个像素所需的位数，必须是1（双色），（29-30字节）  
    // 4(16色），8(256色）16(高彩色)或24（真彩色）之一  
    unsigned long  biCompression; // 位图压缩类型，必须是 0（不压缩），（31-34字节）  
    // 1(BI_RLE8压缩类型）或2(BI_RLE4压缩类型）之一  
    unsigned long  biSizeImage; // 位图的大小，以字节为单位（35-38字节）  
    unsigned long  biXPelsPerMeter; // 位图水平分辨率，每米像素数（39-42字节）  
    unsigned long  biYPelsPerMeter; // 位图垂直分辨率，每米像素数（43-46字节)  
    unsigned long  biClrUsed;// 位图实际使用的颜色表中的颜色数（47-50字节）  
    unsigned long  biClrImportant;// 位图显示过程中重要的颜色数（51-54字节）  
} bi;

//判断范围

static void errno_exit(const char *s)
{
        fprintf(stderr, "%s error %d, %s\\n", s, errno, strerror(errno));
        exit(EXIT_FAILURE);
}

static int xioctl(int fh, int request, void *arg)
{
        int r;

        do {
                r = ioctl(fh, request, arg);
        } while (-1 == r && EINTR == errno);

        return r;
}
//查表法YUV TO RGB
short redAdjust[] = {
-161,-160,-159,-158,-157,-156,-155,-153,
-152,-151,-150,-149,-148,-147,-145,-144,
-143,-142,-141,-140,-139,-137,-136,-135,
-134,-133,-132,-131,-129,-128,-127,-126,
-125,-124,-123,-122,-120,-119,-118,-117,
-116,-115,-114,-112,-111,-110,-109,-108,
-107,-106,-104,-103,-102,-101,-100, -99,
 -98, -96, -95, -94, -93, -92, -91, -90,
 -88, -87, -86, -85, -84, -83, -82, -80,
 -79, -78, -77, -76, -75, -74, -72, -71,
 -70, -69, -68, -67, -66, -65, -63, -62,
 -61, -60, -59, -58, -57, -55, -54, -53,
 -52, -51, -50, -49, -47, -46, -45, -44,
 -43, -42, -41, -39, -38, -37, -36, -35,
 -34, -33, -31, -30, -29, -28, -27, -26,
 -25, -23, -22, -21, -20, -19, -18, -17,
 -16, -14, -13, -12, -11, -10,  -9,  -8,
  -6,  -5,  -4,  -3,  -2,  -1,   0,   1,
   2,   3,   4,   5,   6,   7,   9,  10,
  11,  12,  13,  14,  15,  17,  18,  19,
  20,  21,  22,  23,  25,  26,  27,  28,
  29,  30,  31,  33,  34,  35,  36,  37,
  38,  39,  40,  42,  43,  44,  45,  46,
  47,  48,  50,  51,  52,  53,  54,  55,
  56,  58,  59,  60,  61,  62,  63,  64,
  66,  67,  68,  69,  70,  71,  72,  74,
  75,  76,  77,  78,  79,  80,  82,  83,
  84,  85,  86,  87,  88,  90,  91,  92,
  93,  94,  95,  96,  97,  99, 100, 101,
 102, 103, 104, 105, 107, 108, 109, 110,
 111, 112, 113, 115, 116, 117, 118, 119,
 120, 121, 123, 124, 125, 126, 127, 128,
};

short greenAdjust1[] = {
  34,  34,  33,  33,  32,  32,  32,  31,
  31,  30,  30,  30,  29,  29,  28,  28,
  28,  27,  27,  27,  26,  26,  25,  25,
  25,  24,  24,  23,  23,  23,  22,  22,
  21,  21,  21,  20,  20,  19,  19,  19,
  18,  18,  17,  17,  17,  16,  16,  15,
  15,  15,  14,  14,  13,  13,  13,  12,
  12,  12,  11,  11,  10,  10,  10,   9,
   9,   8,   8,   8,   7,   7,   6,   6,
   6,   5,   5,   4,   4,   4,   3,   3,
   2,   2,   2,   1,   1,   0,   0,   0,
   0,   0,  -1,  -1,  -1,  -2,  -2,  -2,
  -3,  -3,  -4,  -4,  -4,  -5,  -5,  -6,
  -6,  -6,  -7,  -7,  -8,  -8,  -8,  -9,
  -9, -10, -10, -10, -11, -11, -12, -12,
 -12, -13, -13, -14, -14, -14, -15, -15,
 -16, -16, -16, -17, -17, -17, -18, -18,
 -19, -19, -19, -20, -20, -21, -21, -21,
 -22, -22, -23, -23, -23, -24, -24, -25,
 -25, -25, -26, -26, -27, -27, -27, -28,
 -28, -29, -29, -29, -30, -30, -30, -31,
 -31, -32, -32, -32, -33, -33, -34, -34,
 -34, -35, -35, -36, -36, -36, -37, -37,
 -38, -38, -38, -39, -39, -40, -40, -40,
 -41, -41, -42, -42, -42, -43, -43, -44,
 -44, -44, -45, -45, -45, -46, -46, -47,
 -47, -47, -48, -48, -49, -49, -49, -50,
 -50, -51, -51, -51, -52, -52, -53, -53,
 -53, -54, -54, -55, -55, -55, -56, -56,
 -57, -57, -57, -58, -58, -59, -59, -59,
 -60, -60, -60, -61, -61, -62, -62, -62,
 -63, -63, -64, -64, -64, -65, -65, -66,
};

short greenAdjust2[] = {
  74,  73,  73,  72,  71,  71,  70,  70,
  69,  69,  68,  67,  67,  66,  66,  65,
  65,  64,  63,  63,  62,  62,  61,  60,
  60,  59,  59,  58,  58,  57,  56,  56,
  55,  55,  54,  53,  53,  52,  52,  51,
  51,  50,  49,  49,  48,  48,  47,  47,
  46,  45,  45,  44,  44,  43,  42,  42,
  41,  41,  40,  40,  39,  38,  38,  37,
  37,  36,  35,  35,  34,  34,  33,  33,
  32,  31,  31,  30,  30,  29,  29,  28,
  27,  27,  26,  26,  25,  24,  24,  23,
  23,  22,  22,  21,  20,  20,  19,  19,
  18,  17,  17,  16,  16,  15,  15,  14,
  13,  13,  12,  12,  11,  11,  10,   9,
   9,   8,   8,   7,   6,   6,   5,   5,
   4,   4,   3,   2,   2,   1,   1,   0,
   0,   0,  -1,  -1,  -2,  -2,  -3,  -4,
  -4,  -5,  -5,  -6,  -6,  -7,  -8,  -8,
  -9,  -9, -10, -11, -11, -12, -12, -13,
 -13, -14, -15, -15, -16, -16, -17, -17,
 -18, -19, -19, -20, -20, -21, -22, -22,
 -23, -23, -24, -24, -25, -26, -26, -27,
 -27, -28, -29, -29, -30, -30, -31, -31,
 -32, -33, -33, -34, -34, -35, -35, -36,
 -37, -37, -38, -38, -39, -40, -40, -41,
 -41, -42, -42, -43, -44, -44, -45, -45,
 -46, -47, -47, -48, -48, -49, -49, -50,
 -51, -51, -52, -52, -53, -53, -54, -55,
 -55, -56, -56, -57, -58, -58, -59, -59,
 -60, -60, -61, -62, -62, -63, -63, -64,
 -65, -65, -66, -66, -67, -67, -68, -69,
 -69, -70, -70, -71, -71, -72, -73, -73,
};

short blueAdjust[] = {
-276,-274,-272,-270,-267,-265,-263,-261,
-259,-257,-255,-253,-251,-249,-247,-245,
-243,-241,-239,-237,-235,-233,-231,-229,
-227,-225,-223,-221,-219,-217,-215,-213,
-211,-209,-207,-204,-202,-200,-198,-196,
-194,-192,-190,-188,-186,-184,-182,-180,
-178,-176,-174,-172,-170,-168,-166,-164,
-162,-160,-158,-156,-154,-152,-150,-148,
-146,-144,-141,-139,-137,-135,-133,-131,
-129,-127,-125,-123,-121,-119,-117,-115,
-113,-111,-109,-107,-105,-103,-101, -99,
 -97, -95, -93, -91, -89, -87, -85, -83,
 -81, -78, -76, -74, -72, -70, -68, -66,
 -64, -62, -60, -58, -56, -54, -52, -50,
 -48, -46, -44, -42, -40, -38, -36, -34,
 -32, -30, -28, -26, -24, -22, -20, -18,
 -16, -13, -11,  -9,  -7,  -5,  -3,  -1,
   0,   2,   4,   6,   8,  10,  12,  14,
  16,  18,  20,  22,  24,  26,  28,  30,
  32,  34,  36,  38,  40,  42,  44,  46,
  49,  51,  53,  55,  57,  59,  61,  63,
  65,  67,  69,  71,  73,  75,  77,  79,
  81,  83,  85,  87,  89,  91,  93,  95,
  97,  99, 101, 103, 105, 107, 109, 112,
 114, 116, 118, 120, 122, 124, 126, 128,
 130, 132, 134, 136, 138, 140, 142, 144,
 146, 148, 150, 152, 154, 156, 158, 160,
 162, 164, 166, 168, 170, 172, 175, 177,
 179, 181, 183, 185, 187, 189, 191, 193,
 195, 197, 199, 201, 203, 205, 207, 209,
 211, 213, 215, 217, 219, 221, 223, 225,
 227, 229, 231, 233, 235, 238, 240, 242,
};
//判断范围
unsigned char clip(int val)
{
    if(val > 255)
    {
        return 255;
    }
    else if(val > 0)
    {
        return val;
    }
    else
    {
        return 0;
    }
}
//查表法YUV TO RGB
int YUYVToRGB_table(unsigned char *yuv, unsigned char *rgb, unsigned int width,unsigned int height)
{
    short y1=0, y2=0, u=0, v=0;
    unsigned char *pYUV = yuv;
    unsigned char *pGRB = rgb;
    int i=0;
    //int y=0,x=0,in=0,y0,out=0;
    int count =width * height /2;//width * height *2/4;

   for(i = 0; i < count; i++)
    {
        y1 = *pYUV++ ;
        u  = *pYUV++ ;
        y2 = *pYUV++ ;
        v  = *pYUV++ ;

        *pGRB++ = clip(y1 + redAdjust[v]);
        *pGRB++ = clip(y1 + greenAdjust1[u] + greenAdjust2[v]);
        *pGRB++ = clip(y1 + blueAdjust[u]);
        *pGRB++ = clip(y2 + redAdjust[v]);
        *pGRB++ = clip(y2 + greenAdjust1[u] + greenAdjust2[v]);
        *pGRB++ = clip(y2 + blueAdjust[u]);
    }
  return 0;
}

    void init_bmp()  
    {  
      
        //Set BITMAPINFOHEADER  
		printf("aaaaaa %d bbbbbb %d\n",sizeof(bi),sizeof(bf));
		bi.biSize = 40;  
        bi.biWidth = WIDTH;  
        bi.biHeight = HEIGHT;  
        bi.biPlanes = 1;  
        bi.biBitCount = 24;  
        bi.biCompression = 0;  
        bi.biSizeImage = WIDTH*HEIGHT*3;  
        bi.biXPelsPerMeter = 0;  
        bi.biYPelsPerMeter = 0;  
        bi.biClrUsed = 0;  
        bi.biClrImportant = 0;  
       
      
        //Set BITMAPFILEHEADER  
            bf.bfType = 0x4d42;  
            bf.bfSize = 54 + bi.biSizeImage;       
            bf.bfReserved1 = 0;  
			bf.bfReserved2 = 0; 
            bf.bfOffBits = 54;  
      
    }  
	

static void process_image(const void *p, int size)
{
        //if (out_buf)
              //  fwrite(p, size, 1, stdout);

       // fflush(stderr);
       // fprintf(stderr, ".");
       // fflush(stdout);
	   
	   int ret = 0;
	   static char path1[30];  
		int i;
       // static char path2[30];  
        FILE *file_fd;//图片文件流 
		
		if(pixelformat_type==V4L2_PIX_FMT_MJPEG)
		{
			sprintf(path1, "./test_mmap%d.mjpg", numb);//文件名   
		}
		else if (pixelformat_type==V4L2_PIX_FMT_YUYV)
		{
			sprintf(path1, "./test_mmap%d.bmp", numb);//文件名  
		}
		
		      
			printf("path1=%s, %d\n", path1, numb);  
		  
			file_fd = fopen(path1, "w");//图片  
			if (file_fd < 0){  
				  perror("open test_mmap file fialed! \n");  
					  exit(-1);       
			}  
			if(pixelformat_type==V4L2_PIX_FMT_MJPEG)
			{
				ret = fwrite(p, size, 1, file_fd);//将摄像头采集得到的yuyv数据写入文件中  
				
				if(ret <= 0){  
				printf("write mjpeg failed!\n");  
				exit(-1); 
				}
			}
			else if (pixelformat_type==V4L2_PIX_FMT_YUYV)
			{
				unsigned char* rgbbuf = (unsigned char*) malloc(WIDTH*HEIGHT*3);
				memset(rgbbuf,0,WIDTH*HEIGHT*3);
				YUYVToRGB_table((unsigned char*)p,rgbbuf,WIDTH,HEIGHT);
				
				init_bmp();  
				fwrite(&bf, 14, 1, file_fd);  
				fwrite(&bi, 40, 1, file_fd);      
				ret=fwrite(rgbbuf, bi.biSizeImage, 1, file_fd); 
				

				if(ret <= 0){  
				printf("write rgb failed!\n");  
				exit(-1);  
				}
				
			}
			
         
			
}

static int read_frame(void)
{
        struct v4l2_buffer buf;
        unsigned int i;

        switch (io) {
        case IO_METHOD_READ:
                if (-1 == read(fd, buffers[0].start, buffers[0].length)) {
                        switch (errno) {
                        case EAGAIN:
                                return 0;

                        case EIO:
                                /* Could ignore EIO, see spec. */

                                /* fall through */

                        default:
                                errno_exit("read");
                        }
                }

                process_image(buffers[0].start, buffers[0].length);
				
                break;

        case IO_METHOD_MMAP:
                CLEAR(buf);

                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;

                if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
                        switch (errno) {
                        case EAGAIN:
                                return 0;

                        case EIO:
                                /* Could ignore EIO, see spec. */

                                /* fall through */

                        default:
                                errno_exit("VIDIOC_DQBUF");
                        }
                }

                assert(buf.index < n_buffers);

                process_image(buffers[buf.index].start, buf.bytesused);
				numb++;
                if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                        errno_exit("VIDIOC_QBUF");
                break;

        case IO_METHOD_USERPTR:
                CLEAR(buf);

                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_USERPTR;

                if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {
                        switch (errno) {
                        case EAGAIN:
                                return 0;

                        case EIO:
                                /* Could ignore EIO, see spec. */

                                /* fall through */

                        default:
                                errno_exit("VIDIOC_DQBUF");
                        }
                }

                for (i = 0; i < n_buffers; ++i)
                        if (buf.m.userptr == (unsigned long)buffers[i].start
                            && buf.length == buffers[i].length)
                                break;

                assert(i < n_buffers);

                process_image((void *)buf.m.userptr, buf.bytesused);

                if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                        errno_exit("VIDIOC_QBUF");
                break;
        }

        return 1;
}

static void mainloop(void)
{
        unsigned int count;

        count = frame_count;

        while (count-- > 0) {
                for (;;) {
                        fd_set fds;
                        struct timeval tv;
                        int r;

                        FD_ZERO(&fds);
                        FD_SET(fd, &fds);

                        /* Timeout. */
                        tv.tv_sec = 2;
                        tv.tv_usec = 0;

                        r = select(fd + 1, &fds, NULL, NULL, &tv);

                        if (-1 == r) {
                                if (EINTR == errno)
                                        continue;
                                errno_exit("select");
                        }

                        if (0 == r) {
                                fprintf(stderr, "select timeout\\n");
                                exit(EXIT_FAILURE);
                        }

                        if (read_frame())
                                break;
                        /* EAGAIN - continue select loop. */
                }
        }
}

static void stop_capturing(void)
{
        enum v4l2_buf_type type;

        switch (io) {
        case IO_METHOD_READ:
                /* Nothing to do. */
                break;

        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
                type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))
                        errno_exit("VIDIOC_STREAMOFF");
                break;
        }
}

static void start_capturing(void)
{
        unsigned int i;
        enum v4l2_buf_type type;

        switch (io) {
        case IO_METHOD_READ:
                /* Nothing to do. */
                break;

        case IO_METHOD_MMAP:
                for (i = 0; i < n_buffers; ++i) {
                        struct v4l2_buffer buf;

                        CLEAR(buf);
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_MMAP;
                        buf.index = i;

                        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                                errno_exit("VIDIOC_QBUF");
                }
                type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
                        errno_exit("VIDIOC_STREAMON");
                break;

        case IO_METHOD_USERPTR:
                for (i = 0; i < n_buffers; ++i) {
                        struct v4l2_buffer buf;

                        CLEAR(buf);
                        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                        buf.memory = V4L2_MEMORY_USERPTR;
                        buf.index = i;
                        buf.m.userptr = (unsigned long)buffers[i].start;
                        buf.length = buffers[i].length;

                        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))
                                errno_exit("VIDIOC_QBUF");
                }
                type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))
                        errno_exit("VIDIOC_STREAMON");
                break;
        }
}

static void uninit_device(void)
{
        unsigned int i;

        switch (io) {
        case IO_METHOD_READ:
                free(buffers[0].start);
                break;

        case IO_METHOD_MMAP:
                for (i = 0; i < n_buffers; ++i)
                        if (-1 == munmap(buffers[i].start, buffers[i].length))
                                errno_exit("munmap");
                break;

        case IO_METHOD_USERPTR:
                for (i = 0; i < n_buffers; ++i)
                        free(buffers[i].start);
                break;
        }

        free(buffers);
}

static void init_read(unsigned int buffer_size)
{
        buffers = calloc(1, sizeof(*buffers));

        if (!buffers) {
                fprintf(stderr, "Out of memory\\n");
                exit(EXIT_FAILURE);
        }

        buffers[0].length = buffer_size;
        buffers[0].start = malloc(buffer_size);

        if (!buffers[0].start) {
                fprintf(stderr, "Out of memory\\n");
                exit(EXIT_FAILURE);
        }
}

static void init_mmap(void)
{
        struct v4l2_requestbuffers req;

        CLEAR(req);

        req.count = 4;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;

        if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s does not support "
                                 "memory mappingn", dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_REQBUFS");
                }
        }

        if (req.count < 2) {
                fprintf(stderr, "Insufficient buffer memory on %s\\n",
                         dev_name);
                exit(EXIT_FAILURE);
        }

        buffers = calloc(req.count, sizeof(*buffers));

        if (!buffers) {
                fprintf(stderr, "Out of memory\\n");
                exit(EXIT_FAILURE);
        }

        for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
                struct v4l2_buffer buf;

                CLEAR(buf);

                buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory      = V4L2_MEMORY_MMAP;
                buf.index       = n_buffers;

                if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf))
                        errno_exit("VIDIOC_QUERYBUF");

                buffers[n_buffers].length = buf.length;
                buffers[n_buffers].start =
                        mmap(NULL /* start anywhere */,
                              buf.length,
                              PROT_READ | PROT_WRITE /* required */,
                              MAP_SHARED /* recommended */,
                              fd, buf.m.offset);

                if (MAP_FAILED == buffers[n_buffers].start)
                        errno_exit("mmap");
        }
}

static void init_userp(unsigned int buffer_size)
{
        struct v4l2_requestbuffers req;

        CLEAR(req);

        req.count  = 4;
        req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_USERPTR;

        if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s does not support "
                                 "user pointer i/on", dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_REQBUFS");
                }
        }

        buffers = calloc(4, sizeof(*buffers));

        if (!buffers) {
                fprintf(stderr, "Out of memory\\n");
                exit(EXIT_FAILURE);
        }

        for (n_buffers = 0; n_buffers < 4; ++n_buffers) {
                buffers[n_buffers].length = buffer_size;
                buffers[n_buffers].start = malloc(buffer_size);

                if (!buffers[n_buffers].start) {
                        fprintf(stderr, "Out of memory\\n");
                        exit(EXIT_FAILURE);
                }
        }
}

static void init_device(void)
{
        struct v4l2_capability cap;
        struct v4l2_cropcap cropcap;
        struct v4l2_crop crop;
        struct v4l2_format fmt;
        unsigned int min;

        if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {
                if (EINVAL == errno) {
                        fprintf(stderr, "%s is no V4L2 device\\n",
                                 dev_name);
                        exit(EXIT_FAILURE);
                } else {
                        errno_exit("VIDIOC_QUERYCAP");
                }
        }

        if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
                fprintf(stderr, "%s is no video capture device\\n",
                         dev_name);
                exit(EXIT_FAILURE);
        }

        switch (io) {
        case IO_METHOD_READ:
                if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
                        fprintf(stderr, "%s does not support read i/o\\n",
                                 dev_name);
                        exit(EXIT_FAILURE);
                }
                break;

        case IO_METHOD_MMAP:
        case IO_METHOD_USERPTR:
                if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
                        fprintf(stderr, "%s does not support streaming i/o\\n",
                                 dev_name);
                        exit(EXIT_FAILURE);
                }
                break;
        }


        /* Select video input, video standard and tune here. */


        CLEAR(cropcap);

        cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        if (0 == xioctl(fd, VIDIOC_CROPCAP, &cropcap)) {
                crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                crop.c = cropcap.defrect; /* reset to default */

                if (-1 == xioctl(fd, VIDIOC_S_CROP, &crop)) {
                        switch (errno) {
                        case EINVAL:
                                /* Cropping not supported. */
                                break;
                        default:
                                /* Errors ignored. */
                                break;
                        }
                }
        } else {
                /* Errors ignored. */
        }


        CLEAR(fmt);

        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (force_format) {
                fmt.fmt.pix.width       = 640;
                fmt.fmt.pix.height      = 480;
                fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
                fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

                if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
                        errno_exit("VIDIOC_S_FMT");

                /* Note VIDIOC_S_FMT may change width and height. */
        } else {
                /* Preserve original settings as set by v4l2-ctl for example */
                if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt))
                        errno_exit("VIDIOC_G_FMT");
        }
		pixelformat_type = fmt.fmt.pix.pixelformat;
        /* Buggy driver paranoia. */
        min = fmt.fmt.pix.width * 2;
		WIDTH=fmt.fmt.pix.width;
		HEIGHT	 =fmt.fmt.pix.height;
        if (fmt.fmt.pix.bytesperline < min)
                fmt.fmt.pix.bytesperline = min;
        min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;
        if (fmt.fmt.pix.sizeimage < min)
                fmt.fmt.pix.sizeimage = min;

        switch (io) {
        case IO_METHOD_READ:
                init_read(fmt.fmt.pix.sizeimage);
                break;

        case IO_METHOD_MMAP:
                init_mmap();
                break;

        case IO_METHOD_USERPTR:
                init_userp(fmt.fmt.pix.sizeimage);
                break;
        }
}

static void close_device(void)
{
        if (-1 == close(fd))
                errno_exit("close");

        fd = -1;
}

static void open_device(void)
{
        struct stat st;

        if (-1 == stat(dev_name, &st)) {
                fprintf(stderr, "Cannot identify '%s': %d, %s\\n",
                         dev_name, errno, strerror(errno));
                exit(EXIT_FAILURE);
        }

        if (!S_ISCHR(st.st_mode)) {
                fprintf(stderr, "%s is no devicen", dev_name);
                exit(EXIT_FAILURE);
        }

        fd = open(dev_name, O_RDWR /* required */ | O_NONBLOCK, 0);

        if (-1 == fd) {
                fprintf(stderr, "Cannot open '%s': %d, %s\\n",
                         dev_name, errno, strerror(errno));
                exit(EXIT_FAILURE);
        }
}

static void usage(FILE *fp, int argc, char **argv)
{
        fprintf(fp,
                 "Usage: %s [options]\\n\\n"
                 "Version 1.3\\n"
                 "Options:\\n"
                 "-d | --device name   Video device name [%s]n"
                 "-h | --help          Print this messagen"
                 "-m | --mmap          Use memory mapped buffers [default]n"
                 "-r | --read          Use read() callsn"
                 "-u | --userp         Use application allocated buffersn"
                 "-o | --output        Outputs stream to stdoutn"
                 "-f | --format        Force format to 640x480 YUYVn"
                 "-c | --count         Number of frames to grab [%i]n"
                 "",
                 argv[0], dev_name, frame_count);
}

static const char short_options[] = "d:hmruofc:";

static const struct option
long_options[] = {
        { "device", required_argument, NULL, 'd' },
        { "help",   no_argument,       NULL, 'h' },
        { "mmap",   no_argument,       NULL, 'm' },
        { "read",   no_argument,       NULL, 'r' },
        { "userp",  no_argument,       NULL, 'u' },
        { "output", no_argument,       NULL, 'o' },
        { "format", no_argument,       NULL, 'f' },
        { "count",  required_argument, NULL, 'c' },
        { 0, 0, 0, 0 }
};

int main(int argc, char **argv)
{
        dev_name = "/dev/video0";

        for (;;) {
                int idx;
                int c;

                c = getopt_long(argc, argv,
                                short_options, long_options, &idx);

                if (-1 == c)
                        break;

                switch (c) {
                case 0: /* getopt_long() flag */
                        break;

                case 'd':
                        dev_name = optarg;
                        break;

                case 'h':
                        usage(stdout, argc, argv);
                        exit(EXIT_SUCCESS);

                case 'm':
                        io = IO_METHOD_MMAP;
                        break;

                case 'r':
                        io = IO_METHOD_READ;
                        break;

                case 'u':
                        io = IO_METHOD_USERPTR;
                        break;

                case 'o':
                        out_buf++;
                        break;

                case 'f':
                        force_format++;
                        break;

                case 'c':
                        errno = 0;
                        frame_count = strtol(optarg, NULL, 0);
                        if (errno)
                                errno_exit(optarg);
                        break;

                default:
                        usage(stderr, argc, argv);
                        exit(EXIT_FAILURE);
                }
        }

        open_device();
        init_device();
        start_capturing();
        mainloop();
        stop_capturing();
        uninit_device();
        close_device();
        fprintf(stderr, "\\n");
        return 0;
}