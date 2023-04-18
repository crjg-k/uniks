#ifndef __TERMIOS_H__
#define __TERMIOS_H__


#define NCCS 17	  // length of control char array of in termios structure
// termios sturcture of POSIX
struct termios_t {
	unsigned long c_iflag;	    // 输入模式标志
	unsigned long c_oflag;	    // 输出模式标志
	unsigned long c_cflag;	    // 控制模式标志
	unsigned long c_lflag;	    // 本地模式标志
	unsigned char c_line;	    // 线路规程（速率）
	unsigned char c_cc[NCCS];   // 控制字符数组
};


#endif /* !__TERMIOS_H__ */