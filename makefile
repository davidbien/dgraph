# gr_test.exe makefile for Linux clang compilation.

# First set MOD_DEFINES to specialized defines for this specific project.
#MOD_DEFINES = -D__NDEBUG_THROW
# -D__DEBUG_THROW_VERBOSE 
#MODTIDYCHECKFLAGS = -modernize-use-using,-performance-unnecessary-copy-initialization,-google-build-using-namespace

# We will use the same compiler for C and CPP files - choose the C and we will choose the CPP and options.
#CC := gcc
CC := clang
#CC := /usr/local/cuda/bin/nvcc

ifneq (1,$(NDEBUG))
ASAN_OPTIONS=check_initialization_order=1
ASAN_OPTIONS=detect_leaks=1
ASAN_OPTIONS=detect_stack_use_after_return=1
CLANG_ADDR_SANITIZE = -fsanitize=address -fsanitize-address-use-after-scope

# MSAN_OPTIONS=poison_in_dtor=1
# ASAN_OPTIONS=detect_leaks=1
# ASAN_OPTIONS=detect_stack_use_after_return=1
#CLANG_MEM_SANITIZE = -fsanitize=memory -fsanitize-memory-track-origins -fsanitize-memory-use-after-dtor
CLANGSANITIZE = $(CLANG_ADDR_SANITIZE) $(CLANG_MEM_SANITIZE) -fno-omit-frame-pointer
#-fsanitize-blacklist=blacklist.txt 
endif # !NDEBUG

MAKEBASE = ../bienutil/makebase.mk
include $(MAKEBASE)

# tcmalloc performs about 10% better when running dgraph unit testing. nice.
ifneq (1,$(NDEBUG))
MOD_LIBS += -ltcmalloc_debug
else
#MOD_LIBS += /usr/lib/x86_64-linux-gnu/libtcmalloc.so.4.3.0
#MOD_LIBS += /usr/lib/x86_64-linux-gnu/libtcmalloc_minimal.so.4 -- minimal seems a bit slower.
MOD_LIBS += -ltcmalloc
endif

SRCS = _gr_test.cpp dbgthrw.cpp

ifeq (1,$(TIDY))
all: tidy
# The tidy build will invoke default rules to produce the *.tidy files.
tidy: $(patsubst %,$(BUILD_DIR)/%.tidy,$(basename $(SRCS)))
else
all: $(BUILD_DIR)/gr_test.exe
$(BUILD_DIR)/gr_test.exe: $(patsubst %,$(BUILD_DIR)/%.o,$(basename $(SRCS)))
	$(CXX) $(LINKFLAGS) -o $(BUILD_DIR)/gr_test.exe $(patsubst %,$(BUILD_DIR)/%.o,$(basename $(SRCS)))
include $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS))))
endif
