# DS614 RocksDB WAL Experiments
# Prerequisites: sudo apt-get install librocksdb-dev
#   OR build from source: cd .. && make static_lib && make db_bench
#
# Usage:
#   make all          — compile all experiments
#   make run_all      — compile + run all, collect telemetry
#   make clean        — remove binaries

CXX      = g++
CXXFLAGS = -std=c++20 -O2 -Wall
LIBS     = -lrocksdb -lpthread

# If using locally built RocksDB (recommended for instrumented version):
#   make all ROCKSDB_ROOT=..
ifdef ROCKSDB_ROOT
  CXXFLAGS += -I$(ROCKSDB_ROOT)/include
  LIBS     = $(ROCKSDB_ROOT)/librocksdb.a -lpthread -lz -lbz2 -lsnappy -llz4 -lzstd
endif

BINDIR = ../build
RESDIR = ../results
CSVOUT = ../results/wal_performance_telemetry.csv

SOURCES = exp1_wal_throughput.cpp \
          exp2_crash_recovery.cpp \
          exp3_wal_skew.cpp       \
          exp4_batch_grouping.cpp \
          exp5_data_growth.cpp    \
          exp6_sync_modes.cpp

TARGETS = $(patsubst %.cpp,$(BINDIR)/%,$(SOURCES))

.PHONY: all run_all clean setup

setup:
	@mkdir -p $(BINDIR) $(RESDIR)
	@rm -f $(CSVOUT)
	@echo "Event,Count,Metric" > $(CSVOUT)

all: setup $(TARGETS)

$(BINDIR)/%: %.cpp
	$(CXX) $(CXXFLAGS) $< $(LIBS) -o $@

run_all: all
	@echo "=== EXP 1: WAL Throughput (WAL ON vs OFF) ===" && $(BINDIR)/exp1_wal_throughput
	@echo "=== EXP 2: Crash Recovery Durability ===" && $(BINDIR)/exp2_crash_recovery
	@echo "=== EXP 3: Skew Analysis ===" && $(BINDIR)/exp3_wal_skew
	@echo "=== EXP 4: Batch Grouping ===" && $(BINDIR)/exp4_batch_grouping
	@echo "=== EXP 5: Data Growth ===" && $(BINDIR)/exp5_data_growth
	@echo "=== EXP 6: Sync Mode Comparison ===" && $(BINDIR)/exp6_sync_modes
	@cp $(CSVOUT) ../analytics/wal_performance_telemetry.csv
	@echo "Telemetry copied to analytics/wal_performance_telemetry.csv"

clean:
	rm -rf $(BINDIR)
