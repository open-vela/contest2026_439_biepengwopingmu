############################################################################
# VelaGesture — Makefile
############################################################################

include $(APPDIR)/Make.defs

PROGNAME  = velagesture
PRIORITY  = SCHED_PRIORITY_DEFAULT
STACKSIZE = 8192
MODULE    = $(CONFIG_VELAGESTURE)

CSRCS  = src/main.c
CSRCS += src/stable_gate.c
CSRCS += src/gesture_perception.c
CSRCS += src/ws_client.c
CSRCS += src/ai_agent.c
CSRCS += src/skill_executor.c

ifeq ($(CONFIG_VELAGESTURE_AUDIO),y)
CSRCS += src/audio_io.c
endif

CFLAGS += -I$(APPDIR)/packages/demos/contest2026_439_velagesture/include

include $(APPDIR)/Application.mk
