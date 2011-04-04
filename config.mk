INCLUDES = -I$(top_srcdir)/include

if SH_TEXT
APPCFLAGS = @pangocairo_CFLAGS@
APPLDFLAGS = @pangocairo_LIBS@
else
APPCFLAGS = @cairo_CFLAGS@
APPLDFLAGS = @cairo_LIBS@
endif