
//can get mjpeg type usb camrea
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <fcntl.h> /* low-level i/o */
    #include <unistd.h>
    #include <errno.h>
    #include <malloc.h>
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <sys/time.h>
    #include <sys/mman.h>
    #include <sys/ioctl.h>
    #include <linux/videodev2.h>
    #define DEVICE "/dev/video0"
	#define CAP_FORMAT  V4L2_PIX_FMT_YUYV
    static struct v4l2_requestbuffers req;
    struct buffer
    {
        void* start;
        unsigned int length;
    }; 
    static struct buffer *buffers;
    static struct v4l2_buffer buf;
	int numb = 0;  
	#define REQ_BUF_CNT     4
	#define WIDTH	640
	#define HEIGH	480
	#define GETPICTURENUM	4


    int main()
    {
        int fd;
		
		
		
        fd=open_device();
        get_device_info(fd);
        get_frame_fmt(fd);
        get_current_frame_info(fd);
        try_format_support(fd);
        set_frame_format(fd);
		set_timeperframe(fd);
        apply_memory_buf(fd);
        memory_mapping(fd);
        buffer_enqueue(fd);
		check_camera(fd);
		get_picture(fd);
        close(fd);
        return 0;
    }
    int open_device()
    {
        int fd;
        if(-1==(fd=open(DEVICE,O_RDWR)))
		{
			 printf("info:Can't open video device\n");
			exit(-1);
		}
           
        else
            printf("info:Open the device :%d\n",fd);
        return fd;
    }
    int get_device_info(int fd)
    {
        struct v4l2_capability cap;
        if(-1==ioctl(fd,VIDIOC_QUERYCAP,&cap))
		{
			printf("info:VIDIOC_QUERYCAP ERROR\n");
			exit(-1);
		}
        else
            printf("info:Driver Name:%s....Card Name:%s....Bus info:%s....Driver Version:%u.%u.%u\n",
            cap.driver,cap.card,cap.bus_info,(cap.version>>16)&0XFF,(cap.version>>8)&0XFF,cap.version&0XFF);
        return 0;
    }
    int get_frame_fmt(int fd)
    {
        struct v4l2_fmtdesc fmtdesc;
        fmtdesc.index=0;
        fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
        printf("info:Support format:");
        while(ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc)!=-1)
        {
            printf("\t%d.%s",fmtdesc.index+1,fmtdesc.description);
            fmtdesc.index++;
        }
        printf("\n");
        return 0;
    }
    int get_current_frame_info(int fd)
    {
        struct v4l2_format fmt;
        fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(fd,VIDIOC_G_FMT,&fmt);
        printf("info:Current data format information:\n\twidth:%d\n\theight:%d\n\tsizeimage:%d\n",fmt.fmt.pix.width,fmt.fmt.pix.height,fmt.fmt.pix.sizeimage);
        struct v4l2_fmtdesc fmtdesc;
        fmtdesc.index=0;
        fmtdesc.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
        while(ioctl(fd,VIDIOC_ENUM_FMT,&fmtdesc)!=-1)
        {
            if(fmtdesc.pixelformat & fmt.fmt.pix.pixelformat)
            {
                printf("\tformat:%s\n",fmtdesc.description);
                break;
            }
            fmtdesc.index++;
        } 
        return 0;
    }
    int try_format_support(int fd)
    {
        struct v4l2_format fmt;
        fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.pixelformat=V4L2_PIX_FMT_YUYV ;
        if(ioctl(fd,VIDIOC_TRY_FMT,&fmt)==-1)
		{
			if(errno==EINVAL)
			printf("info:not support format MJPEG!\n"); 
			exit(-1);
			
		}
        
        return 0;
    }
    int set_frame_format(int fd)
    {
        struct v4l2_format fmt;
        fmt.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width=WIDTH;
        fmt.fmt.pix.height=HEIGH;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        fmt.fmt.pix.field = V4L2_FIELD_INTERLACED; 
        if(ioctl(fd,VIDIOC_S_FMT,&fmt)==-1)
		{
			if(errno==EINVAL)
			printf("info:set frame format error!\n");
			exit(-1);
		}
        printf("info:set frame format success!\n");
	
        return 0;
    }
	
	int set_timeperframe(int fd)
	{
		
		struct v4l2_streamparm parm;                                                                                    
		parm.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;                                                                      
		parm.parm.capture.timeperframe.numerator=1;                                                                   
		parm.parm.capture.timeperframe.denominator=18;                                                                 
		parm.parm.capture.capturemode=1;                                                                              
		if(ioctl(fd,VIDIOC_S_PARM,&parm)<0)                                                                       
		{                                                                                                           
			printf("VIDIOC_S_PARMfailed");                                                                       
			exit(-1);                                                                                       
		}                                                                                                               
		if(ioctl(fd,VIDIOC_G_PARM,&parm)<0)
		{                                                                                    
			printf("ErrorVIDIOC_S_PARM\n");                                                                             

			exit(-1);
		}                                                                                                               
		printf("streamparm:\n\tnumerator=%d\n\tdenominator=%d\n\tcapturemode=%d\n\n",parm.parm.capture.timeperframe.numerator,parm.parm.capture.timeperframe.denominator,parm.parm.capture.capturemode);        
		return 0;
}
    int apply_memory_buf(int fd)
    {
        
        req.count=REQ_BUF_CNT;
        req.type=V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory=V4L2_MEMORY_MMAP;
        if(-1==ioctl(fd,VIDIOC_REQBUFS,&req))
		{
			printf("info:VIDIOC_REQBUFS FAILED\n");
			exit(-1);
		}
            
        else
            printf("info:VIDIOC_REQBUFS SUCCESS\n");
        return 0;
    }
    int memory_mapping(int fd)
    {
        unsigned int n_buffers;
        buffers = (struct buffer*)calloc(req.count,sizeof(struct buffer));
        if (!buffers) {
            fprintf (stderr, "Out of memory\n");
            exit (EXIT_FAILURE);
        }
        // 映射
        for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
         
            
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = n_buffers;
            // 查询序号为n_buffers 的缓冲区，得到其起始物理地址和大小
            if (-1 == ioctl (fd, VIDIOC_QUERYBUF, &buf))
                exit(-1);
            buffers[n_buffers].length = buf.length;
            // 映射内存
            buffers[n_buffers].start =mmap (NULL,buf.length,PROT_READ | PROT_WRITE,MAP_SHARED,fd, buf.m.offset);
            if (MAP_FAILED == buffers[n_buffers].start)
                exit(-1);
        } 
        printf("info:memory mapping success\n");
        return 0;
    }
	
    int buffer_enqueue(int fd)
    {
        unsigned int i;
        enum v4l2_buf_type type;
        // 将缓冲帧放入队列
        for (i = 0; i < req.count; ++i)
        {
            //struct v4l2_buffer buf;
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            buf.index = i;
            if(-1==ioctl (fd, VIDIOC_QBUF, &buf))
			{
				printf("buffer enqueue failed\n");
				exit(-1);
			}
             
        }
		
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        //open stream 
        if(-1==ioctl (fd, VIDIOC_STREAMON, &type))
			{
				printf("info:open stream failed\n");
				exit(-1);
			}
            
        else
            printf("info:open stream success\n");
        return 0;
    }
int  check_camera(int fd)
{
	enum  v4l2_buf_type type;  
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
    fd_set fds;  
    struct timeval tv;  
    int r;  
  
    FD_ZERO(&fds);//将指定文件描述符集清空  
    FD_SET(fd, &fds);//在文件描述符集合中增加一个新的文件描述符  
    tv.tv_sec = 2;//time out  
    tv.tv_usec = 0;  
    r = select(fd+1, &fds, NULL, NULL, &tv);//判断摄像头是否准备好，tv是定时  
  
    if(-1 == r){  
        if(EINTR == errno){  
            printf("select erro! \n"); 
			exit(-1);			
        }  
    }  
    else if(0 == r){  
        printf("select timeout! \n");//超时  
        exit(-1); 
         
    }  
	printf("infor:check_camera camera is ok! \n");
     return 0;
    

	
}

	
     int get_picture(int fd)  
    {  
        
        int ret =0;  
        static char path1[30];  
		int i;
       // static char path2[30];  
        FILE *file_fd;//图片文件流  
         //test 6
		 for (i = 0; i < GETPICTURENUM; ++i)
        {
				
		  
			sprintf(path1, "./test_mmap%d.mjpg", numb);//文件名   
		      
			printf("path1=%s, %d\n", path1, numb);  
		  
			file_fd = fopen(path1, "w");//图片  
			if (file_fd < 0){  
				  perror("open test_mmap.jpg fialed! \n");  
					  exit(-1);       
			}  
			
			//struct v4l2_buffer buf;
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            
            ret=ioctl (fd, VIDIOC_DQBUF, &buf);
              if(0 != ret){  
				printf("VIDIOC_DQBUF failed!\n");  
				exit(-1);  
				} 
				
				
			
			printf("buf.index %d \n",buf.index);
			ret = fwrite(buffers[buf.index].start,buffers[buf.index].length, 1, file_fd);//将摄像头采集得到的yuyv数据写入文件中  
         
			if(ret <= 0){  
				printf("write yuyv failed!\n");  
				exit(-1);  
			ret=ioctl (fd, VIDIOC_QBUF, &buf);
              if(0 != ret){  
				printf("VIDIOC_QBUF failed!\n");  
				exit(-1);  
				
				} 
			}  
			  
			 
			fclose(file_fd);  
			numb ++;  	
			if(0!=check_camera(fd))
			{
				exit(-1);
				printf("infor check_camera failed!\n");
			}
			}
			
       
          
		printf("infor:get pictures success!\n");  
        return 0;  
    }  
	

	