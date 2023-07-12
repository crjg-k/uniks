#ifndef __TERMIOS_H__
#define __TERMIOS_H__


#define NCCS 17	  // length of control char array of in termios structure
// termios sturcture of POSIX
struct termios_t {
	unsigned long c_iflag;	    // nput mode flags
	unsigned long c_oflag;	    // Output mode flags
	unsigned long c_cflag;	    // Control mode flags
	unsigned long c_lflag;	    // Local mode flags
	unsigned char c_line;	    // Line discipline (baud rate)
	unsigned char c_cc[NCCS];   // Control character array
};


#endif /* !__TERMIOS_H__ */