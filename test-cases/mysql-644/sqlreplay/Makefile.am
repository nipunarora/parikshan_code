
WOLDSTYLE=
#WOLDSTYLE=-Wold-style-cast  -Wunreachable-code -Winline
WARNING_FLAGS=$(WOLDSTYLE) -Wall -W -Wformat=2 -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings -Wconversion -Wmissing-noreturn -Wmissing-format-attribute -Wpacked -Wredundant-decls
AM_CFLAGS= $(WARNING_FLAGS) -Wbad-function-cast -Wmissing-prototypes  -Wnested-externs -Wstrict-prototypes
#  $(AM_CFLAGS)
AM_CXXFLAGS= $(WARNING_FLAGS) -fmessage-length=0 -Wctor-dtor-privacy -Wnon-virtual-dtor  -Wreorder -Wsign-promo -fno-rtti 
#$(AM_CXXFLAGS)

if MYSQL

bin_PROGRAMS= runtran

endif

LIBS="${PTHREAD_LIBS}"
CFLAGS="${PTHREAD_CFLAGS}"
AM_CXXFLAGS+="${PTHREAD_CFLAGS}"
CC="${PTHREAD_CC}"


runtran_SOURCES= runtran.cc

