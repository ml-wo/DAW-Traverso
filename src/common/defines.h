#ifndef TRAVERSO_TYPES_H
#define TRAVERSO_TYPES_H

#include <inttypes.h>
#include <QtGlobal>
#include <QMetaType>
#include "FastDelegate.h"

// Implementation for atomic int get/set from glibc's atomic.h/c
// to get rid of the glib dependency!
// Arches that need memory bariers: ppc (which we support)
// and sparc, alpha, ia64 which we do not support ??

#if defined(__ppc__) || defined(__powerpc__) || defined(__PPC__)

#  define T_ATOMIC_MEMORY_BARRIER __asm__ ("sync" : : : "memory")

static inline int t_atomic_int_get (volatile int *atomic)
{
	T_ATOMIC_MEMORY_BARRIER;
	return *atomic;
}

static inline void t_atomic_int_set (volatile int *atomic, int newval)
{
	*atomic = newval;
	T_ATOMIC_MEMORY_BARRIER; 
}

#else

# define t_atomic_int_get(atomic) 		(*(atomic))
# define t_atomic_int_set(atomic, newval) 	((void) (*(atomic) = (newval)))

#endif // ENDIF __ppc__


using namespace fastdelegate;

/**
 * Type used to represent sample frame counts.
 */
typedef uint32_t     nframes_t;

enum {
	TransportStopped = 0,
	TransportRolling = 1,
  	TransportLooping = 2,
  	TransportStarting = 3
};

// Universal samplerate for the frequences 22050, 32000, 44100, 88200, 96000 and 192000 Hz
static const qint64 UNIVERSAL_SAMPLE_RATE = 28224000;
static const qint64 ONE_HOUR_UNIVERSAL_SAMPLE_RATE = 101606400000LL;
static const qint64 ONE_MINUTE_UNIVERSAL_SAMPLE_RATE = 1693440000LL;

struct TimeRef {
	
	TimeRef() {
		m_position = 0;
	}
	explicit TimeRef(qint64 position) : m_position(position) {}
	explicit TimeRef(double position) : m_position(qint64(position)) {}
	
	TimeRef(nframes_t frame, int rate) {
		m_position = (UNIVERSAL_SAMPLE_RATE / rate) * frame;
	}
	
	void add_frames(nframes_t frames, int rate) {
		m_position += ((UNIVERSAL_SAMPLE_RATE / rate) * frames);
	}
	
	nframes_t to_frame(int rate) {
		Q_ASSERT(rate);
		return nframes_t(m_position / (UNIVERSAL_SAMPLE_RATE / rate));
	}
	
	qint64 universal_frame() const {return m_position;}
	
	TimeRef operator =(qint64 value) {
		TimeRef ref(value);
		return ref;
	}
	
	friend int operator!=(const TimeRef& left, const TimeRef& right) {
		return left.m_position != right.m_position;
	}
	
	friend int operator!=(const TimeRef& left, qint64 right) {
		return left.m_position != right;
	}
	
	friend int operator!=(const TimeRef& left, double right) {
		return left.m_position != qint64(right);
	}
	
	friend TimeRef operator-(const TimeRef& left, const TimeRef& right) {
		return TimeRef(left.m_position - right.m_position);
	}
	
	friend TimeRef operator-(const TimeRef& left, qint64 right) {
		TimeRef location(left.m_position - right);
		return location;
	}
	
	friend TimeRef operator-(const TimeRef& left, double right) {
		TimeRef location(left.m_position - qint64(right));
		return location;
	}
	
	friend TimeRef& operator-=(TimeRef& left, const TimeRef& right) {
		left.m_position -= right.m_position;
		return left;
	}
	
	friend TimeRef& operator-=(TimeRef& left, qint64 right) {
		left.m_position -= right;
		return left;
	}
	
	friend TimeRef& operator-=(TimeRef& left, double right) {
		left.m_position -= qint64(right);
		return left;
	}
	
	friend TimeRef operator+(const TimeRef& left, const TimeRef& right) {
		return TimeRef(left.m_position + right.m_position);
	}
	
	friend TimeRef operator+(const TimeRef& left, qint64 right) {
		TimeRef location(left.m_position + right);
		return location;
	}
	
	friend TimeRef operator+(const TimeRef& left, double right) {
		TimeRef location(left.m_position + qint64(right));
		return location;
	}
	
	friend TimeRef& operator+=(TimeRef& left, const TimeRef& right) {
		left.m_position += right.m_position;
		return left;
	}
	
	friend TimeRef& operator+=(TimeRef& left, qint64 right) {
		left.m_position += right;
		return left;
	}
	
	friend TimeRef& operator+=(TimeRef& left, double right) {
		left.m_position += qint64(right);
		return left;
	}
	
	friend TimeRef operator/(const TimeRef& left, const TimeRef& right) {
		Q_ASSERT(right.m_position != 0);
		TimeRef location(left.m_position / right.m_position);
		return location;
	}
	
	friend qreal operator/(const TimeRef& left, const qint64 right) {
		Q_ASSERT(right != 0);
		return (qreal)left.m_position / right;
	}
	
	friend qreal operator/(const TimeRef& left, double right) {
		Q_ASSERT(right != 0);
		return (qreal)left.m_position / qint64(right);
	}
	
	friend TimeRef operator*(const qint64 left, TimeRef& right) {
		TimeRef location(left * right.m_position);
		return location;
	}
	
	friend TimeRef operator*(const TimeRef& left, const TimeRef& right) {
		TimeRef location(left.m_position * right.m_position);
		return location;
	}
	
	friend TimeRef operator*(const TimeRef& left, double right) {
		TimeRef location(left.m_position * qint64(right));
		return location;
	}
	
	friend int operator<(const TimeRef& left, const TimeRef& right) {
		return left.m_position < right.m_position;
	}
	
	friend int operator<(const TimeRef& left, qint64 right) {
		return left.m_position < right;
	}
	
	friend int operator<(const TimeRef& left, double right) {
		return left.m_position < qint64(right);
	}
	
	friend int operator>(const TimeRef& left, const TimeRef& right) {
		return left.m_position > right.m_position;
	}
	
	friend int operator>(const TimeRef& left, qint64 right) {
		return left.m_position > right;
	}
	
	friend int operator>(const TimeRef& left, double right) {
		return left.m_position > qint64(right);
	}
	
	friend int operator<=(const TimeRef& left, const TimeRef& right) {
		return left.m_position <= right.m_position;
	}
	
	friend int operator<=(const TimeRef& left, qint64 right) {
		return left.m_position <= right;
	}
	
	friend int operator<=(const TimeRef& left, double right) {
		return left.m_position <= qint64(right);
	}
	
	friend int operator>=(const TimeRef& left, const TimeRef& right) {
		return left.m_position >= right.m_position;
	}
	
	friend int operator>=(const TimeRef& left, qint64 right) {
		return left.m_position >= right;
	}
	
	friend int operator>=(const TimeRef& left, double right) {
		return left.m_position >= qint64(right);
	}
	
	friend int operator==(const TimeRef& left, const TimeRef& right) {
		return left.m_position == right.m_position;
	}
	
	friend int operator==(const TimeRef& left, qint64 right) {
		return left.m_position == right;
	}
	
	friend int operator==(const TimeRef& left, double right) {
		return left.m_position == qint64(right);
	}
	
private:
	qint64 m_position;
};

Q_DECLARE_METATYPE(TimeRef);
		
typedef struct {
	int tranport;
	bool isSlave;
	bool realtime;
	TimeRef location;
} transport_state_t;

/**
 * Type used to represent the value of free running
 * monotonic clock with units of microseconds.
 */
typedef double trav_time_t;

typedef unsigned long          channel_t;

typedef float audio_sample_t;
typedef unsigned char peak_data_t;
// typedef short peak_data_t;


typedef FastDelegate1<nframes_t, int> ProcessCallback;
typedef FastDelegate0<int> RunCycleCallback;
typedef FastDelegate1<transport_state_t, int> TransportControlCallback;


/**
 * Used for the type argument of jack_port_register() for default
 * audio ports.
 */
#define JACK_DEFAULT_AUDIO_TYPE "32 bit float mono audio"


/**
 *  A port has a set of flags that are formed by AND-ing together the
 *  desired values from the list below. The flags "PortIsInput" and
 *  "PortIsOutput" are mutually exclusive and it is an error to use
 *  them both.
 */
enum PortFlags {

     /**
	 * if PortIsInput is set, then the port can receive
	 * data.
      */
	PortIsInput = 0x1,

     /**
  * if PortIsOutput is set, then data can be read from
  * the port.
      */
 PortIsOutput = 0x2,

     /**
  * if PortIsPhysical is set, then the port corresponds
  * to some kind of physical I/O connector.
      */
 PortIsPhysical = 0x4,

     /**
  * if PortCanMonitor is set, then a call to
  * jack_port_request_monitor() makes sense.
  *
  * Precisely what this means is dependent on the client. A typical
  * result of it being called with TRUE as the second argument is
  * that data that would be available from an output port (with
  * PortIsPhysical set) is sent to a physical output connector
  * as well, so that it can be heard/seen/whatever.
  *
  * Clients that do not control physical interfaces
  * should never create ports with this bit set.
      */
 PortCanMonitor = 0x8,

     /**
      * PortIsTerminal means:
  *
  *	for an input port: the data received by the port
  *                    will not be passed on or made
  *		           available at any other port
  *
  * for an output port: the data available at the port
  *                    does not originate from any other port
  *
  * Audio synthesizers, I/O hardware interface clients, HDR
  * systems are examples of clients that would set this flag for
  * their ports.
      */
 PortIsTerminal = 0x10
};


#if defined(_MSC_VER) || defined(__MINGW32__)
#  include <time.h>
#ifndef _TIMEVAL_DEFINED /* also in winsock[2].h */
#define _TIMEVAL_DEFINED
struct timeval {
   long tv_sec;
   long tv_usec;
};
#endif /* _TIMEVAL_DEFINED */
#else
#  include <sys/time.h>
#endif


#if defined(_MSC_VER) || defined(__MINGW32__)

#include <Windows.h>

static inline int gettimeofday(struct timeval* tp, void* tzp) {
	DWORD t;
	t = timeGetTime();
	tp->tv_sec = t / 1000;
	tp->tv_usec = t % 1000;
	/* 0 indicates that the call succeeded. */
	return 0;
}
	
typedef uint8_t            u_int8_t;

#endif

static inline trav_time_t get_microseconds()
{
	struct timeval now;
	gettimeofday(&now, 0);
	trav_time_t time = (now.tv_sec * 1000000.0 + now.tv_usec);
	return time;
}

#if defined (RELAYTOOL_PRESENT)

#define RELAYTOOL_JACK \
	extern int libjack_is_present;\
 	extern int libjack_symbol_is_present(char *s);

#define RELAYTOOL_WAVPACK \
	extern int libwavpack_is_present;\
	extern int libwavpack_symbol_is_present(char *s);

#define RELAYTOOL_FLAC \
	extern int libFLAC_is_present;\
 	extern int libFLAC_symbol_is_present(char *s);


#define	RELAYTOOL_MAD \
	extern int libmad_is_present;\
	extern int libmad_symbol_is_present(char *s);

#define RELAYTOOL_OGG \
	extern int libogg_is_present;\
	extern int libogg_symbol_is_present(char *s);

#define RELAYTOOL_VORBIS \
	extern int libvorbis_is_present;\
	extern int libvorbis_symbol_is_present(char *s);

#define RELAYTOOL_VORBISFILE \
	extern int libvorbisfile_is_present;\
	extern int libvorbisfile_symbol_is_present(char *s);

#define RELAYTOOL_VORBISENC \
	extern int libvorbisenc_is_present;\
	extern int libvorbisenc_symbol_is_present(char *s);


#else


#define RELAYTOOL_JACK \
 	static const int libjack_is_present=1;\
	static int __attribute__((unused)) libjack_symbol_is_present(char *) { return 1; }

#define RELAYTOOL_WAVPACK \
	static const int libwavpack_is_present=1;\
	static int __attribute__((unused)) libwavpack_symbol_is_present(char *) { return 1; }

#define RELAYTOOL_FLAC \
	static const int libFLAC_is_present=1;\
	static int __attribute__((unused)) libFLAC_symbol_is_present(char *) { return 1; }

#define RELAYTOOL_MAD \
	static const int libmad_is_present=1;\
	static int __attribute__((unused)) libmad_symbol_is_present(char *) { return 1; }

#define RELAYTOOL_OGG \
	static const int libogg_is_present=1;\
	static int __attribute__((unused)) libogg_symbol_is_present(char *) { return 1; }

#define RELAYTOOL_VORBIS \
	static const int libvorbis_is_present=1;\
	static int __attribute__((unused)) libvorbis_symbol_is_present(char *) { return 1; }

#define RELAYTOOL_VORBISFILE \
	static const int libvorbisfile_is_present=1;\
	static int __attribute__((unused)) libvorbisfile_symbol_is_present(char *) { return 1; }

#define RELAYTOOL_VORBISENC \
	static const int libvorbisenc_is_present=1;\
	static int __attribute__((unused)) libvorbisenc_symbol_is_present(char *) { return 1; }

#define RELAYTOOL_MP3LAME \
	static const int libmp3lame_is_present=1;\
	static int __attribute__((unused)) libmp3lame_symbol_is_present(char *) { return 1; }

#endif // endif RELAYTOOL_PRESENT


#define PROFILE_START trav_time_t starttime = get_microseconds();
#define PROFILE_END(args...) int processtime = (int) (get_microseconds() - starttime);printf("Process time for %s: %d useconds\n\n", args, processtime);

#endif // endif TRAVERSO_TYPES_H

//eof